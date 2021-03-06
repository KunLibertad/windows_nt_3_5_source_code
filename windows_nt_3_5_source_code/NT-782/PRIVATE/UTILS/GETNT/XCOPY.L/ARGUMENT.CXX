/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

	Argument

Abstract:

	Argument processing for the XCopy directory copy utility

Author:

	Ramon Juan San Andres (ramonsa) 01-May-1991

Notes:

	The arguments accepted	by the XCopy utility are:

	Source directory.-	Source path.

	Dest. directory.-	Destination path.

	Archive switch.-	Copy files that have their archive bit set

	Date.-				Copy files modified on or after the specifiec
						date.

	Empty switch.-		Copy directories even if empty. Subdir switch
						must also be set.

	Modify switch.- 	Same as Archive switch, but turns off archive
						bit ins the source file after copying.

	Prompt switch.- 	Prompts before copying each file.

	Subdir switch.- 	Copies also subdirectories, unless they are empty.
						(Empty directories are copied if the Empty switch
						is set).

	Verify switch.- 	Verifies each copy.

	Wait switch.-		Wait before starting to copy the files.

Revision History:


--*/


#include "ulib.hxx"
#include "arg.hxx"
#include "arrayit.hxx"
#include "dir.hxx"
#include "xcopy.hxx"

#ifdef GETNT

    extern "C" PCSTR szCmdLine;
    extern PCSTR szSourcePath;
    extern "C" BOOL fUsage;

#endif

//
//	Switch characters. Used for maintaining DOS5 compatibility when
//	displaying error messages
//
#define SWITCH_CHARACTERS	"dDaAeEmMpPsSvVwW?"

//
//	Static variables
//

PARRAY              LexArray;
PPATH_ARGUMENT      FirstPathArgument       =   NULL;
PPATH_ARGUMENT      FirstQuotedPathArgument =   NULL;
PPATH_ARGUMENT      SecondPathArgument      =   NULL;
PPATH_ARGUMENT      SecondQuotedPathArgument = NULL;
PFLAG_ARGUMENT		ArchiveArgument 		=	NULL;
PTIMEINFO_ARGUMENT  DateArgument            =   NULL;
PFLAG_ARGUMENT		EmptyArgument			=	NULL;
PFLAG_ARGUMENT		ModifyArgument			=	NULL;
PFLAG_ARGUMENT		PromptArgument			=	NULL;
PFLAG_ARGUMENT		SubdirArgument			=	NULL;
PFLAG_ARGUMENT		VerifyArgument			=	NULL;
PFLAG_ARGUMENT		WaitArgument			=	NULL;
PFLAG_ARGUMENT      HelpArgument            =   NULL;
PFLAG_ARGUMENT      ContinueArgument        =   NULL;

PFLAG_ARGUMENT      IntelligentArgument     =   NULL;
PFLAG_ARGUMENT      VerboseArgument         =   NULL;
PFLAG_ARGUMENT      OldArgument             =   NULL;
PFLAG_ARGUMENT      HiddenArgument          =   NULL;
PFLAG_ARGUMENT      ReadOnlyArgument        =   NULL;
PFLAG_ARGUMENT      SilentArgument          =   NULL;
PFLAG_ARGUMENT      NoCopyArgument          =   NULL;
PFLAG_ARGUMENT      StructureArgument       =   NULL;
PFLAG_ARGUMENT      UpdateArgument          =   NULL;
PFLAG_ARGUMENT      CopyAttrArgument        =   NULL;
PFLAG_ARGUMENT      UseShortArgument        =   NULL;

PSTRING_ARGUMENT	InvalidSwitchArgument	=	NULL;


BOOLEAN 			HelpSwitch;

//
//	Prototypes
//


VOID
XCOPY::SetArguments(
	)

/*++

Routine Description:

	Obtains the arguments for the XCopy utility

Arguments:

    None.

Return Value:

    None.

Notes:

--*/

