/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    harderr.c

Abstract:

    This module implements NT Hard Error APIs

Author:

    Mark Lucovsky (markl) 04-Jul-1991

Revision History:

--*/

#include "exp.h"
#include "zwapi.h"
#include "stdio.h"

extern ULONG KiBugCheckData[5];

NTSTATUS
ExpRaiseHardError(
    IN NTSTATUS ErrorStatus,
    IN ULONG NumberOfParameters,
    IN ULONG UnicodeStringParameterMask,
    IN PULONG Parameters,
    IN ULONG ValidResponseOptions,
    OUT PULONG Response
    );

VOID
ExpSystemErrorHandler(
    IN NTSTATUS ErrorStatus,
    IN ULONG NumberOfParameters,
    IN ULONG UnicodeStringParameterMask,
    IN PULONG Parameters,
    IN BOOLEAN CallShutdown
    );

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE, NtRaiseHardError)
#pragma alloc_text(PAGE, NtSetDefaultHardErrorPort)
#pragma alloc_text(PAGE, ExRaiseHardError)
#pragma alloc_text(PAGE, ExpRaiseHardError)
#pragma alloc_text(PAGELK, ExpSystemErrorHandler)
#endif

#define HARDERROR_MSG_OVERHEAD (sizeof(HARDERROR_MSG) - sizeof(PORT_MESSAGE))
#define HARDERROR_API_MSG_LENGTH \
            sizeof(HARDERROR_MSG)<<16 | (HARDERROR_MSG_OVERHEAD)

PEPROCESS ExpDefaultErrorPortProcess;
BOOLEAN ExpReadyForErrors = FALSE;
BOOLEAN ExpTooLateForErrors = FALSE;
HANDLE ExpDefaultErrorPort;
extern PVOID PsSystemDllDllBase;

