/* $Id$
 * $Log$
 * Revision 1.27  1995/03/16 23:57:50  mjl
 * Massively reworked.  Now it is absolutely trivial to merge user command line
 * options to the internal options list for use when parsing the command line.
 * The globally visible functions are now:
 *
 *   plClearOpts()   Clear internal option table info structure.
 *   plResetOpts()   Reset internal option table info structure.
 *   plMergeOpts()   Merge user option table into internal info structure.
 *   plSetUsage()    Set the strings used in usage and syntax messages.
 *   plSetOpt()      Process input strings, treating them as an option and
 *                   argument pair.
 *   plParseOpts()   Process options list using current options info.
 *   plOptUsage()    Print usage & syntax message.
 *
 * See internal documentation and either x01c.c or plrender.c for example
 * usage.
 *
 * Revision 1.26  1995/01/13  23:25:24  mjl
 * Fixed exit with an error code of 0 (was 1) when encountering a -v or -h.
 * Nicer in shell scripts when only verifying the version number.
 *
 * Revision 1.25  1995/01/10  09:38:02  mjl
 * Fixed some braindamage incurred last update.
 *
 * Revision 1.24  1995/01/09  21:53:51  mjl
 * Handling of the geometry string improved; now handles strings of the form:
 *     1) -geometry WxH+Xoff+Yoff
 *     2) -geometry WxH
 *     3) -geometry +Xoff+Yoff or -geometry Xoff+Yoff
 *     4) -geometry 0x0+Xoff+Yoff
 * (previously only types 1&2 were handled).  Contributed by Mark Olesen.
 *
 * Revision 1.23  1995/01/06  20:54:53  mjl
 * Added -freeaspect command line flag, also fixed a bogosity introduced
 * last update.
 *
 * Revision 1.22  1994/12/28  09:38:44  mjl
 * Added PL_PARSE_MERGE, set if passed in table is to be merged with internal
 * options.  Function will return on first unrecognized input.  This is the
 * preferred way to combine user and internal options.  User options have
 * precedence over internal options.  Also changed parsing to allow for
 * "holes" in argv list (i.e. all argc entries are checked even if some are
 * 0-length strings or null).
*/

/*
    plargs.c

    Copyright 1993, 1994, 1995
    Maurice LeBrun			mjl@dino.ph.utexas.edu
    Institute for Fusion Studies	University of Texas at Austin

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    Some parts of this code were derived from "xterm.c" and "ParseCmd.c" of 
    the X-windows Version 11 distribution.  The copyright notice is
    reproduced here:

Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved

    The full permission notice is given in the PLplot documentation.

* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

    This file contains routines to extract & process command flags.  The
    command flags recognized by PLplot are stored in the "ploption_table"
    structure, along with strings giving the syntax, long help message, and
    option handler.  

    The command line parser -- plParseOpts() -- removes all recognized flags
    (decreasing argc accordingly), so that invalid input may be readily
    detected.  It can also be used to process user command line flags.  The
    user can merge an option table of type PLOptionTable into the internal
    option table info structure using plMergeOpts().  Or, the user can
    specify that ONLY the external table(s) be parsed by calling
    plClearOpts() before plMergeOpts().  

    The default action taken by plParseOpts() is as follows:
	- Returns with an error if an unrecognized option or badly formed
	  option-value pair are encountered.
	- Returns immediately (return code 0) when the first non-option
	  command line argument is found.
	- Returns with the return code of the option handler, if one
	  was called.
	- Deletes command line arguments from argv list as they are found,
	  and decrements argc accordingly.
	- Does not show "invisible" options in usage or help messages.
	- Assumes the program name is contained in argv[0].

    These behaviors may be controlled through the "mode" argument, which can
    have the following bits set:

    PL_PARSE_FULL -- Full parsing of command line and all error messages
    enabled, including program exit when an error occurs.  Anything on the
    command line that isn't recognized as a valid option or option argument
    is flagged as an error.

    PL_PARSE_QUIET -- Turns off all output except in the case of
    errors.

    PL_PARSE_NODELETE -- Turns off deletion of processed arguments.

    PL_PARSE_SHOWALL -- Show invisible options 

    PL_PARSE_NOPROGRAM -- Specified if argv[0] is NOT a pointer to the
    program name.

    PL_PARSE_NODASH -- Set if leading dash is NOT required.

    Note: if you want to have both your option and a PLplot option of the
    same name processed (e.g. the -v option in plrender), do the following:
	1. Tag your option with PL_OPT_NODELETE
	2. Give it an option handler that uses a return code of 1.
	3. Merge your option table in.
    By merging your table, your option will be processed before the PLplot
    one.  The PL_OPT_NODELETE ensures that the option string is not deleted
    from the argv list, and the return code of 1 ensures that the parser
    continues looking for it.

    See plrender.c for examples of actual usage.  */

#include "plplotP.h"
#include <ctype.h>

/* Support functions */

static int  ParseOpt	(int *, char ***, int *, char ***, PLOptionTable *);
static int  ProcessOpt	(char *, PLOptionTable *, int *, char ***, int *);
static int  GetOptarg	(char **, int *, char ***, int *);
static void Help	(void);
static void Syntax	(void);

/* Option handlers */

static int opt_h		(char *, char *, void *);
static int opt_v		(char *, char *, void *);
static int opt_hack		(char *, char *, void *);
static int opt_dev		(char *, char *, void *);
static int opt_o		(char *, char *, void *);
static int opt_geo		(char *, char *, void *);
static int opt_a		(char *, char *, void *);
static int opt_jx		(char *, char *, void *);
static int opt_jy		(char *, char *, void *);
static int opt_mar		(char *, char *, void *);
static int opt_ori		(char *, char *, void *);
static int opt_freeaspect	(char *, char *, void *);
static int opt_width		(char *, char *, void *);
static int opt_bg		(char *, char *, void *);
static int opt_fam		(char *, char *, void *);
static int opt_fsiz		(char *, char *, void *);
static int opt_fbeg		(char *, char *, void *);
static int opt_finc		(char *, char *, void *);
static int opt_fflen		(char *, char *, void *);
static int opt_bufmax		(char *, char *, void *);
static int opt_nopixmap		(char *, char *, void *);
static int opt_db		(char *, char *, void *);
static int opt_np		(char *, char *, void *);
static int opt_px		(char *, char *, void *);
static int opt_py		(char *, char *, void *);
static int opt_wplt		(char *, char *, void *);