{

    PATH_ARGUMENT       LocalFirstPathArgument;
    PATH_ARGUMENT       LocalFirstQuotedPathArgument;
    PATH_ARGUMENT       LocalSecondPathArgument;
    PATH_ARGUMENT       LocalSecondQuotedPathArgument;
	FLAG_ARGUMENT		LocalArchiveArgument;
    TIMEINFO_ARGUMENT   LocalDateArgument;
    FLAG_ARGUMENT       LocalOldArgument;
	FLAG_ARGUMENT		LocalEmptyArgument;
	FLAG_ARGUMENT		LocalModifyArgument;
	FLAG_ARGUMENT		LocalPromptArgument;
	FLAG_ARGUMENT		LocalSubdirArgument;
	FLAG_ARGUMENT		LocalVerifyArgument;
	FLAG_ARGUMENT		LocalWaitArgument;
    FLAG_ARGUMENT       LocalHelpArgument;
    FLAG_ARGUMENT       LocalContinueArgument;
    FLAG_ARGUMENT       LocalIntelligentArgument;
    FLAG_ARGUMENT       LocalVerboseArgument;
    FLAG_ARGUMENT       LocalHiddenArgument;
    FLAG_ARGUMENT       LocalReadOnlyArgument;
    FLAG_ARGUMENT       LocalSilentArgument;
    FLAG_ARGUMENT       LocalNoCopyArgument;
    FLAG_ARGUMENT       LocalStructureArgument;
    FLAG_ARGUMENT       LocalUpdateArgument;
    FLAG_ARGUMENT       LocalCopyAttrArgument;
    FLAG_ARGUMENT       LocalUseShortArgument;


	STRING_ARGUMENT 	LocalInvalidSwitchArgument;
	ARRAY				LocalLexArray;

	//
	//	Set the static global pointers
	//
    FirstPathArgument         = &LocalFirstPathArgument;
    FirstQuotedPathArgument   = &LocalFirstQuotedPathArgument;
    SecondPathArgument        = &LocalSecondPathArgument;
    SecondQuotedPathArgument  = &LocalSecondQuotedPathArgument;
	ArchiveArgument 		  = &LocalArchiveArgument;
    DateArgument              = &LocalDateArgument;
    OldArgument               = &LocalOldArgument;
	EmptyArgument			  = &LocalEmptyArgument;
	ModifyArgument			  = &LocalModifyArgument;
	PromptArgument			  = &LocalPromptArgument;
	SubdirArgument			  = &LocalSubdirArgument;
	VerifyArgument			  = &LocalVerifyArgument;
	WaitArgument			  = &LocalWaitArgument;
    HelpArgument              = &LocalHelpArgument;
    ContinueArgument          = &LocalContinueArgument;
    IntelligentArgument       = &LocalIntelligentArgument;
    VerboseArgument           = &LocalVerboseArgument;
    HiddenArgument            = &LocalHiddenArgument;
    ReadOnlyArgument          = &LocalReadOnlyArgument;
    SilentArgument            = &LocalSilentArgument;
    NoCopyArgument            = &LocalNoCopyArgument;
    StructureArgument         = &LocalStructureArgument;
    UpdateArgument            = &LocalUpdateArgument;
    CopyAttrArgument          = &LocalCopyAttrArgument;
    UseShortArgument          = &LocalUseShortArgument;
	InvalidSwitchArgument	  = &LocalInvalidSwitchArgument;
	LexArray				  = &LocalLexArray;

	//
	//	Parse the arguments
	//
	GetArgumentsCmd();

	//
	//	Verify the arguments
	//
	CheckArgumentConsistency();

    LocalLexArray.DeleteAllMembers();

}


VOID
GetSourceAndDestinationPath(
    IN OUT  PPATH_ARGUMENT      FirstPathArgument,
    IN OUT  PPATH_ARGUMENT      FirstQuotedPathArgument,
    IN OUT  PPATH_ARGUMENT      SecondPathArgument,
    IN OUT  PPATH_ARGUMENT      SecondQuotedPathArgument,
    IN OUT  PARGUMENT_LEXEMIZER ArgLex,
    OUT     PPATH*              SourcePath,
    OUT     PPATH*              DestinationPath
    )