VOID
ExpSystemErrorHandler(
    IN NTSTATUS ErrorStatus,
    IN ULONG NumberOfParameters,
    IN ULONG UnicodeStringParameterMask,
    IN PULONG Parameters,
    IN BOOLEAN CallShutdown
    )
{

    ULONG Counter;
    ANSI_STRING AnsiString;
    NTSTATUS Status;
    ULONG ParameterVector[MAXIMUM_HARDERROR_PARAMETERS];
    CHAR DefaultFormatBuffer[32];
    CHAR ExpSystemErrorBuffer[256];
    PMESSAGE_RESOURCE_ENTRY MessageEntry;
    PSZ ErrorCaption;
    PSZ ErrorFormatString;
    ANSI_STRING Astr;
    UNICODE_STRING Ustr;
    OEM_STRING Ostr;
    PSZ OemCaption;
    PSZ OemMessage;
    PSZ UnknownHardError = "Unknown Hard Error";
    PVOID UnlockHandle;

    //
    // This handler is called whenever a hard error occurs before the
    // default handler has been installed.
    //
    // This is done regardless of whether or not the process has chosen
    // default hard error processing.
    //


    //
    // Capture the callers context as closely as possible into the debugger's
    // processor state area of the Prcb
    //
    // N.B. There may be some prologue code that shuffles registers such that
    //      they get destroyed.
    //
    // this code is here only for crash dumps
    RtlCaptureContext(&KeGetCurrentPrcb()->ProcessorState.ContextFrame);
    KiSaveProcessorControlState(&KeGetCurrentPrcb()->ProcessorState);


    DefaultFormatBuffer[0] = '\0';
    RtlZeroMemory(ParameterVector,sizeof(ParameterVector));
    for(Counter=0;Counter < NumberOfParameters;Counter++){
        ParameterVector[Counter] = Parameters[Counter];
        }

    for(Counter=0;Counter < NumberOfParameters;Counter++){
        if ( UnicodeStringParameterMask & 1<<Counter ) {
            strcat(DefaultFormatBuffer," %s");
            RtlUnicodeStringToAnsiString(&AnsiString,(PUNICODE_STRING)Parameters[Counter],TRUE);
            ParameterVector[Counter] = (ULONG)AnsiString.Buffer;
            }
        else {
            strcat(DefaultFormatBuffer," %x");
            }
        }
    strcat(DefaultFormatBuffer,"\n");

    //
    // HELP where do I get the resource from !
    //

    if ( PsSystemDllDllBase ) {
        try {
            Status = RtlFindMessage(PsSystemDllDllBase, 11, 0,
                                    ErrorStatus, &MessageEntry);
            if (!NT_SUCCESS(Status)) {
                ErrorFormatString = UnknownHardError;
                ErrorCaption = UnknownHardError;
                }
            else {
                ErrorCaption = ExAllocatePool(NonPagedPool,strlen(MessageEntry->Text)+16);
                strcpy(ErrorCaption,MessageEntry->Text);
                ErrorFormatString = ErrorCaption;
                while ( *ErrorFormatString >= ' ' ) {
                    ErrorFormatString++;
                    }
                *ErrorFormatString++ = '\0';
                while ( *ErrorFormatString && *ErrorFormatString <= ' ') {
                    *ErrorFormatString++;
                    }
                }
            }
        except ( EXCEPTION_EXECUTE_HANDLER ) {
            ErrorFormatString = UnknownHardError;
            ErrorCaption = UnknownHardError;
            }
        }
    else {
        ErrorFormatString = DefaultFormatBuffer;
        ErrorCaption = UnknownHardError;
        }

    try {
        _snprintf( ExpSystemErrorBuffer, sizeof( ExpSystemErrorBuffer ),
                   "\nSTOP: %lx %s\n", ErrorStatus,ErrorCaption);
        }
    except(EXCEPTION_EXECUTE_HANDLER) {
        _snprintf( ExpSystemErrorBuffer, sizeof( ExpSystemErrorBuffer ),
                   "\nHardError %lx\n", ErrorStatus);
        }

    UnlockHandle = MmLockPagableImageSection((PVOID)ExpSystemErrorHandler);
    ASSERT(UnlockHandle);

    if (CallShutdown) {
        ZwShutdownSystem( FALSE );
        }

    //
    // take the caption and convert it to OEM
    //

    OemCaption = UnknownHardError;
    OemMessage = UnknownHardError;

    RtlInitAnsiString(&Astr,ExpSystemErrorBuffer);
    Status = RtlAnsiStringToUnicodeString(&Ustr,&Astr,TRUE);
    if ( !NT_SUCCESS(Status) ) {
        goto punt1;
        }
    Status = RtlUnicodeStringToOemString(&Ostr,&Ustr,TRUE);
    if ( !NT_SUCCESS(Status) ) {
        goto punt1;
        }
    OemCaption = Ostr.Buffer;

    //
    // Can't do much of anything after calling HalDisplayString...
    //

punt1:;
    try {
        _snprintf( ExpSystemErrorBuffer, sizeof( ExpSystemErrorBuffer ),
                   ErrorFormatString,
                   ParameterVector[0],
                   ParameterVector[1],
                   ParameterVector[2],
                   ParameterVector[3]
                 );
        }
    except(EXCEPTION_EXECUTE_HANDLER) {
        _snprintf( ExpSystemErrorBuffer, sizeof( ExpSystemErrorBuffer ),
                   "Exception Processing Message %lx Parameters %lx %lx %lx %lx",
                   ErrorStatus,
                   ParameterVector[0],
                   ParameterVector[1],
                   ParameterVector[2],
                   ParameterVector[3]
                 );
        }


    RtlInitAnsiString(&Astr,ExpSystemErrorBuffer);
    Status = RtlAnsiStringToUnicodeString(&Ustr,&Astr,TRUE);
    if ( !NT_SUCCESS(Status) ) {
        goto punt2;
        }
    Status = RtlUnicodeStringToOemString(&Ostr,&Ustr,TRUE);
    if ( !NT_SUCCESS(Status) ) {
        goto punt2;
        }
    OemMessage = Ostr.Buffer;

punt2:;
    HalDisplayString( OemCaption );
    HalDisplayString( OemMessage );

    if (CallShutdown) {
#if DBG
        DbgPrint( "EX: Typing go from this breakpoint will shutdown and restart the system.\n" );
        DbgBreakPoint();
        DbgUnLoadImageSymbols( NULL, (PVOID)-1, 0 );
        HalReturnToFirmware( HalRebootRoutine );
#endif
        }

    //
    // Attempt to enter the kernel debugger.
    //

    KiBugCheckData[0] = ErrorStatus;
    KiBugCheckData[1] = ParameterVector[0];
    KiBugCheckData[2] = ParameterVector[1];
    KiBugCheckData[3] = ParameterVector[2];
    KiBugCheckData[4] = ParameterVector[3];

    //
    // attempt to write a crash dump
    //
    IoWriteCrashDump( ErrorStatus,
                      ParameterVector[0],
                      ParameterVector[1],
                      ParameterVector[2],
                      ParameterVector[3]
                    );

    //
    // if we get here then a crash dump was not invoked
    //

    while(TRUE) {
        try {

            DbgBreakPoint();

        } except(EXCEPTION_EXECUTE_HANDLER) {

            for (;;) {
            }

        }
    }
}