static int opt_plserver		(char *, char *, void *);
static int opt_plwindow		(char *, char *, void *);
static int opt_tcl_cmd		(char *, char *, void *);
static int opt_auto_path	(char *, char *, void *);
static int opt_bufmax		(char *, char *, void *);
static int opt_server_name	(char *, char *, void *);
static int opt_server_host	(char *, char *, void *);
static int opt_server_port	(char *, char *, void *);
static int opt_user		(char *, char *, void *);

/* Global variables */

static char *program = NULL;
static char *usage   = NULL;

static int  mode_full;
static int  mode_quiet;
static int  mode_nodelete;
static int  mode_showall;
static int  mode_noprogram;
static int  mode_nodash;

/*--------------------------------------------------------------------------*\
 * PLPLOT options data structure definition.
 *
 * The table is defined as follows
 *
 * typedef struct {
 *     char *opt;
 *     int  (*handler)	(char *, char *, void *);
 *     void *client_data;
 *     void *var;
 *     long mode;
 *     char *syntax;
 *     char *desc;
 * } PLOptionTable;
 *
 * where each entry has the following meaning:
 *
 * opt		option string
 * handler	pointer to function for processing the option and
 *		 (optionally) its argument
 * client_data	pointer to data that gets passed to (*handler)
 * var		address of variable to set based on "mode"
 * mode		governs handling of option (see below)
 * syntax	short syntax description
 * desc		long syntax description
 *
 * The syntax and or desc strings can be NULL if the option is never to be
 * described.  Usually this is only used for obsolete arguments; those we
 * just wish to hide from normal use are better made invisible (which are
 * made visible by either specifying -showall first or PL_PARSE_SHOWALL).
 *
 * The mode bits are:
 *
 * PL_OPT_ENABLED	Processing for option is enabled
 * PL_OPT_ARG		Option has an argment 
 * PL_OPT_NODELETE	Don't delete after processing 
 * PL_OPT_INVISIBLE	Make invisible (usually for debugging)
 *
 * The following mode bits cause the option to be processed as specified:
 *
 * PL_OPT_FUNC		Call function handler (opt, optarg)
 * PL_OPT_BOOL		Set *var=1
 * PL_OPT_INT		Set *var=atoi(optarg)
 * PL_OPT_FLOAT		Set *var=atof(optarg)
 * PL_OPT_STRING		Set var=optarg
 *
 * where opt points to the option string and optarg points to the
 * argument string.
 *
\*--------------------------------------------------------------------------*/

