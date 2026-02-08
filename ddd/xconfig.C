// $Id$ -*- C++ -*-
// Setup X defaults and determine common problem causes.

// Copyright (C) 1995 Technische Universitaet Braunschweig, Germany.
// Copyright (C) 2001, 2003 Free Software Foundation, Inc.
// Written by Andreas Zeller <zeller@gnu.org>.
// 
// This file is part of DDD.
// 
// DDD is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// 
// DDD is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public
// License along with DDD -- see the file COPYING.
// If not, see <http://www.gnu.org/licenses/>.
// 
// DDD is the data display debugger.
// For details, see the DDD World-Wide-Web page, 
// `http://www.gnu.org/software/ddd/',
// or send a mail to the DDD developers <ddd@gnu.org>.

char xconfig_rcsid[] = 
    "$Id$";

#include "xconfig.h"
#include "base/strclass.h"
#include "base/cook.h"
#include "filetype.h"
#include "shell.h"
#include "assert.h"
#include "base/casts.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <limits.h>
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#include <Xm/VirtKeys.h>
#include <Xm/SelectioB.h>

#if !HAVE_POPEN_DECL
extern "C" FILE *popen(const char *command, const char *mode);
#endif
#if !HAVE_PCLOSE_DECL
extern "C" int pclose(FILE *stream);
#endif
#if !HAVE_PUTENV_DECL
extern "C" int putenv(char *string);
#endif

//-----------------------------------------------------------------------------
// Determine X project root
//-----------------------------------------------------------------------------

static bool is_file(const string& file)
{
    FILE *fp = fopen(file.chars(), "r");
    if (fp == 0)
	return false;

    fclose(fp);
    return true;
}

// Fetch the X library directory, using xmkmf(1)
static const _XtString xlibdir(Display *display, bool verbose = false)
{
    static bool tried = false;
    static const _XtString dir = 0;

    if (tried)
	return dir;
    tried = true;

    if (!is_cmd_file(cmd_file("xmkmf")))
	return dir;		// No `xmkmf' in PATH
    if (!is_cmd_file(cmd_file("make")))
	return dir;		// No `make' in PATH

    static const char *shell_command =
	""
#include "xlibdir.C"
	"";

    String me, my_class;
    XtGetApplicationNameAndClass(display, &me, &my_class);

    if (verbose)
    {
	std::cout << "Checking for X11 library directory... ";
	std::cout.flush();
    }

    const string s1 = "/bin/sh -c " + sh_quote(shell_command); 
    FILE *fp = popen(s1.chars(), "r");
    if (fp == 0)
    {
	if (verbose)
	{
	    std::cout << strerror(errno) << "\n";
	    std::cout.flush();
	}
	return dir;
    }

    char buffer[PATH_MAX];
    buffer[0] = '\0';
    (void)! fgets(buffer, sizeof(buffer), fp);
    pclose(fp);

    int len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n')
	buffer[len - 1] = '\0';

    if (buffer[0] == '/')	// Sanity check
	dir = (String)XtNewString(buffer);

    if (verbose)
    {
	if (dir)
	    std::cout << dir << "\n";
	else
	    std::cout << "(not found)\n";
	std::cout.flush();
    }

    return dir;
}

static int check_xkeysymdb(Display *display, bool verbose)
{
    if (verbose)
    {
	(void) xlibdir(display, verbose);
    }

    if (verbose)
    {
	std::cout << "Checking for XKeysymDB... ";
	std::cout.flush();
    }

    String me, my_class;
    XtGetApplicationNameAndClass(display, &me, &my_class);

    string xkeysymdb;

    {
	const _XtString s = getenv("XKEYSYMDB");
	if (s != 0)
	    xkeysymdb = s;
    }

    if (xkeysymdb.empty())
    {
	String s = XtResolvePathname(display, "", "XKeysymDB", "",
				     (String)0, Substitution(0), 0, 
				     XtFilePredicate(0));
	if (s != 0)
	    xkeysymdb = s;
	XtFree(s);
    }

    if (!xkeysymdb.empty())
    {
	if (verbose)
	{
	    std::cout << xkeysymdb << "\n";
	    std::cout.flush();
	}

	// Fix it now
	static string env;
	env = "XKEYSYMDB=" + xkeysymdb;
	putenv(CONST_CAST(char*,env.chars()));

	return 0;			// Okay
    }

    if (xlibdir(display) != 0)
    {
	string path = string(xlibdir(display)) + "/XKeysymDB";
	if (is_file(path))
	{
	    if (verbose)
	    {
		std::cout << path << "\n"
		     << "Note: this is not the default path compiled into " 
		     << me << ".\n"
		     << "    To avoid having " << me 
		     << " determine this setting each time it is started,\n"
		     << "    please set the XKEYSYMDB "
		     << "environment variable to\n"
		     << "    " << quote(path) << ".\n";
		std::cout.flush();
	    }

	    // Fix it
	    static string env;
	    env = "XKEYSYMDB=" + path;
	    putenv(CONST_CAST(char*,env.chars()));
	    return 0;
	}
    }

    if (verbose)
    {
	std::cout << "(none)\n";
	std::cout.flush();
    }

    if (verbose)
    {
	std::cout << "Warning: The `XKeysymDB' file is not in the "
	     << "default X file search path.\n"
	     << "    If " << me << " was not compiled on this machine, " 
	     << me << "\n" 
	     << "    may not run properly "
	     << "(lots of warnings for unknown keysym names).\n";

	if (xlibdir(display) != 0)
	    std::cout << "    and install it into `" << xlibdir(display, verbose) 
	     << "'\n";
	else
	    std::cout << "    and install it into your X project root "
		 << "(typically `/usr/lib/X11')\n";

	std::cout << "    or have the XKEYSYMDB environment variable "
	     << "point at it.\n";

	std::cout.flush();
    }

    return 1;
}

// Set up the X11 library directory
int check_x_configuration(Widget toplevel, bool verbose)
{
    Display *display = XtDisplay(toplevel);

    int ret = 0;

    // This is required for all Motif programs.
    ret = check_xkeysymdb(display, verbose) || ret;

    if (verbose)
    {
	if (ret == 0)
	{
	    std::cout << "No configuration problems found.\n";
	    std::cout.flush();
	}
    }

    return ret;
}
