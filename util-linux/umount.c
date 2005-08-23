/* vi: set sw=4 ts=4: */
/*
 * Mini umount implementation for busybox
 *
 * Copyright (C) 1999-2004 by Erik Andersen <andersen@codepoet.org>
 * Copyright (C) 2005 by Rob Landley <rob@landley.net>
 * 
 * This program is licensed under the GNU General Public license (GPL)
 * version 2 or later, see http://www.fsf.org/licensing/licenses/gpl.html
 * or the file "LICENSE" in the busybox source tarball for the full text.
 *
 */

#include <limits.h>
#include <stdio.h>
#include <mntent.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mount.h>
#include "busybox.h"

extern int umount_main(int argc, char **argv)
{
	int doForce = 0;
	int freeLoop = ENABLE_FEATURE_MOUNT_LOOP;
	int useMtab = ENABLE_FEATURE_MTAB_SUPPORT;
	int umountAll = FALSE;
	int doRemount = FALSE;
	char path[2*PATH_MAX];
	struct mntent me;
	FILE *fp;
	int status=EXIT_SUCCESS;
	struct mtab_list {
		char *dir;
		char *device;
	   	struct mtab_list *next;
	} *mtl, *m;

	if(argc < 2) bb_show_usage();
	
	/* Parse any options */
	while (--argc > 0 && **(++argv) == '-') {
		while (*++(*argv)) {
			if(**argv=='a') umountAll = TRUE;
			else if(ENABLE_FEATURE_MOUNT_LOOP && **argv=='D') freeLoop = FALSE;
			else if(ENABLE_FEATURE_MTAB_SUPPORT && **argv=='n') useMtab = FALSE;
			else if(**argv=='f') doForce = 1;   // MNT_FORCE
			else if(**argv=='l') doForce = 2;   // MNT_DETACH
			else if(**argv=='r') doRemount = TRUE;
			else if(**argv=='v');
			else bb_show_usage();
		}
	}

	/* Get a list of mount points from mtab.  We read them all in now mostly
	 * for umount -a (so we don't have to worry about the list changing while
	 * we iterate over it, or about getting stuck in a loop on the same failing
	 * entry.  Notice that this also naturally reverses the list so that -a
	 * umounts the most recent entries first. */
	
	m=mtl=0;
	if(!(fp = setmntent(bb_path_mtab_file, "r")))
		bb_error_msg_and_die("Cannot open %s", bb_path_mtab_file);
	while (getmntent_r(fp,&me,path,sizeof(path))) {
		m=xmalloc(sizeof(struct mtab_list));
		m->next=mtl;
		m->device=bb_xstrdup(me.mnt_fsname);
		m->dir=bb_xstrdup(me.mnt_dir);
		mtl=m;
	}
	endmntent(fp);

	/* If we're umounting all, then m points to the start of the list and
	 * the argument list should be empty (which will match all). */
	if(!umountAll) m=0;

	// Loop through everything we're supposed to umount, and do so.
	for(;;) {
		int curstat;
		
		// Do we alrady know what to umount this time through the loop?
		if(m) safe_strncpy(path,m->dir,PATH_MAX);
		// For umountAll, end of mtab means time to exit.
		else if(umountAll) break;
		// Get next command line argument (and look it up in mtab list)
		else if(!argc--) break;
		else {
			// Get next command line argument (and look it up in mtab list)
			realpath(*argv++, path);
			for(m = mtl; m; m = m->next)
				if(!strcmp(path, m->dir) || !strcmp(path, m->device))
					break;
		}

		// Let's ask the thing nicely to unmount.
		curstat = umount(path);

		// Force the unmount, if necessary.
		if(curstat && doForce) {
			curstat = umount2(path, doForce);
			if(curstat)
				bb_error_msg_and_die("forced umount of %s failed!", path);
		}

		// If still can't umount, maybe remount read-only?	
		if (curstat && doRemount && errno == EBUSY && m) {
			curstat = mount(m->device, path, NULL, MS_REMOUNT|MS_RDONLY, NULL);
			bb_error_msg(curstat ? "Cannot remount %s read-only" :
						 "%s busy - remounted read-only", m->device);
		}

		/* De-allcate the loop device.  This ioctl should be ignored on any
		 * non-loop block devices. */
		if(ENABLE_FEATURE_MOUNT_LOOP && freeLoop && m)
			del_loop(m->device);

		if(curstat) {
			/* Yes, the ENABLE is redundant here, but the optimizer for ARM
			 * can't do simple constant propogation in local variables... */
			if(ENABLE_FEATURE_MTAB_SUPPORT && useMtab && m) erase_mtab(m->dir);
			status = EXIT_FAILURE;
			bb_perror_msg("Couldn't umount %s\n", path);
		}
		// Find next matching mtab entry for -a or umount /dev
		while(m && (m = m->next)) 
			if(umountAll || !strcmp(path,m->device))
			   	break;
	}

	// Free mtab list if necessary
	
	if(ENABLE_FEATURE_CLEAN_UP) {
		while(mtl) {
			m=mtl->next;
			free(mtl->device);
			free(mtl->dir);
			free(mtl);
			mtl=m;
		}
	}

	return status;
}