static PLOptionTable ploption_table[] = {
{
    "showall",			/* Turns on invisible options */
    NULL,
    NULL,
    &mode_showall,
    PL_OPT_BOOL | PL_OPT_ENABLED | PL_OPT_INVISIBLE,
    "-showall",
    "Turns on invisible options" },
{
    "h",			/* Help */
    opt_h,
    NULL,
    NULL,
    PL_OPT_FUNC | PL_OPT_ENABLED,
    "-h",
    "Print out this message" },
{
    "v",			/* Version */
    opt_v,
    NULL,
    NULL,
    PL_OPT_FUNC | PL_OPT_ENABLED,
    "-v",
    "Print out the PLplot library version number" },
{
    "hack",			/* Enable driver-specific hack(s) */
    opt_hack,
    NULL,
    NULL,
    PL_OPT_FUNC | PL_OPT_ENABLED | PL_OPT_INVISIBLE,
    "-hack",
    "Enable driver-specific hack(s)" },
{
    "dev",			/* Output device */
    opt_dev,
    NULL,
    NULL,
    PL_OPT_FUNC | PL_OPT_ENABLED | PL_OPT_ARG,
    "-dev name",
    "Output device name" },
{
    "o",			/* Output filename */
    opt_o,
    NULL,
    NULL,
    PL_OPT_FUNC | PL_OPT_ENABLED | PL_OPT_ARG,
    "-o name",
    "Output filename" },
{
    "display",			/* X server */
    opt_o,
    NULL,
    NULL,
    PL_OPT_FUNC | PL_OPT_ENABLED | PL_OPT_ARG,
    "-display name",
    "X server to contact" },
{
    "px",			/* Plots per page in x */
    opt_px,
    NULL,
    NULL,
    PL_OPT_FUNC | PL_OPT_ENABLED | PL_OPT_ARG,
    "-px number",
    "Plots per page in x" },
{
    "py",			/* Plots per page in y */
    opt_py,
    NULL,
    NULL,
    PL_OPT_FUNC | PL_OPT_ENABLED | PL_OPT_ARG,
    "-py number",
    "Plots per page in y" },
{
    "geometry",			/* Geometry */
    opt_geo,
    NULL,
    NULL,
    PL_OPT_FUNC | PL_OPT_ENABLED | PL_OPT_ARG,
    "-geometry geom",
    "Window size, in pixels (e.g. -geometry 400x300)" },
{
    "geo",			/* Geometry (alias) */
    opt_geo,
    NULL,
    NULL,
    PL_OPT_FUNC | PL_OPT_ENABLED | PL_OPT_ARG | PL_OPT_INVISIBLE,
    "-geo geom",
    "Window size, in pixels (e.g. -geo 400x300)" },
{
    "wplt",			/* Plot window */
    opt_wplt,
    NULL,
    NULL,
    PL_OPT_FUNC | PL_OPT_ENABLED | PL_OPT_ARG,
    "-wplt xl,yl,xr,yr",
    "Relative coordinates [0-1] of window into plot" },
{
    "mar",			/* Margin */
    opt_mar,
    NULL,
    NULL,
    PL_OPT_FUNC | PL_OPT_ENABLED | PL_OPT_ARG,
    "-mar margin",
    "Margin space in relative coordinates (0 to 0.5, def 0)" },
{
    "a",			/* Aspect ratio */
    opt_a,
    NULL,
    NULL,
    PL_OPT_FUNC | PL_OPT_ENABLED | PL_OPT_ARG,
    "-a aspect",
    "Page aspect ratio (def: same as output device)"},
{
    "jx",			/* Justification in x */
    opt_jx,
    NULL,
    NULL,
    PL_OPT_FUNC | PL_OPT_ENABLED | PL_OPT_ARG,
    "-jx justx",
    "Page justification in x (-0.5 to 0.5, def 0)"},
{
    "jy",			/* Justification in y */
    opt_jy,
    NULL,
    NULL,
    PL_OPT_FUNC | PL_OPT_ENABLED | PL_OPT_ARG,
    "-jy justy",
    "Page justification in y (-0.5 to 0.5, def 0)"},
{
    "ori",			/* Orientation */
    opt_ori,
    NULL,
    NULL,
    PL_OPT_FUNC | PL_OPT_ENABLED | PL_OPT_ARG,
    "-ori orient",
    "Plot orientation (0,2=landscape, 1,3=portrait)" },
{
    "freeaspect",		/* floating aspect ratio */
    opt_freeaspect,
    NULL,
    NULL,
    PL_OPT_FUNC | PL_OPT_ENABLED,
    "-freeaspect",
    "Do not preserve aspect ratio on orientation swaps" },
{
    "width",			/* Pen width */
    opt_width,
    NULL,
    NULL,
    PL_OPT_FUNC | PL_OPT_ENABLED | PL_OPT_ARG,
    "-width width",
    "Sets pen width (1 <= width <= 10)" },
{
    "bg",			/* Background color */
    opt_bg,
    NULL,
    NULL,
    PL_OPT_FUNC | PL_OPT_ENABLED | PL_OPT_ARG,
    "-bg color",
    "Background color (0=black, FFFFFF=white)" },
{
    "fam",			/* Familying on switch */
    opt_fam,
    NULL,
    NULL,
    PL_OPT_FUNC | PL_OPT_ENABLED,
    "-fam",
    "Create a family of output files" },
{
    "fsiz",			/* Family file size */
    opt_fsiz,
    NULL,
    NULL,
    PL_OPT_FUNC | PL_OPT_ENABLED | PL_OPT_ARG,
    "-fsiz size",
    "Output family file size in MB (e.g. -fsiz 1.0)" },
{
    "fbeg",			/* Family starting member */
    opt_fbeg,
    NULL,
    NULL,
    PL_OPT_FUNC | PL_OPT_ENABLED | PL_OPT_ARG,
    "-fbeg number",
    "First family member number on output" },
{
    "finc",			/* Family member increment */
    opt_finc,
    NULL,
    NULL,
    PL_OPT_FUNC | PL_OPT_ENABLED | PL_OPT_ARG,
    "-finc number",
    "Increment between family members" },
{
    "fflen",			/* Family member min field width */
    opt_fflen,
    NULL,
    NULL,
    PL_OPT_FUNC | PL_OPT_ENABLED | PL_OPT_ARG,
    "-fflen length",
    "Family member number minimum field width" },
{
    "nopixmap",			/* Do not use pixmaps */
    opt_nopixmap,
    NULL,
    NULL,
    PL_OPT_FUNC | PL_OPT_ENABLED,
    "-nopixmap",
    "Don't use pixmaps in X-based drivers" },
{
    "db",			/* Double buffering on switch */
    opt_db,
    NULL,
    NULL,
    PL_OPT_FUNC | PL_OPT_ENABLED,
    "-db",
    "Double buffer X window output" },
{
    "np",			/* Page pause off switch */
    opt_np,
    NULL,
    NULL,
    PL_OPT_FUNC | PL_OPT_ENABLED,
    "-np",
    "No pause between pages" },
{
    "bufmax",			/* # bytes sent before flushing output */
    opt_bufmax,
    NULL,
    NULL,
    PL_OPT_FUNC | PL_OPT_ENABLED | PL_OPT_ARG | PL_OPT_INVISIBLE,
    "-bufmax",
    "bytes sent before flushing output" },
{
    "server_name",		/* Main window name of server */
    opt_server_name,
    NULL,
    NULL,
    PL_OPT_FUNC | PL_OPT_ENABLED | PL_OPT_ARG,
    "-server_name name",
    "Main window name of PLplot server (tk driver)" },
{
    "server_host",		/* Host to run server on */
    opt_server_host,
    NULL,
    NULL,
    PL_OPT_FUNC | PL_OPT_ENABLED | PL_OPT_ARG,
    "-server_host name",
    "Host to run PLplot server on (dp driver)" },
{
    "server_port",		/* Port to talk to server on */
    opt_server_port,
    NULL,
    NULL,
    PL_OPT_FUNC | PL_OPT_ENABLED | PL_OPT_ARG,
    "-server_port name",
    "Port to talk to PLplot server on (dp driver)" },
{
    "user",			/* user name on remote node */
    opt_user,
    NULL,
    NULL,
    PL_OPT_FUNC | PL_OPT_ENABLED | PL_OPT_ARG,
    "-user name",
    "User name on remote node (dp driver)" },
{
    "plserver",			/* PLplot server name */
    opt_plserver,
    NULL,
    NULL,
    PL_OPT_FUNC | PL_OPT_ENABLED | PL_OPT_ARG | PL_OPT_INVISIBLE,
    "-plserver name",
    "Invoked name of PLplot server (tk or dp driver)" },
{
    "plwindow",			/* PLplot container window name */
    opt_plwindow,
    NULL,
    NULL,
    PL_OPT_FUNC | PL_OPT_ENABLED | PL_OPT_ARG | PL_OPT_INVISIBLE,
    "-plwindow name",
    "Name of PLplot container window (tk or dp driver)" },
{
    "tcl_cmd",			/* TCL initialization command */
    opt_tcl_cmd,
    NULL,
    NULL,
    PL_OPT_FUNC | PL_OPT_ENABLED | PL_OPT_ARG | PL_OPT_INVISIBLE,
    "-tcl_cmd command",
    "TCL command string run at startup (note: disabled)" },
{
    "auto_path",		/* Additional directory(s) to autoload */
    opt_auto_path,
    NULL,
    NULL,
    PL_OPT_FUNC | PL_OPT_ENABLED | PL_OPT_ARG | PL_OPT_INVISIBLE,
    "-auto_path dir",
    "Additional directory(s) to autoload (tk or dp driver)" },
{
    NULL,			/* option */
    NULL,			/* handler */
    NULL,			/* client data */
    NULL,			/* address of variable to set */
    0,				/* mode flag */
    NULL,			/* short syntax */
    NULL }			/* long syntax */
};