NTSTATUS
ExpRaiseHardError(
    IN NTSTATUS ErrorStatus,
    IN ULONG NumberOfParameters,
    IN ULONG UnicodeStringParameterMask,
    IN PULONG Parameters,
    IN ULONG ValidResponseOptions,
    OUT PULONG Response
    )
{

    PEPROCESS Process;
    HARDERROR_MSG m;
    NTSTATUS Status;
    HANDLE ErrorPort;
    KPROCESSOR_MODE PreviousMode;

    PAGED_CODE();

    PreviousMode = KeGetPreviousMode();

    if (ValidResponseOptions == OptionShutdownSystem) {
        //
        // Check to see if the caller has the privilege to make this
        // call.
        //


        if (!SeSinglePrivilegeCheck( SeShutdownPrivilege, PreviousMode )) {
            return STATUS_PRIVILEGE_NOT_HELD;
            }

        ExpReadyForErrors = FALSE;
        }

    Process = PsGetCurrentProcess();

    //
    // If the default handler is not installed, then
    // call the fatal hard error handler if the error
    // status is error
    //

    if (ExpReadyForErrors == FALSE && NT_ERROR(ErrorStatus)){
        ExpSystemErrorHandler(
            ErrorStatus,
            NumberOfParameters,
            UnicodeStringParameterMask,
            Parameters,
            (BOOLEAN)((PreviousMode != KernelMode) ? TRUE : FALSE)
            );
        }

    //
    // If the process has an error port, then if it wants default
    // handling, use its port. If it disabled default handling, then
    // return the error to the caller. If the process does not
    // have a port, then use the registered default handler.
    //

    if ( Process->ExceptionPort ) {
        if ( Process->DefaultHardErrorProcessing & 1 ) {
            ErrorPort = Process->ExceptionPort;
            }
        else {

            //
            // if error processing is disabled, check the error override
            // status
            //
            if ( ErrorStatus & 0x10000000 ) {
                ErrorPort = Process->ExceptionPort;
                }
            else {
                ErrorPort = NULL;
                }
            }
        }
    else {
        if ( Process->DefaultHardErrorProcessing & 1 ) {
            ErrorPort = ExpDefaultErrorPort;
            }
        else {

            //
            // if error processing is disabled, check the error override
            // status
            //

            if ( ErrorStatus & 0x10000000 ) {
                ErrorPort = ExpDefaultErrorPort;
                }
            else {
                ErrorPort = NULL;
                }
            ErrorPort = NULL;
            }
        }

    if ( PsGetCurrentThread()->HardErrorsAreDisabled ) {
        ErrorPort = NULL;
        }

    if ( ErrorPort ) {
        if ( Process == ExpDefaultErrorPortProcess ) {
            if ( NT_ERROR(ErrorStatus) ) {
                ExpSystemErrorHandler(
                    ErrorStatus,
                    NumberOfParameters,
                    UnicodeStringParameterMask,
                    Parameters,
                    (BOOLEAN)((PreviousMode != KernelMode) ? TRUE : FALSE)
                    );
                }
            *Response = (ULONG)ResponseReturnToCaller;
            Status = STATUS_SUCCESS;
            return Status;
            }
        m.h.u1.Length = HARDERROR_API_MSG_LENGTH;
        m.h.u2.ZeroInit = LPC_ERROR_EVENT;
        m.Status = ErrorStatus & 0xefffffff;
        m.ValidResponseOptions = ValidResponseOptions;
        m.UnicodeStringParameterMask = UnicodeStringParameterMask;
        m.NumberOfParameters = NumberOfParameters;
        if ( Parameters ) {
            RtlMoveMemory(&m.Parameters,Parameters, sizeof(ULONG)*NumberOfParameters);
            }
        KeQuerySystemTime(&m.ErrorTime);

        Status = LpcRequestWaitReplyPort(
                    ErrorPort,
                    (PPORT_MESSAGE) &m,
                    (PPORT_MESSAGE) &m
                    );
        if ( NT_SUCCESS(Status) ) {
            switch ( m.Response ) {
                case ResponseReturnToCaller :
                case ResponseNotHandled :
                case ResponseAbort :
                case ResponseCancel :
                case ResponseIgnore :
                case ResponseNo :
                case ResponseOk :
                case ResponseRetry :
                case ResponseYes :
                    break;
                default:
                    m.Response = (ULONG)ResponseReturnToCaller;
                    break;
                }
            *Response = m.Response;
            }
        }
    else {
        *Response = (ULONG)ResponseReturnToCaller;
        Status = STATUS_SUCCESS;
        }
    return Status;
}

