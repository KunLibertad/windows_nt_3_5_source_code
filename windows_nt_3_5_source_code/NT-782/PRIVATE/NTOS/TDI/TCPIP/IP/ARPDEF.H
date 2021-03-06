/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-1992          **/
/********************************************************************/
/* :ts=4 */

//***   arpdef.h - ARP definitions
//
//  This file containes all of the private ARP related definitions.


#define MEDIA_DIX   	0
#define MEDIA_TR    	1
#define	MEDIA_FDDI		2
#define	MEDIA_ARCNET	3

#define MAX_MEDIA   	4

#define INTERFACE_UP    0                   // Interface is up.
#define INTERFACE_INIT  1                   // Interface is initializing.
#define INTERFACE_DOWN  2                   // Interface is down.

#define LOOKAHEAD_SIZE  128                 // A reasonable lookahead size

// Definitions for state of an ATE. The 'RESOLVING' indicators must occur first.
#define ARP_RESOLVING_LOCAL     0           // Address is being resolved (on local ring, if TR)
#define ARP_RESOLVING_GLOBAL    1           // Address is being resolved globally.
#define ARP_RESOLVING ARP_RESOLVING_GLOBAL
#define ARP_GOOD            2               // ATE is good.
#define ARP_BAD             3               // ATE is bad.
#define ARP_FLOOD_RATE      1000L           // No more than once a second.
#define ARP_802_ADDR_LENGTH 6               // Length of an 802 address.

#define MIN_ETYPE           0x600           // Minimum valid Ethertype
#define SNAP_SAP            170
#define SNAP_UI             3


//* Structure of an Ethernet header.
struct  ENetHeader {
    uchar       eh_daddr[ARP_802_ADDR_LENGTH];
    uchar       eh_saddr[ARP_802_ADDR_LENGTH];
    ushort      eh_type;
}; /* ENetHeader */

typedef struct ENetHeader ENetHeader;

//* Structure of a token ring header.
struct TRHeader {
    uchar       tr_ac;
    uchar       tr_fc;
    uchar       tr_daddr[ARP_802_ADDR_LENGTH];
    uchar       tr_saddr[ARP_802_ADDR_LENGTH];
}; /* TRHeader */
#define ARP_AC      0x10
#define ARP_FC      0x40
#define TR_RII      0x80

typedef struct TRHeader TRHeader;
struct RC {
        uchar   rc_blen;                    // Broadcast indicator and length.
        uchar   rc_dlf;                     // Direction and largest frame.
}; /* RC */
#define RC_DIR      0x80
#define RC_LENMASK  0x1f
#define RC_SRBCST   0xc2                    // Single route broadcast RC.
#define	RC_BCST_LEN	0x70					// Length for a broadcast.
#define RC_LF_MASK	0x70					// Mask for length bits.

typedef struct RC RC;

//* Structure of source routing information.
struct SRInfo {
    RC      sri_rc;                         // Routing control info.
    ushort  sri_rd[1];                      // Routing designators.
}; /* SRInfo */

#define ARP_MAX_RD      8

typedef struct SRInfo SRInfo;

//* Structure of an FDDI header.
struct  FDDIHeader {
	uchar		fh_pri;
    uchar       fh_daddr[ARP_802_ADDR_LENGTH];
    uchar       fh_saddr[ARP_802_ADDR_LENGTH];
}; /* FDDIHeader */

typedef struct FDDIHeader FDDIHeader;

#define	ARP_FDDI_PRI	0x57
#define	ARP_FDDI_MSS	4352

//* Structure of an ARCNET header.
struct  ARCNetHeader {
    uchar       ah_saddr;
    uchar       ah_daddr;
    uchar		ah_prot;
}; /* ARCNetHeader */

typedef struct ARCNetHeader ARCNetHeader;

//* Structure of a SNAP header.
struct SNAPHeader {
    uchar       sh_dsap;
    uchar       sh_ssap;
    uchar       sh_ctl;
    uchar       sh_protid[3];
    ushort      sh_etype;
}; /* SNAPHeader */

typedef struct SNAPHeader SNAPHeader;

#define ARP_MAX_MEDIA_ENET  sizeof(ENetHeader)
#define ARP_MAX_MEDIA_TR    (sizeof(TRHeader)+sizeof(RC)+(ARP_MAX_RD*sizeof(ushort))+sizeof(SNAPHeader))
#define ARP_MAX_MEDIA_FDDI  (sizeof(FDDIHeader)+sizeof(SNAPHeader))
#define ARP_MAX_MEDIA_ARC	sizeof(ARCNetHeader)

#define	ENET_BCAST_MASK		0x01
#define	TR_BCAST_MASK		0x80
#define	FDDI_BCAST_MASK		0x80
#define	ARC_BCAST_MASK		0xff