/*++

Routine Description:

    This routine computes the Source and Destination path from
    the given list of arguments.

Arguments:

    FirstPathArgument           - Supplies the first unquoted path argument.
    FirstQuotedPathArgument     - Supplies the first quoted path argument.
    SecondPathArgument          - Supplies the second unquoted path argument.
    SecondQuotedPathArgument    - Supplies the second quoted path argument.
    ArgLex                      - Supplies the argument lexemizer.
    SourcePath                  - Returns the source path.
    DestinationPath             - Returns the destination path.

Return Value:

    None.

--*/
{
    BOOLEAN         f, qf, s, qs;
    PPATH_ARGUMENT  source, destination;
    ULONG           i;
    PWSTRING        string, qstring;

    f = FirstPathArgument->IsValueSet();
    qf = FirstQuotedPathArgument->IsValueSet();
    s = SecondPathArgument->IsValueSet();
    qs = SecondQuotedPathArgument->IsValueSet();
    source = NULL;
    destination = NULL;
    *SourcePath = NULL;
    *DestinationPath = NULL;

    if (f && !qf && s && !qs) {

        source = FirstPathArgument;
        destination = SecondPathArgument;

    } else if (!f && qf && !s && qs) {

        source = FirstQuotedPathArgument;
        destination = SecondQuotedPathArgument;

    } else if (f && qf && !s && !qs) {

        string = FirstPathArgument->GetLexeme();
        qstring = FirstQuotedPathArgument->GetLexeme();

        for (i = 0; i < ArgLex->QueryLexemeCount(); i++) {
            if (!ArgLex->GetLexemeAt(i)->Strcmp(string)) {
                source = FirstPathArgument;
                destination = FirstQuotedPathArgument;
                break;
            }

            if (!ArgLex->GetLexemeAt(i)->Strcmp(qstring)) {
                source = FirstQuotedPathArgument;
                destination = FirstPathArgument;
                break;
            }
        }
    } else if (f && !qf && !s && !qs) {
        source = FirstPathArgument;
    } else if (!f && qf && !s && !qs) {
        source = FirstQuotedPathArgument;
    }

    if (source) {
        if (!(*SourcePath = NEW PATH) ||
            !(*SourcePath)->Initialize(source->GetPath(),
                                       VerboseArgument->IsValueSet())) {

            *SourcePath = NULL;
        }
    }

    if (destination) {
        if (!(*DestinationPath = NEW PATH) ||
            !(*DestinationPath)->Initialize(destination->GetPath(),
                                            VerboseArgument->IsValueSet())) {

            *DestinationPath = NULL;
        }
    }
}



VOID
XCOPY::GetArgumentsCmd(
	)

/*++

Routine Description:

	Obtains the arguments from the Command line

Arguments:

    None.

Return Value:

	None

Notes:

--*/