static char *plplot_notes[] = {
"All parameters must be white-space delimited.  Some options are driver",
"dependent.  Please see the PLplot reference document for more detail.",
NULL};

/*--------------------------------------------------------------------------*\
 * Array of option tables and associated info.
 *
 * The user may merge up to PL_MAX_OPT_TABLES custom option tables (of type
 * PLOptionTable) with the internal one.  The resulting treatment is simple,
 * powerful, and robust.  The tables are parsed in the order of last added
 * first, to the internal table last.  If multiple options of the same name
 * occur, only the first parsed is "seen", thus, the user can easily
 * override any PLplot internal option merely by providing the same option.
 * This same precedence is followed when printing help and usage messages,
 * with each set of options given separately.  See example usage in
 * plrender.c.
\*--------------------------------------------------------------------------*/

typedef struct {
    PLOptionTable *options;
    char *name;
    char **notes;
} PLOptionInfo;

PLOptionInfo ploption_info_default = {
    ploption_table,
    "PLplot options",
    plplot_notes
};

#define PL_MAX_OPT_TABLES 10
PLOptionInfo ploption_info[PL_MAX_OPT_TABLES] = { 
    {
	ploption_table,
	"PLplot options",
	plplot_notes
    }
};

static int  tables = 1;

/*--------------------------------------------------------------------------*\
 * plSetOpt()
 *
 * Process input strings, treating them as an option and argument pair.
 * Returns 1 on an error.
\*--------------------------------------------------------------------------*/

int
plSetOpt(char *opt, char *optarg)
{
    int mode = 0, argc = 2, status;
    char *argv[3];

    argv[0] = opt;
    argv[1] = optarg;
    argv[2] = NULL;
    mode =
	PL_PARSE_QUIET |
	PL_PARSE_NODELETE |
	PL_PARSE_NOPROGRAM |
	PL_PARSE_NODASH;

    status = plParseOpts(&argc, argv, mode);
    if (status) {
	fprintf( stderr, "plSetOpt: Unrecognized option %s\n", opt);
    }
    return status;
}

/*--------------------------------------------------------------------------*\
 * plMergeOpts()
 *
 * Merge user option table info structure with internal one.
\*--------------------------------------------------------------------------*/

int
plMergeOpts(PLOptionTable *options, char *name, char **notes)
{
    if (tables++ >= PL_MAX_OPT_TABLES) {
	plabort("Max options tables limit exceeded");
	return 1;
    }

    ploption_info[tables-1].options = options;
    ploption_info[tables-1].name    = name;
    ploption_info[tables-1].notes   = notes;

    return 0;
}

/*--------------------------------------------------------------------------*\
 * plClearOpts()
 *
 * Clear internal option table info structure.
\*--------------------------------------------------------------------------*/

void
plClearOpts(void)
{
    tables = 0;
}

/*--------------------------------------------------------------------------*\
 * plResetOpts()
 *
 * Reset internal option table info structure.
\*--------------------------------------------------------------------------*/

void
plResetOpts(void)
{
    ploption_info[0] = ploption_info_default;
    tables = 1;
}

/*--------------------------------------------------------------------------*\
 * plParseOpts()
 *
 * Process options list using current ploptions_info structure.
 * An error in parsing the argument list causes a program exit if
 * mode_full is set, otherwise the function returns with an error.
\*--------------------------------------------------------------------------*/

int
plParseOpts(int *p_argc, char **argv, PLINT mode)
{
    char **argsave, **argend;
    int	i, myargc, status = 0;

/* Initialize */

    mode_full      = mode & PL_PARSE_FULL;
    mode_quiet     = mode & PL_PARSE_QUIET;
    mode_nodelete  = mode & PL_PARSE_NODELETE;
    mode_showall   = mode & PL_PARSE_SHOWALL;
    mode_noprogram = mode & PL_PARSE_NOPROGRAM;
    mode_nodash    = mode & PL_PARSE_NODASH;

    myargc = (*p_argc); 
    argend = argv + myargc;

/* If program name is first argument, save and advance */

    if ( ! mode_noprogram) {
	program = argv[0];
	plsc->program = argv[0];

	--myargc; ++argv;
	argsave = argv;
    }

    if (myargc == 0)
	return 0;

/* Process the command line */

    for (; myargc > 0; --myargc, ++argv) {

    /* Allow for "holes" in argv list */

	if (*argv == NULL || *argv[0] == '\0')
	    continue;

    /* Loop over all options tables, starting with the last */

	for (i = tables-1; i >= 0; i--) {

	/* Check option table for option */

	    status = ParseOpt(&myargc, &argv, p_argc, &argsave,
			      ploption_info[i].options);

	    if ( ! status) break;
	}

    /* Handle error return as specified by the mode flag */

	switch (status) {

	/* No match.  Abort if fully parsing, otherwise return without */
	/* error. */ 

	case -1:
	    if ( ! mode_quiet && mode_full) {
		fprintf(stderr, "\nBad command line option \"%s\"\n", argv[0]);
		plOptUsage();
	    }
	    if (mode_full) exit(1);

	    status = 0;
	    break;

	/* Illegal or badly formed */

	case 1:
	    if ( ! mode_quiet) {
		fprintf(stderr, "\nBad command line option \"%s\"\n", argv[0]);
		plOptUsage();
	    }
	    if (mode_full) exit(1);

	    break;

	/* Informational option encountered (-h or -v) */

	case 2:
	    exit(0);
	}
    }

/* Compress and NULL-terminate argv */

    if ( ! mode_nodelete) {
	*argsave++ = *argv;

	if (argsave < argend)
	    *argsave = NULL;
    }

    return status;
}

