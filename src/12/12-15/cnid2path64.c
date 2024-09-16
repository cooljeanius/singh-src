/* cnid2path64.c
 * Mac OS X Internals, Chapter 12, Section 7.2.1, Figure 12-15
 * Pages 1516-1517
 * Compilation instructions page 1517
 * Compile with:
 *  gcc -Wall -m64 -o cnid2path64 cnid2path64.c -framework Carbon -framework CoreServices
 */

#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>
#include <hfs/hfs_format.h>

#ifndef __LP64__
# error "This file is supposed to be for 64-bit systems (still might not work even then though)."
#else /* is actually __LP64__: */
/* not normally available to 64-bit applications: */
struct FIDParam {
	QElemPtr qLink; /* A pointer to the next entry in the file I/O queue. */
	short qType; /* The queue type. */
	short ioTrap; /* The trap number of the function that was called. */
	Ptr ioCmdAddr; /* The address of the function that was called. */
	IOCompletionUPP ioCompletion; /* A universal procedure pointer to a completion routine to be executed at the end of an asynchronous call. */
	volatile OSErr ioResult; /*The result code of the function. */
	StringPtr ioNamePtr; /* A pointer to a pathname. */
	short ioVRefNum; /* A volume reference number, 0 for the default volume, or a drive number. */
	long filler14; /* Reserved. */
	StringPtr ioDestNamePtr; /* A pointer to the name of the destination file. */
	long filler15; /* Reserved. */
	long ioDestDirID; /* The parent directory ID of the destination file. */
	long filler16; /* Reserved. */
	long filler17; /* Reserved. */
	long ioSrcDirID; /* The parent directory ID of the source file. */
	short filler18; /* Reserved. */
	long ioFileID; /* The file ID. */
};
typedef struct FIDParam FIDParam;
typedef FIDParam * FIDParamPtr;
#endif /* !__LP64__ */

/* this is returned by PBResolveFileIDRefSync(): */
typedef struct {
    unsigned char length;
    unsigned char characters[255];
} HFSStr255;

#if 1
/* some functions taken from MoreFilesX: */

/******************************************************************************/

#pragma mark FSMakeFSRef

OSErr
FSMakeFSRef(FSVolumeRefNum volRefNum, SInt32 dirID, ConstStr255Param name,
			FSRef *ref);

/*
 * The FSMakeFSRef function creates an FSRef from the traditional
 * volume reference number, directory ID and pathname inputs. It is
 * functionally equivalent to FSMakeFSSpec followed by FSpMakeFSRef.
 *
 * volRefNum		--> Volume specification.
 * dirID			--> Directory specification.
 * name				--> The file or directory name, or NULL.
 * ref				<-- The FSRef.
 */

/*****************************************************************************/

OSErr
FSMakeFSRef(FSVolumeRefNum volRefNum, SInt32 dirID, ConstStr255Param name,
			FSRef *ref)
{
	OSErr		result;
	FSRefParam	pb;

	/* check parameters: */
	require_action((NULL != ref), BadParameter, (result = paramErr));

	pb.ioVRefNum = volRefNum;
	pb.ioDirID = dirID;
	pb.ioNamePtr = (StringPtr)name;
	pb.newRef = ref;
	result = PBMakeFSRefUnicodeSync(&pb);
	require_noerr(result, PBMakeFSRefUnicodeSync);

PBMakeFSRefUnicodeSync:
BadParameter:

	return (result);
}

/*****************************************************************************/

#pragma mark FSResolveFileIDRef

OSErr
FSResolveFileIDRef(FSVolumeRefNum volRefNum, SInt32 fileID, FSRef *ref);

/*
 * The FSResolveFileIDRef function returns an FSRef for the file with the
 * specified file ID reference.
 *
 * volRefNum			--> Volume specification.
 * fileID				--> The file ID reference.
 * ref					<-- The FSRef for the file ID reference.
 *
 * __________
 *
 * Also see: FSCreateFileIDRef, FSDeleteFileIDRef
 */

OSErr
FSResolveFileIDRef(FSVolumeRefNum volRefNum, SInt32 fileID, FSRef *ref)
{
	OSErr		result;
	FIDParam	pb;
	Str255		tempStr;

	/* check parameters: */
	require_action((NULL != ref), BadParameter, (result = paramErr));

	/* resolve the file ID reference: */
	tempStr[0] = 0;
	pb.ioNamePtr = tempStr;
	pb.ioVRefNum = volRefNum;
	pb.ioFileID = fileID;
	pb.ioSrcDirID = fileID; /* not sure if correct... */
	/* had to remove call to PBResolveFileIDRefSync() here */

	/* and then make an FSRef to the file: */
	result = FSMakeFSRef(volRefNum, pb.ioSrcDirID, tempStr, ref);
	require_noerr(result, FSMakeFSRef);

FSMakeFSRef:
BadParameter:

	return (result);
}
#endif /* 0 */