#define	ENET_BCAST_VAL		0x01
#define	TR_BCAST_VAL		0x80
#define	FDDI_BCAST_VAL		0x80
#define	ARC_BCAST_VAL		0x00

#define	ENET_BCAST_OFF		0x00
#define	TR_BCAST_OFF		offsetof(struct TRHeader, tr_daddr)
#define	FDDI_BCAST_OFF		offsetof(struct FDDIHeader, fh_daddr)
#define	ARC_BCAST_OFF		offsetof(struct ARCNetHeader, ah_daddr)

//* Structure of an ARP table entry.
typedef struct ARPTableEntry {
    struct ARPTableEntry    *ate_next;      // Next ATE in hash chain
    ulong                   ate_valid;      // Last time ATE was known to be valid.
    IPAddr                  ate_dest;       // IP address represented.
    PNDIS_PACKET            ate_packet;     // Packet (if any) queued for resolution
    RouteCacheEntry         *ate_rce;       // List of RCEs that reference this ATE.
    DEFINE_LOCK_STRUCTURE(ate_lock)         // Lock for this ATE.
	uint					ate_useticks;	// Number of ticks left until this
											// goes away.
    uchar                   ate_addrlength; // Length of the address.
    uchar                   ate_state;      // State of this entry
    uchar                   ate_addr[1];    // Address that maps to dest
} ARPTableEntry;

#define ALWAYS_VALID        0xffffffff

//* Structure of the ARP table.
#define ARP_TABLE_SIZE      32
#define ARP_HASH(x)         ((((uchar *)&(x))[3]) % ARP_TABLE_SIZE)
typedef ARPTableEntry   *ARPTable[];

//* List structure for local representation of an IPAddress.
typedef struct ARPIPAddr {
    struct ARPIPAddr        *aia_next;      // Next in list.
	uint					aia_age;
    IPAddr                  aia_addr;       // The address.
	IPMask					aia_mask;
} ARPIPAddr;

#define	ARPADDR_NOT_LOCAL	4
#define	ARPADDR_NEW_LOCAL	3
#define	ARPADDR_OLD_LOCAL	0

//* List structure for Proxy-ARP addresses.
typedef struct ARPPArpAddr {
	struct ARPPArpAddr		*apa_next;		// Next in list.
	IPAddr					apa_addr;		// The address.
	IPMask					apa_mask;		// And the mask.
} ARPPArpAddr;

//* List structure for a multicast IP address.
typedef struct ARPMCastAddr {
	struct ARPMCastAddr		*ama_next;		// Next in list.
	IPAddr					ama_addr;		// The (masked) address.
	uint					ama_refcnt;		// Reference count for this address.
} ARPMCastAddr;

#define	ARP_MCAST_MASK		0xffff7f00

#define ARP_TIMER_TIME      1000L
#define ARP_RESOLVE_TIMEOUT 1000L
#define ARP_VALID_TIMEOUT   600000L
#define	ATE_TICK_VALUE		((2L*60L*1000L)/ARP_TIMER_TIME)

#ifdef VXD
#define	ARP_DEFAULT_MAXHDRS	100
#else
#define	ARP_DEFAULT_MAXHDRS	0xffffffff
#endif

typedef struct ARPBufferTracker {
	struct	ARPBufferTracker	*abt_next;
	NDIS_HANDLE					abt_handle;
	uchar						*abt_buffer;
} ARPBufferTracker;