/*--------------------------------------------------------------------------*\
 * ParseOpt()
 *
 * Parses & determines appropriate action for input flag.
\*--------------------------------------------------------------------------*/

static int
ParseOpt(int *p_myargc, char ***p_argv, int *p_argc, char ***p_argsave,
	 PLOptionTable *option_table)
{
    PLOptionTable *tab;
    char *opt;

/* Only handle actual flags and their arguments */

    if ( mode_nodash || (*p_argv)[0][0] == '-') {

	opt = (*p_argv)[0];
	if (*opt == '-') 
	    opt++;

	for (tab = option_table; tab->opt; tab++) {

	/* Skip if option not enabled */

	    if ( ! (tab->mode & PL_OPT_ENABLED)) 
		continue;

	    if (*opt == *tab->opt && ! strcmp(opt, tab->opt)) {

	    /* Option matched, so remove from argv list if applicable. */

		if ( ! mode_nodelete) {
		    if (tab->mode & PL_OPT_NODELETE)
			(*(*p_argsave)++) = (**p_argv);
		    else
			--(*p_argc);
		}

	    /* Process option (and argument if applicable) */

		return (ProcessOpt(opt, tab, p_myargc, p_argv, p_argc));
	    }
	}
    }

    return -1;
}

/*--------------------------------------------------------------------------*\
 * ProcessOpt()
 *
 * Process option (and argument if applicable).
\*--------------------------------------------------------------------------*/

static int
ProcessOpt(char *opt, PLOptionTable *tab, int *p_myargc, char ***p_argv,
	   int *p_argc)
{
    int need_arg;
    char *optarg = NULL;

/* Get option argument if necessary */

    need_arg = PL_OPT_ARG | PL_OPT_INT | PL_OPT_FLOAT | PL_OPT_STRING;

    if (tab->mode & need_arg) {
	if (GetOptarg(&optarg, p_myargc, p_argv, p_argc))
	    return 1;
    }

/* Process argument */

    switch (tab->mode & 0xFF00) {

    case PL_OPT_FUNC:

    /* Call function handler to do the job */

	if (tab->handler == NULL) {
	    fprintf(stderr,
		    "ProcessOpt: no handler specified for option %s\n",
		    tab->opt);
	    return 1;
	}
	return ((*tab->handler) (opt, optarg, tab->client_data));

    case PL_OPT_BOOL:

    /* Set *var as a boolean */

	if (tab->var == NULL) {
	    fprintf(stderr,
		    "ProcessOpt: no variable specified for option %s\n",
		    tab->opt);
	    return 1;
	}
	*(int *)tab->var = 1;
	break;

    case PL_OPT_INT:

    /* Set *var as an int */

	if (tab->var == NULL) {
	    fprintf(stderr,
		    "ProcessOpt: no variable specified for option %s\n",
		    tab->opt);
	    return 1;
	}
	*(int *)tab->var = atoi(optarg);
	break;

    case PL_OPT_FLOAT:

    /* Set *var as a float */

	if (tab->var == NULL) {
	    fprintf(stderr,
		    "ProcessOpt: no variable specified for option %s\n",
		    tab->opt);
	    return 1;
	}
	*(float *)tab->var = atof(optarg);
	break;

    case PL_OPT_STRING:

    /* Set var (can be NULL initially) to point to optarg string */

	tab->var = optarg;
	break;

    default:

    /* Somebody messed up.. */

	fprintf(stderr,
		"ProcessOpt: invalid processing mode for option %s\n",
		tab->opt);
	return 1;
    }
    return 0;
}

/*--------------------------------------------------------------------------*\
 * GetOptarg()
 *
 * Retrieves an option argument.
 * If an error occurs here it is a true syntax error.
\*--------------------------------------------------------------------------*/

static int
GetOptarg(char **poptarg, int *p_myargc, char ***p_argv, int *p_argc)
{
    int result = 0;

    --(*p_myargc);

    if ((*p_myargc) <= 0)		/* oops, no more arguments */
	result = 1;

    if ( ! result) {
	(*p_argv)++;
	if ((*p_argv)[0][0] == '-' && isalpha((*p_argv)[0][1])) {

	    (*p_argv)--;		/* oops, next arg is a flag */
	    result = 1;
	}
    }

    if ( ! result) {			/* yeah, the user got it right */
	(*p_argc)--;
	*poptarg = (*p_argv)[0];
    }
    else {
	if ( ! mode_quiet) {
	    fprintf(stderr, "Argument missing for %s option.\n", (*p_argv)[0]);
	    plOptUsage();
	}
    }
    return result;
}

/*--------------------------------------------------------------------------*\
 * plSetUsage()
 *
 * Set the strings used in usage and syntax messages.
\*--------------------------------------------------------------------------*/

void
plSetUsage(char *program_string, char *usage_string)
{
    if (program_string != NULL)
	program = program_string;

    if (usage_string != NULL)
	usage = usage_string;
}

/*--------------------------------------------------------------------------*\
 * plOptUsage()
 *
 * Print usage & syntax message.
\*--------------------------------------------------------------------------*/

void
plOptUsage(void)
{
    if (usage == NULL)
	fprintf(stderr, "\nUsage:\n        %s [options]\n", program);
    else
	fputs(usage, stderr);

    Syntax();

    fprintf(stderr, "\n\nType %s -h for a full description.\n\n",
	    program);
}

