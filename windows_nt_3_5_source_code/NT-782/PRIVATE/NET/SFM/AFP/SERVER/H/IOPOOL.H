/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	iopool.h

Abstract:

	This module contains the IO pool management stuff.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	25 Feb 1994		Initial Version

Notes:	Tab stop: 4
--*/

#ifndef _IOPOOL_

#define	_IOPOOL_

#define	DWORDSIZEBLOCK(Size)		(((Size) + sizeof(DWORD) - 1) & ~(sizeof(DWORD)-1))
#define	PAGED_BLOCK_SIGNATURE		*(PDWORD)"PAGD"
#define	NONPAGED_BLOCK_SIGNATURE	*(PDWORD)"NPGD"
#define	MAXIMUM_ALLOC_SIZE			0x20000			// For sanity checking

typedef	struct
{
	unsigned	tg_Size:20;
	unsigned	tg_Flags:4;
	unsigned	tg_Tag:8;
} TAG, *PTAG;

#define	MAX_POOL_AGE		6
#define	POOL_AGE_TIME		15
#define	POOL_ALLOC_SIZE		2*PAGE_SIZE - POOL_OVERHEAD
#define	NUM_BUFS_IN_POOL	3
#define	POOL_ALLOC_3		ASP_QUANTUM
#define	POOL_ALLOC_2		1500
#define	POOL_ALLOC_1		500
#define	LOCKS_BUF_SPACE		(POOL_ALLOC_SIZE - sizeof(IOPOOL) -				\
							 POOL_ALLOC_1 - POOL_ALLOC_2 - POOL_ALLOC_3	-	\
							 (NUM_BUFS_IN_POOL * sizeof(IOPOOL_HDR)))
#define	NUM_LOCKS_IN_POOL	LOCKS_BUF_SPACE/(sizeof(IOPOOL_HDR) + sizeof(FORKLOCK))

// The pool is structured as a set of 1 each of POOL_ALLOC_x buffers linked in
// ascending order of sizes. The balance of the POOL_ALLOC_SIZE is divided into
// a number of fork-lock structures. The layout is as follows:
//
//			+---------------------+
//			|  IoPool Structure   |----------+
//			|                     |--+       |
//			+---------------------+  |       |
//		 +--|    IoPool Hdr       |<-+       |
//		 |  +---------------------+          |
//		 |  |                     |          |
//		 |  .      Buffer 1       .          |
//		 |  |                     |          |
//		 |  +---------------------+          |
//		 +->|    IoPool Hdr       |--+       |
//		    +---------------------+  |       |
//		    |                     |  |       |
//		    .      Buffer 2       .  |       |
//		    |                     |  |       |
//		    +---------------------+  |       |
//	   |||--|    IoPool Hdr       |<-+       |
//			+---------------------+          |
//			|                     |          |
//			.      Buffer 3       .          |
//			|                     |          |
//			+---------------------+          |
//		 +--|    IoPool Hdr       |<---------+
//		 |  +---------------------+
//		 |  |                     |
//		 .  .      ForkLock1      .
//		 .  |                     |
//		 |  +---------------------+
//		 +->|    IoPool Hdr       |--|||
//		    +---------------------+
//		    |                     |
//		    .      ForkLockN      .
//		    |                     |
//		    +---------------------+
//
//
#if DBG
#define	POOLHDR_SIGNATURE			*(PDWORD)"PLH"
#define	VALID_PLH(pPoolHdr)			(((pPoolHdr) != NULL) && \
									 ((pPoolHdr)->Signature == POOLHDR_SIGNATURE))
#else
#define	VALID_PLH(pPoolHdr)			((pPoolHdr) != NULL)
#endif

typedef	struct _IoPoolHdr
{
#if	DBG
	DWORD				Signature;
#endif
	union
	{
		struct _IoPoolHdr *	iph_Next;    // Valid when it is on the free list
		struct _IoPool	  *	iph_pPool;   // Valid when it is allocated. Used
										 // to put it back on the free list
	};
	TAG						iph_Tag;	 // Keep it at end since it is accessed
										 // by moving back from the free ptr
} IOPOOL_HDR, *PIOPOOL_HDR;

#if DBG
#define	IOPOOL_SIGNATURE			*(PDWORD)"IOP"
#define	VALID_IOP(pPool)			(((pPool) != NULL) && \
									 ((pPool)->Signature == IOPOOL_SIGNATURE))
#else
#define	VALID_IOP(pPool)			((pPool) != NULL)
#endif

typedef	struct _IoPool
{
#if	DBG
	DWORD				Signature;
#endif
	struct _IoPool *	iop_Next;
	struct _IoPool **	iop_Prev;
	struct _IoPoolHdr *	iop_FreeBufHead;	// The list of POOL headers start here
	struct _IoPoolHdr *	iop_FreeLockHead;	// The list of fork-locks start here
	USHORT				iop_Age;			// Used to age out pool
	BYTE				iop_NumFreeBufs;	// Number of free IO buffer
	BYTE				iop_NumFreeLocks;	// Number of free fork locks
} IOPOOL, *PIOPOOL;

LOCAL	PIOPOOL		afpIoPoolHead = { NULL };
LOCAL	KSPIN_LOCK	afpIoPoolLock = { 0 };
LOCAL	USHORT		afpPoolAllocSizes[NUM_BUFS_IN_POOL] =
					{
						POOL_ALLOC_1,
						POOL_ALLOC_2,
						POOL_ALLOC_3
					};

LOCAL AFPSTATUS
afpIoPoolAge(
	IN	PVOID	pContext
);

#endif	// _IOPOOL_