{

	ARRAY				ArgArray;
	PATH_ARGUMENT		ProgramNameArgument;
    DSTRING             CmdLine;
    DSTRING             InvalidParms;
	WCHAR				Ch;
    PWSTRING            InvalidSwitch;
    PARGUMENT_LEXEMIZER ArgLex;

	//
	//	Prepare for parsing
	//
	if (//
		//	Initialize the arguments
		//
#ifdef GETNT
                !(CmdLine.Initialize( szCmdLine ))                      ||
#else
                !(CmdLine.Initialize( GetCommandLine() ))                       ||
#endif // GETNT
		!(ArgArray.Initialize( 15, 15 ))					||
		!(ProgramNameArgument.Initialize( "*" ))			||
        !(FirstPathArgument->Initialize( "*",  FALSE ))     ||
        !(FirstQuotedPathArgument->Initialize( "\"*\"",
                                                FALSE ))    ||
        !(SecondPathArgument->Initialize( "*", FALSE))      ||
        !(SecondQuotedPathArgument->Initialize( "\"*\"",
                                                    FALSE)) ||
		!(ArchiveArgument->Initialize( "/A"	))				||
        !(DateArgument->Initialize( "/D:*" ))               ||
        !(OldArgument->Initialize( "/D" ))                  ||
		!(EmptyArgument->Initialize( "/E" ))				||
		!(ModifyArgument->Initialize( "/M" ))				||
		!(PromptArgument->Initialize( "/P" ))				||
		!(SubdirArgument->Initialize( "/S" ))				||
		!(VerifyArgument->Initialize( "/V" ))				||
        !(WaitArgument->Initialize( "/W" ))                 ||
        !(HelpArgument->Initialize( "/?" ))                 ||
        !(ContinueArgument->Initialize( "/C" ))             ||
        !(IntelligentArgument->Initialize( "/I" ))          ||
        !(VerboseArgument->Initialize( "/F" ))              ||
        !(HiddenArgument->Initialize( "/H" ))               ||
        !(ReadOnlyArgument->Initialize( "/R" ))             ||
        !(SilentArgument->Initialize( "/Q" ))               ||
        !(NoCopyArgument->Initialize( "/L" ))               ||
        !(StructureArgument->Initialize( "/T" ))            ||
        !(UpdateArgument->Initialize( "/U" ))               ||
        !(CopyAttrArgument->Initialize( "/K" ))             ||
        !(UseShortArgument->Initialize( "/N" ))             ||
        !(InvalidSwitchArgument->Initialize( "/*" ))        ||
		//
		//	Put the arguments in the argument array
		//
		!(ArgArray.Put( &ProgramNameArgument ))				||
		!(ArgArray.Put( ArchiveArgument ))					||
		!(ArgArray.Put( DateArgument ))						||
        !(ArgArray.Put( OldArgument ))                      ||
		!(ArgArray.Put( EmptyArgument ))					||
		!(ArgArray.Put( ModifyArgument ))					||
		!(ArgArray.Put( PromptArgument ))					||
		!(ArgArray.Put( SubdirArgument ))					||
		!(ArgArray.Put( VerifyArgument ))					||
		!(ArgArray.Put( WaitArgument )) 					||
        !(ArgArray.Put( HelpArgument ))                     ||
        !(ArgArray.Put( ContinueArgument ))                 ||
        !(ArgArray.Put( IntelligentArgument ))              ||
        !(ArgArray.Put( VerboseArgument ))                  ||
        !(ArgArray.Put( HiddenArgument ))                   ||
        !(ArgArray.Put( ReadOnlyArgument ))                 ||
        !(ArgArray.Put( SilentArgument ))                   ||
        !(ArgArray.Put( NoCopyArgument ))                   ||
        !(ArgArray.Put( StructureArgument ))                ||
        !(ArgArray.Put( UpdateArgument ))                   ||
        !(ArgArray.Put( CopyAttrArgument ))                 ||
        !(ArgArray.Put( UseShortArgument ))                 ||
        !(ArgArray.Put( InvalidSwitchArgument ))            ||
        !(ArgArray.Put( FirstQuotedPathArgument ))          ||
        !(ArgArray.Put( SecondQuotedPathArgument ))         ||
        !(ArgArray.Put( FirstPathArgument ))                ||
        !(ArgArray.Put( SecondPathArgument ))
		)	{


		DisplayMessageAndExit( XCOPY_ERROR_NO_MEMORY, NULL, EXIT_MISC_ERROR);
	}

	//
	//	Parse the arguments
	//
    ArgLex = ParseArguments( &CmdLine, &ArgArray );

	if ( InvalidSwitchArgument->IsValueSet() ) {

		InvalidSwitch = InvalidSwitchArgument->GetString();

		InvalidParms.Initialize( SWITCH_CHARACTERS );

		Ch = InvalidSwitch->QueryChAt(0);

		if ( Ch == 'd' || Ch == 'D' ) {
			Ch = InvalidSwitch->QueryChAt(1);
			if ( Ch == INVALID_CHAR ) {
				DisplayMessageAndExit( XCOPY_ERROR_INVALID_NUMBER_PARAMETERS,
									   NULL,
									   EXIT_MISC_ERROR );
			} else if ( Ch != ':' || InvalidSwitch->QueryChCount() == 2 ) {
				DisplayMessageAndExit( XCOPY_ERROR_INVALID_SWITCH_SWITCH,
									   InvalidSwitchArgument->GetLexeme(),
									   EXIT_MISC_ERROR );
			}
		} else if ( Ch == '/' ) {
			Ch = InvalidSwitch->QueryChAt(1);
			if ( Ch == ':' && InvalidSwitchArgument->GetString()->QueryChAt(2) == INVALID_CHAR ) {
				InvalidSwitchArgument->GetLexeme()->Truncate(1);
			}
		}

		Ch = InvalidSwitch->QueryChAt(0);

		if ( InvalidParms.Strchr( Ch ) != INVALID_CHNUM ) {
			DisplayMessageAndExit( XCOPY_ERROR_INVALID_PARAMETER,
								   InvalidSwitchArgument->GetLexeme(),
								   EXIT_MISC_ERROR );
		} else {
			DisplayMessageAndExit( XCOPY_ERROR_INVALID_SWITCH_SWITCH,
								   InvalidSwitchArgument->GetLexeme(),
								   EXIT_MISC_ERROR );
		}
	}

	//
	//	Set the switches
	//
	_EmptySwitch	=	EmptyArgument->QueryFlag();
    _ModifySwitch   =   ModifyArgument->QueryFlag();

	//
	//	ModifySwitch implies ArchiveSwitch
	//
	if ( _ModifySwitch ) {
		_ArchiveSwitch = TRUE;
	} else {
		_ArchiveSwitch	=	ArchiveArgument->QueryFlag();
    }

    //
    //  Set the switches
    //
    _PromptSwitch       =   PromptArgument->QueryFlag();
    _SubdirSwitch       =   SubdirArgument->QueryFlag();
    _VerifySwitch       =   VerifyArgument->QueryFlag();
    _WaitSwitch         =   WaitArgument->QueryFlag();
    _ContinueSwitch     =   ContinueArgument->QueryFlag();
    _IntelligentSwitch  =   IntelligentArgument->QueryFlag();
    _CopyIfOldSwitch    =   OldArgument->QueryFlag();
    _VerboseSwitch      =   VerboseArgument->QueryFlag();
    _HiddenSwitch       =   HiddenArgument->QueryFlag();
    _ReadOnlySwitch     =   ReadOnlyArgument->QueryFlag();
    _SilentSwitch       =   SilentArgument->QueryFlag();
    _DontCopySwitch     =   NoCopyArgument->QueryFlag();
    _StructureOnlySwitch=   StructureArgument->QueryFlag();
    _UpdateSwitch       =   UpdateArgument->QueryFlag();
    _CopyAttrSwitch     =   CopyAttrArgument->QueryFlag();
    _UseShortSwitch     =   UseShortArgument->QueryFlag();
    HelpSwitch          =   HelpArgument->QueryFlag();


	//
	//	Set the source and destination paths.  Argument checking is
	//	done somewhere else, so it is ok. to set the source path to
	//	NULL here.
    //
    GetSourceAndDestinationPath(FirstPathArgument,
                                FirstQuotedPathArgument,
                                SecondPathArgument,
                                SecondQuotedPathArgument,
                                ArgLex,
                                &_SourcePath,
                                &_DestinationPath);

    DELETE(ArgLex);

	//
	//	Set the date argument
	//

	if ( DateArgument->IsValueSet() ) {

		if ((_Date = NEW TIMEINFO) == NULL ) {

			DisplayMessageAndExit( XCOPY_ERROR_NO_MEMORY, NULL, EXIT_MISC_ERROR );
		}

		_Date->Initialize( DateArgument->GetTimeInfo() );

	} else {

		_Date = NULL;
    }

#ifdef GETNT

    if (!fUsage) {
        szSourcePath = _SourcePath->QueryFullPathString()->QuerySTR();
    }

#endif // GETNT
}