/*--------------------------------------------------------------------------*\
 * Syntax()
 *
 * Print short syntax message.
\*--------------------------------------------------------------------------*/

static void
Syntax(void)
{
    PLOptionTable *tab;
    int i, col, len;

/* Loop over all options tables */

    for (i = tables-1; i >= 0; i--) {

    /* Introducer */

	if (ploption_info[i].name)
	    fprintf(stderr, "\n%s:", ploption_info[i].name);
	else
	    fputs("\nUser options:", stderr);

    /* Print syntax for each option */

	col = 80;
	for (tab = ploption_info[i].options; tab->opt; tab++) {
	    if ( ! (tab->mode & PL_OPT_ENABLED))
		continue;

	    if ( ! mode_showall && (tab->mode & PL_OPT_INVISIBLE))
		continue;

	    if (tab->syntax == NULL)
		continue;

	    len = 3 + strlen(tab->syntax);		/* space [ string ] */
	    if (col + len > 79) {
		fprintf(stderr, "\n   ");		/* 3 spaces */
		col = 3;
	    }
	    fprintf(stderr, " [%s]", tab->syntax);
	    col += len;
	}
	fprintf(stderr, "\n");
    }
}

/*--------------------------------------------------------------------------*\
 * Help()
 *
 * Print long help message.
\*--------------------------------------------------------------------------*/

static void
Help(void)
{
    PLOptionTable *tab;
    char **note;
    int i;

/* Loop over all options tables */

    for (i = tables-1; i >= 0; i--) {

    /* Introducer */

	if (ploption_info[i].name)
	    fprintf(stderr, "\n%s:\n", ploption_info[i].name);
	else
	    fputs("\nUser options:\n", stderr);

    /* Print description for each option */

	for (tab = ploption_info[i].options; tab->opt; tab++) {
	    if ( ! (tab->mode & PL_OPT_ENABLED))
		continue;

	    if ( ! mode_showall && (tab->mode & PL_OPT_INVISIBLE))
		continue;

	    if (tab->desc == NULL)
		continue;

	    if (tab->mode & PL_OPT_INVISIBLE) 
		fprintf(stderr, " *  %-20s %s\n", tab->syntax, tab->desc);
	    else 
		fprintf(stderr, "    %-20s %s\n", tab->syntax, tab->desc);
	}

    /* Usage notes */

	if (ploption_info[i].notes) {
	    putc('\n', stderr);
	    for (note = ploption_info[i].notes; *note; note++) {
		fputs(*note, stderr);
		putc('\n', stderr);
	    }
	}
    }
}

/*--------------------------------------------------------------------------*\
 * Option handlers
\*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*\
 * opt_h()
 *
 * Performs appropriate action for option "h":
 * Issues help message
\*--------------------------------------------------------------------------*/

static int
opt_h(char *opt, char *optarg, void *client_data)
{
    if ( ! mode_quiet) {
	if (usage == NULL)
	    fprintf(stderr, "\nUsage:\n        %s [options]\n", program);
	else
	    fputs(usage, stderr);

	Help();
    }
    return 2;
}

/*--------------------------------------------------------------------------*\
 * opt_v()
 *
 * Performs appropriate action for option "v":
 * Issues version message
\*--------------------------------------------------------------------------*/

static int
opt_v(char *opt, char *optarg, void *client_data)
{
    if ( ! mode_quiet) {
	fprintf(stderr, "PLplot library version: %s\n", PLPLOT_VERSION);
    }
    return 2;
}

/*--------------------------------------------------------------------------*\
 * opt_hack()
 *
 * Performs appropriate action for option "hack":
 * Enables driver-specific hack(s)
\*--------------------------------------------------------------------------*/

static int
opt_hack(char *opt, char *optarg, void *client_data)
{
    plsc->hack = 1;
    return 0;
}

/*--------------------------------------------------------------------------*\
 * opt_dev()
 *
 * Performs appropriate action for option "dev":
 * Sets output device keyword
\*--------------------------------------------------------------------------*/

static int
opt_dev(char *opt, char *optarg, void *client_data)
{
    plsdev(optarg);
    return 0;
}

/*--------------------------------------------------------------------------*\
 * opt_o()
 *
 * Performs appropriate action for option "o":
 * Sets output file name
\*--------------------------------------------------------------------------*/

static int
opt_o(char *opt, char *optarg, void *client_data)
{
    plsfnam(optarg);
    return 0;
}

/*--------------------------------------------------------------------------*\
 * opt_mar()
 *
 * Performs appropriate action for option "mar":
 * Sets relative margin width
\*--------------------------------------------------------------------------*/

static int
opt_mar(char *opt, char *optarg, void *client_data)
{
    plsdidev(atof(optarg), PL_NOTSET, PL_NOTSET, PL_NOTSET);
    return 0;
}

/*--------------------------------------------------------------------------*\
 * opt_a()
 *
 * Performs appropriate action for option "a":
 * Sets plot aspect ratio on page
\*--------------------------------------------------------------------------*/

static int
opt_a(char *opt, char *optarg, void *client_data)
{
    plsdidev(PL_NOTSET, atof(optarg), PL_NOTSET, PL_NOTSET);
    return 0;
}

/*--------------------------------------------------------------------------*\
 * opt_jx()
 *
 * Performs appropriate action for option "jx":
 * Sets relative justification in x
\*--------------------------------------------------------------------------*/

static int
opt_jx(char *opt, char *optarg, void *client_data)
{
    plsdidev(PL_NOTSET, PL_NOTSET, atof(optarg), PL_NOTSET);
    return 0;
}

/*--------------------------------------------------------------------------*\
 * opt_jy()
 *
 * Performs appropriate action for option "jy":
 * Sets relative justification in y
\*--------------------------------------------------------------------------*/

static int
opt_jy(char *opt, char *optarg, void *client_data)
{
    plsdidev(PL_NOTSET, PL_NOTSET, PL_NOTSET, atof(optarg));
    return 0;
}