NTSTATUS
NtRaiseHardError(
    IN NTSTATUS ErrorStatus,
    IN ULONG NumberOfParameters,
    IN ULONG UnicodeStringParameterMask,
    IN PULONG Parameters,
    IN ULONG ValidResponseOptions,
    OUT PULONG Response
    )
{
    NTSTATUS Status;
    PULONG CapturedParameters;
    KPROCESSOR_MODE PreviousMode;
    ULONG LocalResponse;
    UNICODE_STRING CapturedString;
    ULONG Counter;

    PAGED_CODE();

    if ( NumberOfParameters > MAXIMUM_HARDERROR_PARAMETERS ) {
        return STATUS_INVALID_PARAMETER_2;
        }

    PreviousMode = KeGetPreviousMode();
    if (PreviousMode != KernelMode) {
        switch ( ValidResponseOptions ) {
            case OptionAbortRetryIgnore :
            case OptionOk :
            case OptionOkCancel :
            case OptionRetryCancel :
            case OptionYesNo :
            case OptionYesNoCancel :
            case OptionShutdownSystem :
                break;
            default :
                return STATUS_INVALID_PARAMETER_4;
            }

        CapturedParameters = NULL;
        try {
            ProbeForWriteUlong(Response);

            if ( ARGUMENT_PRESENT(Parameters) ) {
                ProbeForRead(
                    Parameters,
                    sizeof(ULONG)*NumberOfParameters,
                    sizeof(ULONG)
                    );
                CapturedParameters = ExAllocatePool(PagedPool,sizeof(ULONG)*NumberOfParameters);
                if ( !CapturedParameters ) {
                    return STATUS_NO_MEMORY;
                    }
                RtlMoveMemory(CapturedParameters,Parameters,sizeof(ULONG)*NumberOfParameters);

                //
                // probe all strings
                //

                if ( UnicodeStringParameterMask ) {
                    for(Counter=0;Counter < NumberOfParameters;Counter++){

                        //
                        // if there is a string in this position,
                        // then probe and capture the string
                        //

                        if ( UnicodeStringParameterMask & 1<<Counter ) {
                            ProbeForRead(
                                (PVOID)CapturedParameters[Counter],
                                sizeof(UNICODE_STRING),
                                sizeof(ULONG)
                                );
                            RtlMoveMemory(
                                &CapturedString,
                                (PVOID)CapturedParameters[Counter],
                                sizeof(UNICODE_STRING)
                                );

                            //
                            // Now probe the string
                            //

                            ProbeForRead(
                                CapturedString.Buffer,
                                CapturedString.MaximumLength,
                                sizeof(UCHAR)
                                );

                            }
                        }
                    }
                }
            else {
                CapturedParameters = NULL;
                }
            }
        except(EXCEPTION_EXECUTE_HANDLER) {
            if ( CapturedParameters ) {
                ExFreePool(CapturedParameters);
                }
            return GetExceptionCode();
            }

        //
        // Call ExpRaiseHardError. All parameters are probed and everything
        // should be user-mode.
        // ExRaiseHardError will squirt all strings into user-mode
        // without any probing
        //

        Status = ExpRaiseHardError(
                    ErrorStatus,
                    NumberOfParameters,
                    UnicodeStringParameterMask,
                    CapturedParameters,
                    ValidResponseOptions,
                    &LocalResponse
                    );
        }
    else {
        CapturedParameters = Parameters;

        Status = ExRaiseHardError(
                    ErrorStatus,
                    NumberOfParameters,
                    UnicodeStringParameterMask,
                    CapturedParameters,
                    ValidResponseOptions,
                    &LocalResponse
                    );
        }

    if (PreviousMode != KernelMode) {
        if ( CapturedParameters ) {
            ExFreePool(CapturedParameters);
            }
        try {
            *Response = LocalResponse;
            }
        except (EXCEPTION_EXECUTE_HANDLER) {
            return Status;
            }
        }
    else {
        *Response = LocalResponse;
        }

    return Status;
}