//* Structure of information we keep on a per-interface basis.
typedef struct ARPInterface {
    void                    *ai_context;    // Upper layer context info.
    NDIS_HANDLE             ai_handle;      // NDIS bind handle.
    NDIS_MEDIUM             ai_media;       // Media type.
    NDIS_HANDLE             ai_bpool;       // Handle for buffer pool.
    NDIS_HANDLE             ai_ppool;       // Handle for packet pool.
    DEFINE_LOCK_STRUCTURE(ai_lock)          // Lock for this structure.
    DEFINE_LOCK_STRUCTURE(ai_ARPTblLock)    // ARP Table lock for this structure.
    PNDIS_BUFFER			ai_sblist;		// Free list of header buffers.
    uchar                   *ai_bblist;     // Free list of 'big' buffers.
    ARPTable                *ai_ARPTbl;     // Pointer to the ARP table for this interface
    ARPIPAddr               ai_ipaddr;      // Local IP address list.
	ARPPArpAddr				*ai_parpaddr;	// Proxy ARP address list.
	IPAddr					ai_bcast;		// Broadcast mask for this interface.
	// SNMP required counters
	uint					ai_inoctets;	// Input octets.
	uint					ai_inpcount[2];	// Count of nonunicast and unicast
											// packets received.
	uint					ai_outoctets;	// Output octets
	uint					ai_outpcount[2];// Count of nonunicast and unicast
											// packets sent.
	uint					ai_qlen;		// Output q length.
    uchar                   ai_addr[ARP_802_ADDR_LENGTH]; // Local HW address.
    uchar                   ai_sbsize;      // Size of a small buffer
    uchar                   ai_state;       // State of the interface. Union of
											// admin and operational states.
	uchar					ai_addrlen;		// Length of ai_addr.
	uchar					ai_bcastmask;	// Mask for checking unicast.
	uchar					ai_bcastval;	// Value to check against.
	uchar					ai_bcastoff;	// Offset in frame to check against.
	uchar					ai_hdrsize;		// Size of 'typical' header.
	uchar					ai_snapsize;	// Size of snap header, if any.
	uchar					ai_pad[2];		// PAD PAD
	uint					ai_pfilter;		// Packet filter for this i/f.
	uint					ai_count;		// Number of entries in the ARPTable.
	uint					ai_parpcount;	// Number of proxy ARP entries.
    CTETimer                ai_timer;       // ARP timer for this interface.
    CTEBlockStruc           ai_block;       // Structure for blocking on.
	ushort					ai_mtu;			// MTU for this interface.
	uchar					ai_adminstate;	// Admin state.
	uchar					ai_operstate;	// Operational state;
	uint					ai_speed;		// Speed.
	uint					ai_lastchange;	// Last change time.
	uint					ai_indiscards;	// In discards.
	uint					ai_inerrors;	// Input errors.
	uint					ai_uknprotos;	// Unknown protocols received.
	uint					ai_outdiscards;	// Output packets discarded.
	uint					ai_outerrors;	// Output errors.
	uint					ai_desclen;		// Length of desc. string.
	uint					ai_index;		// Global I/F index ID.
	uint					ai_atinst;		// AT instance number.
	uint					ai_ifinst;		// IF instance number.
	char					*ai_desc;		// Descriptor string.
	ARPMCastAddr			*ai_mcast;		// Multicast list.
	uint					ai_mcastcnt;	// Count of elements on mcast list.
	void					*ai_bbbase;		// Base of big buffers.
	uint					ai_curhdrs;		// Current number of headers.
	uint					ai_maxhdrs;		// Maximum allowed number of headers.
	ARPBufferTracker		*ai_buflist;	// List of header buffer handles.
} ARPInterface;

typedef struct ARPNotifyStruct {
	CTEEvent				ans_event;
	uint					ans_shutoff;
	IPAddr					ans_addr;
	uint					ans_hwaddrlen;
	uchar					ans_hwaddr[1];
} ARPNotifyStruct;
	
//* NOTE: These two values MUST stay at 0 and 1.
#define	AI_UCAST_INDEX		0
#define	AI_NONUCAST_INDEX	1

#define ARP_DEFAULT_PACKETS 10              // Default to this many packets.
#define ARP_DEFAULT_BUFFERS 50              // And this many buffers.

#define	ARP_HDRBUF_GROW_SIZE	32			// This many header buffers.

//* Structure of information passed as context in RCE.
typedef struct ARPContext {
    RouteCacheEntry         *ac_next;       // Next RCE in ARP table chain.
    ARPTableEntry           *ac_ate;        // Back pointer to ARP table entry.
} ARPContext;

typedef struct IPNMEContext {
	uint				inc_index;
	ARPTableEntry		*inc_entry;
} IPNMEContext;

#ifdef NT
//
// This structure must be packed under NT.
//
#include <packon.h>
#endif // NT

// Structure of an ARP header.
struct ARPHeader {
    ushort      ah_hw;                      // Hardware address space.
    ushort      ah_pro;                     // Protocol address space.
    uchar       ah_hlen;                    // Hardware address length.
    uchar       ah_plen;                    // Protocol address length.
    ushort      ah_opcode;                  // Opcode.
    uchar       ah_shaddr[ARP_802_ADDR_LENGTH]; // Source HW address.
    IPAddr      ah_spaddr;                  // Source protocol address.
    uchar       ah_dhaddr[ARP_802_ADDR_LENGTH]; // Destination HW address.
    IPAddr      ah_dpaddr;                  // Destination protocol address.
}; /* ARPHeader */

#ifdef NT
#include <packoff.h>
#endif // NT

typedef struct ARPHeader ARPHeader;

#define ARP_ETYPE_IP    0x800
#define ARP_ETYPE_ARP   0x806
#define ARP_REQUEST     1
#define ARP_RESPONSE    2
#define ARP_HW_ENET     1
#define ARP_HW_802      6
#define	ARP_HW_ARCNET	7

#define	ARP_ARCPROT_ARP	0xd5
#define	ARP_ARCPROT_IP	0xd4

// The size we need to back off the buffer length because ARCNet address
// are one bytes instead of six.
#define	ARCNET_ARPHEADER_ADJUSTMENT		10