/*--------------------------------------------------------------------------*\
 * opt_ori()
 *
 * Performs appropriate action for option "ori":
 * Sets orientation
\*--------------------------------------------------------------------------*/

static int
opt_ori(char *opt, char *optarg, void *client_data)
{
    plsdiori(atof(optarg));
    return 0;
}

/*--------------------------------------------------------------------------*\
 * opt_freeaspect()
 *
 * Performs appropriate action for option "freeaspect":
 * Do not preserve aspect ratio on orientation swaps.
\*--------------------------------------------------------------------------*/

static int
opt_freeaspect(char *opt, char *optarg, void *client_data)
{
    plsc->freeaspect = 1;
    return 0;
}

/*--------------------------------------------------------------------------*\
 * opt_width()
 *
 * Performs appropriate action for option "width":
 * Sets pen width
\*--------------------------------------------------------------------------*/

static int
opt_width(char *opt, char *optarg, void *client_data)
{
    int width;

    width = atoi(optarg);
    if (width == 0) {
	fprintf(stderr, "?invalid width\n");
	return 1;
    }
    else {
	plwid(width);
	plsc->widthlock = 1;
    }
    return 0;
}

/*--------------------------------------------------------------------------*\
 * opt_bg()
 *
 * Performs appropriate action for option "bg":
 * Sets background color
\*--------------------------------------------------------------------------*/

static int
opt_bg(char *opt, char *optarg, void *client_data)
{
    char *rgb;
    long bgcolor, r, g, b;

/* Always in hex!  Strip off leading "#" (TK-ism) if present. */

    if (*optarg == '#')
	rgb = optarg + 1;
    else
	rgb = optarg;

/* Get number in hex */

    bgcolor = strtol(rgb, NULL, 16);

/* Must be either a 3 or 6 digit hex number */
/* If 3 digits, each is "doubled" (i.e. ABC becomes AABBCC). */

    switch (strlen(rgb)) {
    case 3:
	r = (bgcolor & 0xF00) >> 8;
	g = (bgcolor & 0x0F0) >> 4;
	b = (bgcolor & 0x00F);

	r = r | (r << 4);
	g = g | (g << 4);	/* doubling */
	b = b | (b << 4);
	break;

    case 6:
	r = (bgcolor & 0xFF0000) >> 16;
	g = (bgcolor & 0x00FF00) >> 8;
	b = (bgcolor & 0x0000FF);
	break;

    default:
	fprintf(stderr, "Unrecognized background color value %s\n", rgb);
	return 1;
    }

    plscolbg(r, g, b);

    return 0;
}

/*--------------------------------------------------------------------------*\
 * opt_wplt()
 *
 * Performs appropriate action for option "wplt":
 * Sets (zoom) window into plot (e.g. "0,0,0.5,0.5")
\*--------------------------------------------------------------------------*/

static int
opt_wplt(char *opt, char *optarg, void *client_data)
{
    char *field;
    float xl, yl, xr, yr;

    if ((field = strtok(optarg, ",")) == NULL)
	return 1;

    xl = atof(field);

    if ((field = strtok(NULL, ",")) == NULL)
	return 1;

    yl = atof(field);

    if ((field = strtok(NULL, ",")) == NULL)
	return 1;

    xr = atof(field);

    if ((field = strtok(NULL, ",")) == NULL)
	return 1;

    yr = atof(field);

    plsdiplt(xl, yl, xr, yr);
    return 0;
}

/*--------------------------------------------------------------------------*\
 * opt_fam()
 *
 * Performs appropriate action for option "fam":
 * Enables family output files
\*--------------------------------------------------------------------------*/

static int
opt_fam(char *opt, char *optarg, void *client_data)
{
    plsfam(1, -1, -1);
    return 0;
}

/*--------------------------------------------------------------------------*\
 * opt_fsiz()
 *
 * Performs appropriate action for option "fsiz":
 * Sets size of a family member file (may be somewhat larger since eof must
 * occur at a page break).
\*--------------------------------------------------------------------------*/

static int
opt_fsiz(char *opt, char *optarg, void *client_data)
{
    PLINT bytemax;

    bytemax = 1.0e6 * atof(optarg);
    if (bytemax == 0) {
	fprintf(stderr, "?invalid bytemax\n");
	return 1;
    }
    plsfam(-1, -1, bytemax);

    return 0;
}

/*--------------------------------------------------------------------------*\
 * opt_fbeg()
 *
 * Performs appropriate action for option "fbeg":
 * Starts with the specified family member number.
\*--------------------------------------------------------------------------*/

static int
opt_fbeg(char *opt, char *optarg, void *client_data)
{
    plsc->member = atoi(optarg);

    return 0;
}

/*--------------------------------------------------------------------------*\
 * opt_finc()
 *
 * Performs appropriate action for option "finc":
 * Specify increment between family members.
\*--------------------------------------------------------------------------*/

static int
opt_finc(char *opt, char *optarg, void *client_data)
{
    plsc->finc = atoi(optarg);

    return 0;
}

/*--------------------------------------------------------------------------*\
 * opt_fflen()
 *
 * Performs appropriate action for option "fflen":
 * Specify minimum field length for family member number.
\*--------------------------------------------------------------------------*/

static int
opt_fflen(char *opt, char *optarg, void *client_data)
{
    plsc->fflen = atoi(optarg);

    return 0;
}

/*--------------------------------------------------------------------------*\
 * opt_np()
 *
 * Performs appropriate action for option "np":
 * Disables pause between pages
\*--------------------------------------------------------------------------*/

static int
opt_np(char *opt, char *optarg, void *client_data)
{
    plspause(0);
    return 0;
}

/*--------------------------------------------------------------------------*\
 * opt_nopixmap()
 *
 * Performs appropriate action for option "nopixmap":
 * Disables use of pixmaps in X drivers
\*--------------------------------------------------------------------------*/