PARGUMENT_LEXEMIZER
XCOPY::ParseArguments(
	IN	PWSTRING	CmdLine,
	OUT PARRAY		ArgArray
	)

/*++

Routine Description:

	Parses a group of arguments

Arguments:

	CmdLine 	-	Supplies pointer to a command line to parse
	ArgArray	-	Supplies pointer to array of arguments

Return Value:

    Returns the argument lexemizer used which then needs to be freed
    by the client.

Notes:

--*/

{
    PARGUMENT_LEXEMIZER  ArgLex;

	//
	//	Initialize lexeme array and the lexemizer.
	//
    if ( !(ArgLex = NEW ARGUMENT_LEXEMIZER) ||
         !(LexArray->Initialize( 9, 9 )) ||
         !(ArgLex->Initialize( LexArray )) ) {

		DisplayMessageAndExit( XCOPY_ERROR_NO_MEMORY,
							   NULL,
							   EXIT_MISC_ERROR );

	}

	//
	//	Set our parsing preferences
	//
    ArgLex->PutMultipleSwitch( "/?AMDPSEVWCIFHRQLKTUN" );
    ArgLex->PutSwitches( "/" );
    ArgLex->SetCaseSensitive( FALSE );
    ArgLex->PutSeparators( " /\t" );
    ArgLex->PutStartQuotes( "\"" );
    ArgLex->PutEndQuotes( "\"" );

	//
	//	Parse the arguments
	//
    if ( !(ArgLex->PrepareToParse( CmdLine ))) {

		DisplayMessageAndExit( XCOPY_ERROR_PARSE,
							   NULL,
							   EXIT_MISC_ERROR );

	}

    if ( !ArgLex->DoParsing( ArgArray ) ) {

		DisplayMessageAndExit( XCOPY_ERROR_INVALID_NUMBER_PARAMETERS,
							   NULL,
							   EXIT_MISC_ERROR );
	}

    return ArgLex;
}

