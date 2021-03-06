#define IPXSID_INDEX(id) (CSHORT)( (id) & 0x0FFF )
#define IPXSID_SEQUENCE(id) (CSHORT)( (id) >> 12 )
#define MAKE_IPXSID(index, sequence) (CSHORT)( ((sequence) << 12) | (index) )
#define INCREMENT_IPXSID_SEQUENCE(id) (id) = (CSHORT)(( (id) + 1 ) & 0xF);

//
// Name claim routine
//

NTSTATUS
SrvIpxClaimServerName (
    IN PENDPOINT Endpoint,
    IN PVOID NetbiosName
    );

//
// Transport Receive Datagram indication handlers
//

NTSTATUS
SrvIpxServerDatagramHandler (
    IN PVOID TdiEventContext,
    IN int SourceAddressLength,
    IN PVOID SourceAddress,
    IN int OptionsLength,
    IN PVOID Options,
    IN ULONG ReceiveDatagramFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG *BytesTaken,
    IN PVOID Tsdu,
    OUT PIRP *IoRequestPacket
    );

NTSTATUS
SrvIpxNameDatagramHandler (
    IN PVOID TdiEventContext,
    IN int SourceAddressLength,
    IN PVOID SourceAddress,
    IN int OptionsLength,
    IN PVOID Options,
    IN ULONG ReceiveDatagramFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG *BytesTaken,
    IN PVOID Tsdu,
    OUT PIRP *IoRequestPacket
    );

//
// Datagram send routine
//

VOID
SrvIpxStartSend (
    IN OUT PWORK_CONTEXT WorkContext,
    IN PIO_COMPLETION_ROUTINE SendCompletionRoutine
    );