static int
opt_nopixmap(char *opt, char *optarg, void *client_data)
{
    plsc->nopixmap = 1;
    return 0;
}

/*--------------------------------------------------------------------------*\
 * opt_db()
 *
 * Performs appropriate action for option "db":
 * Double buffer X output (update only done on eop or Expose)
\*--------------------------------------------------------------------------*/

static int
opt_db(char *opt, char *optarg, void *client_data)
{
    plsc->db = 1;
    return 0;
}

/*--------------------------------------------------------------------------*\
 * opt_bufmax()
 *
 * Performs appropriate action for option "bufmax":
 * Sets size of data buffer for tk driver
\*--------------------------------------------------------------------------*/

static int
opt_bufmax(char *opt, char *optarg, void *client_data)
{
    plsc->bufmax = atoi(optarg);
    return 0;
}

/*--------------------------------------------------------------------------*\
 * opt_server_name()
 *
 * Performs appropriate action for option "server_name":
 * Sets main window name of server (Tcl/TK/DP driver only)
\*--------------------------------------------------------------------------*/

static int
opt_server_name(char *opt, char *optarg, void *client_data)
{
    plsc->server_name = optarg;
    return 0;
}

/*--------------------------------------------------------------------------*\
 * opt_server_host()
 *
 * Performs appropriate action for option "server_host":
 * Sets host to run server on (Tcl/TK/DP driver only)
\*--------------------------------------------------------------------------*/

static int
opt_server_host(char *opt, char *optarg, void *client_data)
{
    plsc->server_host = optarg;
    return 0;
}

/*--------------------------------------------------------------------------*\
 * opt_server_port()
 *
 * Performs appropriate action for option "server_port":
 * Sets port to talk to server on (Tcl/TK/DP driver only)
\*--------------------------------------------------------------------------*/

static int
opt_server_port(char *opt, char *optarg, void *client_data)
{
    plsc->server_port = optarg;
    return 0;
}

/*--------------------------------------------------------------------------*\
 * opt_user()
 *
 * Performs appropriate action for option "user":
 * Sets user name on remote node (for remsh), dp driver only
\*--------------------------------------------------------------------------*/

static int
opt_user(char *opt, char *optarg, void *client_data)
{
    plsc->user = optarg;
    return 0;
}

/*--------------------------------------------------------------------------*\
 * opt_plserver()
 *
 * Performs appropriate action for option "plserver":
 * Sets name to use when invoking server (Tcl/TK/DP driver only)
\*--------------------------------------------------------------------------*/

static int
opt_plserver(char *opt, char *optarg, void *client_data)
{
    plsc->plserver = optarg;
    return 0;
}

/*--------------------------------------------------------------------------*\
 * opt_plwindow()
 *
 * Performs appropriate action for option "plwindow":
 * Sets PLplot window name
\*--------------------------------------------------------------------------*/

static int
opt_plwindow(char *opt, char *optarg, void *client_data)
{
    plsc->plwindow = optarg;
    return 0;
}

/*--------------------------------------------------------------------------*\
 * opt_tcl_cmd()
 *
 * Performs appropriate action for option "tcl_cmd":
 * Sets TCL command(s) to eval on startup
\*--------------------------------------------------------------------------*/

static int
opt_tcl_cmd(char *opt, char *optarg, void *client_data)
{
    plsc->tcl_cmd = optarg;
    return 0;
}

/*--------------------------------------------------------------------------*\
 * opt_auto_path()
 *
 * Performs appropriate action for option "auto_path":
 * Sets additional directories to autoload
\*--------------------------------------------------------------------------*/

static int
opt_auto_path(char *opt, char *optarg, void *client_data)
{
    plsc->auto_path = optarg;
    return 0;
}

/*--------------------------------------------------------------------------*\
 * opt_px()
 *
 * Performs appropriate action for option "px":
 * Set packing in x
\*--------------------------------------------------------------------------*/

static int
opt_px(char *opt, char *optarg, void *client_data)
{
    plssub(atoi(optarg), -1);
    return 0;
}

/*--------------------------------------------------------------------------*\
 * opt_py()
 *
 * Performs appropriate action for option "py":
 * Set packing in y
\*--------------------------------------------------------------------------*/

static int
opt_py(char *opt, char *optarg, void *client_data)
{
    plssub(-1, atoi(optarg));
    return 0;
}

/*--------------------------------------------------------------------------*\
 * opt_geo()
 *
 * Performs appropriate action for option "geo":
 * Set geometry for output window
 *   e.g.,  "-geometry 400x400+100+0"
 * or with offsets alone "-geometry +Xoff+Yoff"
 *   e.g., "-geometry +100+0"
\*--------------------------------------------------------------------------*/

static int
opt_geo(char *opt, char *optarg, void *client_data)
{
    char *field;
    PLFLT xdpi = 0., ydpi = 0.;
    PLINT xwid = 0, ywid = 0, xoff = 0, yoff = 0;
    
    /* The TK driver uses the geometry string directly */    

    plsc->geometry = (char *) malloc((size_t)(1+strlen(optarg))*sizeof(char));
    strcpy (plsc->geometry, optarg);

    if (strchr (optarg, 'x')) {

    /* -geometry WxH or -geometry WxH+Xoff+Yoff */

	field = strtok (optarg, "x");
	xwid = atoi (field);
	if (xwid == 0)
	    fprintf (stderr, "?invalid xwid\n");

	if ((field = strtok (NULL, "+")) == NULL)
	    return 1;

	ywid = atoi (field);
	if (ywid == 0)
	    fprintf (stderr, "?invalid ywid\n");

	field = strtok (NULL, "+");
    }
    else {

    /* -geometry +Xoff or -geometry +Xoff+Yoff only */

	field = strtok (optarg, "+");
    }

    if (field != NULL) {
	xoff = atoi (field);
	if ((field = strtok (NULL, "+")) != NULL)
	    yoff = atoi (field);
    }

    plspage (xdpi, ydpi, xwid, ywid, xoff, yoff);
    return 0;
}