VOID
XCOPY::CheckArgumentConsistency (
	)

/*++

Routine Description:

	Checks the consistency of the arguments

Arguments:

	none

Return Value:

	none

Notes:

--*/

{

    PFSN_DIRECTORY      DirSrc;
    PFSN_DIRECTORY      DirDst;
    PWSTRING DevSrc;
    PWSTRING DevDst;
    PATH                PathSrc, PathSrc1;
    PATH                PathDst, PathDst1;
    DSTRING             Slash;

	if ( HelpSwitch ) {
		//
		//	Help requested
		//
		Usage();
		DisplayMessageAndExit( 0,
							   NULL,
							   EXIT_NORMAL );
	}


	//
	//	Make sure that we have a source path
	//
	if ( _SourcePath == NULL ) {

		DisplayMessageAndExit( XCOPY_ERROR_INVALID_NUMBER_PARAMETERS,
							   NULL,
							   EXIT_MISC_ERROR );
	}

	//
    //  The empty switch implies Subdir switch (note that DOS
    //  requires Subdir switch explicitly, but we are not that
    //  brain-damaged).
    //
    if ( _EmptySwitch ) {
        _SubdirSwitch = TRUE;
    }


    //
    //  The StructureOnly switch imples the subdir switch
    //
    if ( _StructureOnlySwitch ) {
        _SubdirSwitch = TRUE;
    }

    //if ( _EmptySwitch && !_SubdirSwitch )  {
    //
    //    DisplayMessageAndExit( XCOPY_ERROR_INVALID_SWITCH,
    //                           NULL,
    //                           EXIT_MISC_ERROR );
    //
    //}

	//
	//	If destination path is null, then the destination path is the
	//	current directory
	//
	if ( _DestinationPath == NULL ) {

		if ( ((_DestinationPath = NEW PATH) == NULL ) ||
             !_DestinationPath->Initialize( (LPWSTR)L".", TRUE ) ) {

			DisplayMessageAndExit( XCOPY_ERROR_NO_MEMORY, NULL, EXIT_MISC_ERROR );
		}
	}

	_DestinationPath->TruncateNameAtColon();

	if ( !PathSrc1.Initialize( _SourcePath, TRUE )      ||
         !PathDst1.Initialize( _DestinationPath, TRUE ) ||
         !(DevSrc = PathSrc1.QueryDevice())             ||
         !(DevDst = PathDst1.QueryDevice())             ||
         !PathSrc.Initialize( DevSrc )                  ||
         !PathDst.Initialize( DevDst )                  ||
         !Slash.Initialize( "\\" )                      ||
         !PathSrc.AppendBase( &Slash )                  ||
         !PathDst.AppendBase( &Slash )                  ||
         !(DirSrc = SYSTEM::QueryDirectory( &PathSrc )) ||
         !(DirDst = SYSTEM::QueryDirectory( &PathDst )) ) {
		DisplayMessageAndExit( XCOPY_ERROR_INVALID_DRIVE, NULL, EXIT_MISC_ERROR );
    }
    DELETE( DevSrc );
    DELETE( DevDst );
    DELETE( DirSrc );
    DELETE( DirDst );

}
