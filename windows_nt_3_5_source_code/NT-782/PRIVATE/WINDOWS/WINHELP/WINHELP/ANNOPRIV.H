/***************************************************************************\
*
*  ANNOPRIV.H
*
*  Copyright (C) Microsoft Corporation 1991.
*  All Rights reserved.
*
*****************************************************************************
*
*  Module Intent
*
*  The module intent goes here.
*
*****************************************************************************
*
*  Testing Notes
*
*****************************************************************************
*
*  Created 00-Ooo-0000 by KevynCT
*
*****************************************************************************
*
*  Released by Development:  00-Ooo-0000
*
*****************************************************************************
*
*  Current Owner: Dann
*
\***************************************************************************/


/*  The Magic Number identifies the version of annotation format  */
#define ldAnnoMagicHelp3_0    0x666d6201
#define ldAnnoMagicCur        0x666d6208

/* These macros convert the given MLA to/from Help 3.0 format if necessary */
#define ConvertQMLA(qmla, wVer)  (((wVer) == wVersion3_0) ? \
 ((qmla)->va.dword = VAToOffset30(&(qmla)->va)) : 0)

#define ConvertOldQMLA(qmla, wVer) { if ((wVer) == wVersion3_0) \
 OffsetToVA30(&(qmla)->va, (qmla)->va.dword) }

typedef struct annoversion {
  LONG  ldReserved;           /*  Contains magic number  */
  WORD  wHelpFileVersion;
} ANNOVERSION, FAR *QANNOVERSION;


#define wMaxNumAnnotations     32767

/* Memory Annotation Position Struct */
typedef GH    HAPS;

/* The following structure defines the disk record stored in the
 * annotation LINK file.  In Help 3.0, this was defined as a TO
 * followed by a HASH.  The to.ich field was always zero.
 * The HASH field was always zero.
 *
 * The Help 3.5 annotation file format has changed so that a TO has
 * been replaced by a VA and an OBJRG: i.e. a logical address.  Help 3.0
 * will not be able to read Help 3.6+ annotation files, and so the Magic
 * Number has been changed in the Annotation version file.  Help 3.6+ will
 * create/update Help 3.0-format annotation files for Help 3.0 files;
 * otherwise it will use Help 3.6+ annotation files.
 *
 */

typedef struct link
  {
  MLA   mla;
  LONG  lReserved;
  } LINK, FAR *QLINK;

typedef struct aps
  {
  WORD wNumRecs;
  LINK link[1];
  } APS, FAR *QAPS;

/*
 *  Annotation "Document" : the file system which contains
 *  the version file, link file, and zero or more annotation text files.
 */
typedef struct ads
  {
  FM    fmFS;      /*  The name of the fs  */
  WORD  wVersion;
  HAPS    haps;      /*  Handle to sorted list of links  */
  } ADS, FAR *QADS;


extern SZ   PASCAL SzFromQMLA( QMLA qmla, SZ sz );
extern HAPS PASCAL HapsInitHf( HF hf );
extern void PASCAL DestroyHaps( HAPS haps );
extern HAPS PASCAL HapsReadHf( HF hf );
extern BOOL PASCAL FFlushHfHaps( HF hf, HAPS haps );
extern BOOL PASCAL FInsertLinkHaps( HAPS haps, QMLA qmla, HAPS FAR *qhapsNew );
extern BOOL PASCAL FDeleteLinkHaps( HAPS haps, QMLA qmla, HAPS FAR *qhapsNew );
extern BOOL PASCAL FLookupHaps( HAPS haps, QMLA qmla, QMLA qmlaPrev,
 QMLA qmlaNext, QI qi);
extern BOOL PASCAL FReadTextHf( HF hf, QV qv, INT cMax, WORD FAR *qwActual);
extern BOOL PASCAL FWriteTextHf( HF hf, QV qv, INT cLen);
