/*
 * clone.c - start a forked instance of the current shell on a new terminal
 *
 * This file is part of zsh, the Z shell.
 *
 * Copyright (c) 1997 Zolt�n Hidv�gi
 * All rights reserved.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and to distribute modified versions of this software for any
 * purpose, provided that the above copyright notice and the following
 * two paragraphs appear in all copies of this software.
 *
 * In no event shall Zolt�n Hidv�gi or the Zsh Development Group be liable
 * to any party for direct, indirect, special, incidental, or consequential
 * damages arising out of the use of this software and its documentation,
 * even if Zolt�n Hidv�gi and the Zsh Development Group have been advised of
 * the possibility of such damage.
 *
 * Zolt�n Hidv�gi and the Zsh Development Group specifically disclaim any
 * warranties, including, but not limited to, the implied warranties of
 * merchantability and fitness for a particular purpose.  The software
 * provided hereunder is on an "as is" basis, and Zolt�n Hidv�gi and the
 * Zsh Development Group have no obligation to provide maintenance,
 * support, updates, enhancements, or modifications.
 *
 */

/*
 * The clone builtin can be used to start a forked instance of the current
 * shell on a new terminal.  The only argument to the builtin is the name
 * of the new terminal.  In the new shell the PID, PPID and TTY parameters
 * are changed appropriately.  $! is set to zero in the new instance of the
 * shell and to the pid of the new instance in the original shell.
 *
 */

#include "clone.mdh"
#include "clone.pro"

/**/
static int
bin_clone(char *nam, char **args, UNUSED(Options ops), UNUSED(int func))
{
    int ttyfd, pid, cttyfd;

    unmetafy(*args, NULL);
    ttyfd = open(*args, O_RDWR|O_NOCTTY);
    if (ttyfd < 0) {
	zwarnnam(nam, "%s: %e", *args, errno);
	return 1;
    }
    pid = fork();
    if (!pid) {
	clearjobtab(0);
	ppid = getppid();
	mypid = getpid();
#ifdef HAVE_SETSID
	if (setsid() != mypid)
	    zwarnnam(nam, "failed to create new session: %e", NULL, errno);
#elif defined(TIOCNOTTY)
	    if (ioctl(SHTTY, TIOCNOTTY, 0))
	    zwarnnam(*args, "%e", NULL, errno);
	    setpgrp(0L, mypid);
#endif
	dup2(ttyfd,0);
	dup2(ttyfd,1);
	dup2(ttyfd,2);
	if (ttyfd > 2)
	    close(ttyfd);
	closem(0);
	close(coprocin);
	close(coprocout);
	/* Acquire a controlling terminal */
	cttyfd = open(*args, O_RDWR);
	if (cttyfd == -1)
	    zwarnnam(nam, "%e", NULL, errno);
	else {
#ifdef TIOCSCTTY
	    ioctl(cttyfd, TIOCSCTTY, 0);
#endif
	    close(cttyfd);
	}
	/* check if we acquired the tty successfully */
	cttyfd = open("/dev/tty", O_RDWR);
	if (cttyfd == -1)
	    zwarnnam(nam, "could not make %s my controlling tty, job control "
		     "disabled", *args, 0);
	else
	    close(cttyfd);

	/* Clear mygrp so that acquire_pgrp() gets the new process group.
	 * (acquire_pgrp() is called from init_io()) */
	mypgrp = 0;
	init_io();
	setsparam("TTY", ztrdup(ttystrname));
    }
    close(ttyfd);
    if (pid < 0) {
	zerrnam(nam, "fork failed: %e", NULL, errno);
	return 1;
    }
    lastpid = pid;
    return 0;
}

static struct builtin bintab[] = {
    BUILTIN("clone", 0, bin_clone, 1, 1, 0, NULL, NULL),
};

/**/
int
setup_(UNUSED(Module m))
{
    return 0;
}

/**/
int
boot_(Module m)
{
    return !addbuiltins(m->nam, bintab, sizeof(bintab)/sizeof(*bintab));
}

/**/
int
cleanup_(Module m)
{
    deletebuiltins(m->nam, bintab, sizeof(bintab)/sizeof(*bintab));
    return 0;
}

/**/
int
finish_(UNUSED(Module m))
{
    return 0;
}