NTSTATUS
ExRaiseHardError(
    IN NTSTATUS ErrorStatus,
    IN ULONG NumberOfParameters,
    IN ULONG UnicodeStringParameterMask,
    IN PULONG Parameters,
    IN ULONG ValidResponseOptions,
    OUT PULONG Response
    )
{
    NTSTATUS Status;
    PULONG ParameterBlock;
    PULONG UserModeParameterBase;
    PUNICODE_STRING UserModeStringsBase;
    PUCHAR UserModeStringDataBase;
    UNICODE_STRING CapturedStrings[MAXIMUM_HARDERROR_PARAMETERS];
    ULONG LocalResponse;
    ULONG Counter;
    ULONG UserModeSize;

    PAGED_CODE();

    //
    //  If we are in the process of shuting down the system, do not allow
    //  hard errors.
    //

    if ( ExpTooLateForErrors ) {

        *Response = ResponseNotHandled;

        return STATUS_SUCCESS;
    }

    //
    // If the parameters contain strings, we need to capture
    // the strings and the string descriptors and push them into
    // user-mode.
    //

    if ( ARGUMENT_PRESENT(Parameters) ) {
        if ( UnicodeStringParameterMask ) {

            //
            // We have strings. The parameter block and all strings
            // must be pushed into usermode.
            //

            UserModeSize = (sizeof(ULONG)+sizeof(UNICODE_STRING))*MAXIMUM_HARDERROR_PARAMETERS;
            UserModeSize += sizeof(UNICODE_STRING);

            for(Counter=0;Counter < NumberOfParameters;Counter++){

                //
                // if there is a string in this position,
                // then probe and capture the string
                //

                if ( UnicodeStringParameterMask & 1<<Counter ) {

                    RtlMoveMemory(
                        &CapturedStrings[Counter],
                        (PVOID)Parameters[Counter],
                        sizeof(UNICODE_STRING)
                        );

                    UserModeSize += CapturedStrings[Counter].MaximumLength;

                    }
                }

            //
            // Now we have the user-mode size all figured out.
            // Allocate some memory and point to it with the
            // parameter block. Then go through and copy all
            // of the parameters, string descriptors, and
            // string data into the memory
            //

            ParameterBlock = NULL;
            Status = ZwAllocateVirtualMemory(
                        NtCurrentProcess(),
                        (PVOID *)&ParameterBlock,
                        0,
                        &UserModeSize,
                        MEM_COMMIT,
                        PAGE_READWRITE
                        );

            if (!NT_SUCCESS( Status )) {
                return( Status );
                }

            UserModeParameterBase = ParameterBlock;
            UserModeStringsBase = (PUNICODE_STRING)((PUCHAR)ParameterBlock + sizeof(ULONG)*MAXIMUM_HARDERROR_PARAMETERS);
            UserModeStringDataBase = (PUCHAR)UserModeStringsBase + sizeof(UNICODE_STRING)*MAXIMUM_HARDERROR_PARAMETERS;

            for(Counter=0;Counter < NumberOfParameters;Counter++){

                //
                // move parameters to user-mode portion of the
                // address space.
                //

                if ( UnicodeStringParameterMask & 1<<Counter ) {

                    //
                    // fix the parameter to point at the string descriptor slot
                    // in the user-mode buffer.
                    //

                    UserModeParameterBase[Counter] = (ULONG)&UserModeStringsBase[Counter];

                    //
                    // Copy the string data to user-mode
                    //

                    RtlMoveMemory(
                        UserModeStringDataBase,
                        CapturedStrings[Counter].Buffer,
                        CapturedStrings[Counter].MaximumLength
                        );

                    CapturedStrings[Counter].Buffer = (PWSTR)UserModeStringDataBase;

                    //
                    // copy the string descriptor
                    //

                    RtlMoveMemory(
                        &UserModeStringsBase[Counter],
                        &CapturedStrings[Counter],
                        sizeof(UNICODE_STRING)
                        );

                    //
                    // Adjust the string data base
                    //

                    UserModeStringDataBase += CapturedStrings[Counter].MaximumLength;

                    }
                else {
                    UserModeParameterBase[Counter] = Parameters[Counter];
                    }
                }
            }
        else {
            ParameterBlock = Parameters;
            }
        }
    else {
        ParameterBlock = NULL;
        }

    //
    // Call the hard error sender.
    //

    Status = ExpRaiseHardError(
                ErrorStatus,
                NumberOfParameters,
                UnicodeStringParameterMask,
                ParameterBlock,
                ValidResponseOptions,
                &LocalResponse
                );
    //
    // If the parameter block was allocated, it needs to be
    // freed
    //

    if ( ParameterBlock && ParameterBlock != Parameters ) {
        UserModeSize = 0;
        ZwFreeVirtualMemory(
              NtCurrentProcess(),
              (PVOID *)&ParameterBlock,
              &UserModeSize,
              MEM_RELEASE
              );
        }
    *Response = LocalResponse;

    return Status;

}

NTSTATUS
NtSetDefaultHardErrorPort(
    IN HANDLE DefaultHardErrorPort
    )
{
    NTSTATUS Status;

    PAGED_CODE();

    if (!SeSinglePrivilegeCheck( SeTcbPrivilege, KeGetPreviousMode() )) {
        return STATUS_PRIVILEGE_NOT_HELD;
        }

    if ( ExpReadyForErrors ) {
        return STATUS_UNSUCCESSFUL;
        }

    //
    // Priv check ?
    //

    Status = ObReferenceObjectByHandle (
                DefaultHardErrorPort,
                0,
                LpcPortObjectType,
                KeGetPreviousMode(),
                (PVOID *)&ExpDefaultErrorPort,
                NULL
                );
    if ( !NT_SUCCESS(Status) ) {
        return Status;
        }

    ExpReadyForErrors = TRUE;
    ExpDefaultErrorPortProcess = PsGetCurrentProcess();

    return STATUS_SUCCESS;
}

VOID
_CRTAPI1
_purecall()
{
    ASSERTMSG("_purecall() was called", FALSE);
    ExRaiseStatus(STATUS_NOT_IMPLEMENTED);
}