int
main(int argc, char **argv)
{
    FIDParam   pb;
	FSRefParam pb_ref;
    OSStatus   result; /* OSStatus is really 'SInt32' */
	OSErr      err_result; /* OSErr is really 'SInt16' */
    long       tmpSrcDirID;
    int        len = (MAXPATHLEN - 1);
	int        uni_len = (MAXPATHLEN - 1);
    char       path[MAXPATHLEN] = { '\0' };
    char      *cursor = (char *)(path + (MAXPATHLEN - 1));
	char      *uni_cursor = (char *)(path + (MAXPATHLEN - 1));
    char      *upath;
    HFSStr255 *p, pbuf;
	HFSUniStr255 *uni_p, uni_pbuf;
	FSRef *my_ref, *my_parent_ref;
	FSCatalogInfo *p_catinfo;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <CNID>\n", argv[0]);
        exit(1);
    }

	/* argv[1] should be the CNID, so make sure that is a number: */
    tmpSrcDirID = atoi(argv[1]);

	printf("tmpSrcDirID is '%li' \n", tmpSrcDirID);

    pb.ioVRefNum = 0; /* no volume reference number -- use default
					   * parent directory ID -- we do NOT know it yet */
	pb_ref.ioVRefNum = pb.ioVRefNum;
    pb.ioSrcDirID = -1;
	pb_ref.ioDirID = pb.ioSrcDirID;

    while (1) {
        pb.ioNamePtr = (StringPtr)&pbuf; /* a pointer to a pathname */
		pb_ref.ioNamePtr = (ConstStringPtr)pb.ioNamePtr; /* or maybe use 'uni_pbuf' here? */
        pb.ioFileID = tmpSrcDirID;       /* the given CNID */
		err_result = FSResolveFileIDRef(pb_ref.ioVRefNum, tmpSrcDirID, my_ref);
		/* trying to replace PBResolveFileIDRefSync() with FSGetCatalogInfo()
		 * here (the Apple Developer docs say that even that is deprecated on
		 * 10.8 and later, but I am still on 10.6, so this should be okay): */
        if ((result = FSGetCatalogInfo((const FSRef *)&my_ref,
									   (kFSCatInfoNone | kFSCatInfoVolume | kFSCatInfoParentDirID | kFSCatInfoNodeID),
									   p_catinfo, uni_p, NULL,
									   my_parent_ref)) < 0) {
			fprintf(stderr, "cnid2path64: FSGetCatalogInfo() returned error '%i'\n",
					(int)result);
			/* a negative return code is an error, so return that: */
            return result;
		}

        if ((pb.ioSrcDirID == tmpSrcDirID) || (len <= 0)) {
            cursor++;
            break; /* out of while-loop */
        }

        p = (HFSStr255 *)&pbuf;
		uni_p = (HFSUniStr255 *)&uni_pbuf;
        cursor -= (p->length);
		/* For some reason the Unicode version does not come initialized like
		 * the normal version does, so initialize the Unicode version manually.
		 * 255 is the max, so just assume that for the value: */
		uni_p->length = 255;
		uni_cursor -= (uni_p->length);
        memcpy(cursor, p->characters, p->length);
		memcpy(uni_cursor, uni_p->unicode, uni_p->length);
        *--cursor = '/';
		*--uni_cursor = '/';
        len -= (1 + p->length);
		uni_len -= (1 + uni_p->length);

        tmpSrcDirID = pb.ioSrcDirID;
    }

    if ((upath = strchr(cursor, '/')) != NULL) {
        *upath = '\0';
        upath++;
    } else {
        upath = "";
	}

	if ((cursor != NULL) && (strncmp(upath, "", (size_t)len) > 0)) {
		printf("%s:/%s\n", cursor, upath);
	} else {
		if (cursor == NULL) {
			printf("unable to print parent directory\n");
		} else {
			printf("%s:\n", cursor);
		}
		if (!strncmp(upath, "", (size_t)len)) {
			printf("unable to print filename\n");
		} else {
			printf("/%s\n", upath);
		}
	}

    return 0;
}

/* EOF */
