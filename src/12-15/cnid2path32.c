/* cnid2path32.c (formerly just "cnid2path.c")
 * Mac OS X Internals, Chapter 12, Section 7.2.1, Figure 12-15
 * Pages 1516-1517
 * Compilation instructions page 1517
 * Compile with:
 *  gcc -Wall -m32 -o cnid2path32 cnid2path32.c -framework Carbon -framework CoreServices
 */

#include <stdio.h>
#include <sys/param.h>
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>
#include <hfs/hfs_format.h>

#ifdef __LP64__
# error "This file fails to work on 64-bit systems."
#endif /* __LP64__ */

/* this is returned by PBResolveFileIDRefSync(): */
typedef struct {
    unsigned char length;
    unsigned char characters[255];
} HFSStr255;

int
main(int argc, char **argv)
{
    FIDParam   pb;
    OSStatus   result;
    long       tmpSrcDirID;
    int        len = (MAXPATHLEN - 1);
    char       path[MAXPATHLEN] = { '\0' };
    char      *cursor = (char *)(path + (MAXPATHLEN - 1));
    char      *upath;
    HFSStr255 *p, pbuf;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <CNID>\n", argv[0]);
        exit(1);
    }

    tmpSrcDirID = atoi(argv[1]);

    pb.ioVRefNum = 0; /* no volume reference number -- use default
					   * parent directory ID -- we do NOT know it yet */
    pb.ioSrcDirID = -1;

    while (1) {
        pb.ioNamePtr = (StringPtr)&pbuf; /* a pointer to a pathname */
        pb.ioFileID = tmpSrcDirID;       /* the given CNID */
		/* PBResolveFileIDRefSync() is deprecated as of 10.5, the Apple
		 * Developer docs say to use FSGetCatalogInfo() instead (I am trying
		 * that in the 64-bit version): */
        if ((result = PBResolveFileIDRefSync((HParmBlkPtr)&pb)) < 0) {
			fprintf(stderr, "cnid2path32: PBResolveFileIDRefSync() returned error '%i'\n",
					(int)result);
			/* a negative return code is an error, so return that: */
            return result;
		}

        if ((pb.ioSrcDirID == tmpSrcDirID) || (len <= 0)) {
            cursor++;
            break;
        }

        p = (HFSStr255 *)&pbuf;
        cursor -= (p->length);
        memcpy(cursor, p->characters, p->length);
        *--cursor = '/';
        len -= (1 + p->length);

        tmpSrcDirID = pb.ioSrcDirID;
    }

    if ((upath = strchr(cursor, '/')) != NULL) {
        *upath = '\0';
        upath++;
    } else {
        upath = "";
	}

    printf("%s:/%s\n", cursor, upath);

    return 0;
}

/* EOF */
