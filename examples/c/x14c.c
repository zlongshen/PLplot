/* $Id$
 * $Log$
 * Revision 1.12  1995/03/16 23:18:57  mjl
 * All example C programs: changed plParseInternalOpts() call to plParseOpts().
 *
 * Revision 1.11  1994/08/25  04:02:32  mjl
 * Now can work with either TK or Tcl-DP.  Changed to use TK by default.
 *
 * Revision 1.10  1994/08/10  05:28:50  mjl
 * Ensured that geometry strings are in writable memory as required, and
 * other minor tweaks to improve the demo.
 *
 * Revision 1.9  1994/06/30  17:57:54  mjl
 * All C example programs: made another pass to eliminate warnings when using
 * gcc -Wall.  Lots of cleaning up: got rid of includes of math.h or string.h
 * (now included by plplot.h), eliminated redundant casts, put in more
 * uniform comments, and other minor changes.
 *
 * Revision 1.8  1994/05/14  05:43:56  mjl
 * Now uses the DP driver and finally works the way I always wanted it to.
 *
 * Revision 1.7  1994/03/30  07:21:58  mjl
 * Changes to all C example programs: special handling for malloc re: header
 * files eliminated, include of stdio.h and stdlib.h eliminated (now done
 * by plplot.h), include of "plplot.h" changed to <plplot.h> to enable
 * simpler builds by the general user, some cleaning up also.
*/

/*	x14c.c

	Demo of multiple stream/window capability (requires Tk or Tcl-DP).

	Maurice LeBrun
	IFS, University of Texas at Austin
*/

#include <plplot.h>

#ifndef ROUND
#define ROUND(a)    (PLINT)((a)<0. ? ((a)-.5) : ((a)+.5))
#endif

static PLFLT x[101], y[101];
static PLFLT xscale, yscale, xoff, yoff, xs[6], ys[6];
static PLINT space0 = 0, mark0 = 0, space1 = 1500, mark1 = 1500;

void plot1();
void plot2();
void plot3();
void plot4();
void plot5();

/*--------------------------------------------------------------------------*\
 * main
 *
 * Plots several simple functions from other example programs.
 *
 * This version sends the output of the first 4 plots (one page) to two
 * independent streams.  
\*--------------------------------------------------------------------------*/

int
main(void)
{
    int i, digmax;
    int xleng0 = 400, yleng0 = 300, xoff0 = 200, yoff0 = 200;
    int xleng1 = 400, yleng1 = 300, xoff1 = 500, yoff1 = 500;

/* Select either TK or DP driver and use a small window */
/* The geometry strings MUST be in writable memory */

    char driver[] = "tk";
    char geometry_master[] = "500x410+100+200";
    char geometry_slave[]  = "500x410+650+200";

    printf("Demo of multiple output streams via the %s driver.\n", driver);
    printf("Running with the second window as slave.\n");
    printf("\n");

/* Set up first stream */

    plSetInternalOpt("geometry", geometry_master);

    plsdev(driver);
    plssub(2, 2);
    plinit();

/* Start next stream */

    plsstrm(1);

/* Turn off pause to make this a slave (must follow master) */

    plSetInternalOpt("geometry", geometry_slave);
    plspause(0);
    plsdev(driver);
    plinit();

/* Set up the data & plot */
/* Original case */

    plsstrm(0);

    xscale = 6.;
    yscale = 1.;
    xoff = 0.;
    yoff = 0.;
    plot1();

/* Set up the data & plot */

    xscale = 1.;
    yscale = 1.e+6;
    plot1();

/* Set up the data & plot */

    xscale = 1.;
    yscale = 1.e-6;
    digmax = 2;
    plsyax(digmax, 0);
    plot1();

/* Set up the data & plot */

    xscale = 1.;
    yscale = 0.0014;
    yoff = 0.0185;
    digmax = 5;
    plsyax(digmax, 0);
    plot1();

/* To slave */
/* The pleop() ensures the eop indicator gets lit. */

    plsstrm(1);
    plot4();
    pleop();

/* Back to master */

    plsstrm(0);
    plot2();
    plot3();

/* To slave */

    plsstrm(1);
    plot5();
    pleop();

/* Back to master to wait for user to advance */

    plsstrm(0);
    pleop();

/* Call plend to finish off. */

    plend();
    exit(0);
}

 /* =============================================================== */

void
plot1(void)
{
    int i;
    PLFLT xmin, xmax, ymin, ymax;

    for (i = 0; i < 60; i++) {
	x[i] = xoff + xscale * (i + 1) / 60.0;
	y[i] = yoff + yscale * pow(x[i], 2.);
    }

    xmin = x[0];
    xmax = x[59];
    ymin = y[0];
    ymax = y[59];

    for (i = 0; i < 6; i++) {
	xs[i] = x[i * 10 + 3];
	ys[i] = y[i * 10 + 3];
    }

/* Set up the viewport and window using PLENV. The range in X is */
/* 0.0 to 6.0, and the range in Y is 0.0 to 30.0. The axes are */
/* scaled separately (just = 0), and we just draw a labelled */
/* box (axis = 0). */

    plcol(1);
    plenv(xmin, xmax, ymin, ymax, 0, 0);
    plcol(6);
    pllab("(x)", "(y)", "#frPLplot Example 1 - y=x#u2");

/* Plot the data points */

    plcol(9);
    plpoin(6, xs, ys, 9);

/* Draw the line through the data */

    plcol(4);
    plline(60, x, y);
    plflush();
}


 /* =============================================================== */

void
plot2(void)
{
    int i;

/* Set up the viewport and window using PLENV. The range in X is -2.0 to
       10.0, and the range in Y is -0.4 to 2.0. The axes are scaled separately
       (just = 0), and we draw a box with axes (axis = 1). */

    plcol(1);
    plenv(-2.0, 10.0, -0.4, 1.2, 0, 1);
    plcol(2);
    pllab("(x)", "sin(x)/x", "#frPLplot Example 1 - Sinc Function");

/* Fill up the arrays */

    for (i = 0; i < 100; i++) {
	x[i] = (i - 19.0) / 6.0;
	y[i] = 1.0;
	if (x[i] != 0.0)
	    y[i] = sin(x[i]) / x[i];
    }

/* Draw the line */

    plcol(3);
    plline(100, x, y);
    plflush();
}

 /* =============================================================== */

void
plot3(void)
{
    int i;

/* For the final graph we wish to override the default tick intervals, and
       so do not use PLENV */

    pladv(0);

/* Use standard viewport, and define X range from 0 to 360 degrees, Y range
       from -1.2 to 1.2. */

    plvsta();
    plwind(0.0, 360.0, -1.2, 1.2);

    /* Draw a box with ticks spaced 60 degrees apart in X, and 0.2 in Y. */

    plcol(1);
    plbox("bcnst", 60.0, 2, "bcnstv", 0.2, 2);

    /* Superimpose a dashed line grid, with 1.5 mm marks and spaces. plstyl
       expects a pointer!! */

    plstyl(1, &mark1, &space1);
    plcol(2);
    plbox("g", 30.0, 0, "g", 0.2, 0);
    plstyl(0, &mark0, &space0);

    plcol(3);
    pllab("Angle (degrees)", "sine", "#frPLplot Example 1 - Sine function");

    for (i = 0; i < 101; i++) {
	x[i] = 3.6 * i;
	y[i] = sin(x[i] * 3.141592654 / 180.0);
    }

    plcol(4);
    plline(101, x, y);
    plflush();
}

 /* =============================================================== */

void
plot4(void)
{
    int i, j;
    PLFLT dtr, theta, dx, dy, r;
    char text[3];
    PLFLT x0[361], y0[361];
    PLFLT x[361], y[361];

    dtr = 3.141592654 / 180.0;
    for (i = 0; i <= 360; i++) {
	x0[i] = cos(dtr * i);
	y0[i] = sin(dtr * i);
    }

/* Set up viewport and window, but do not draw box */

    plenv(-1.3, 1.3, -1.3, 1.3, 1, -2);
    for (i = 1; i <= 10; i++) {
	for (j = 0; j <= 360; j++) {
	    x[j] = 0.1 * i * x0[j];
	    y[j] = 0.1 * i * y0[j];
	}

/* Draw circles for polar grid */

	plline(361, x, y);
    }

    plcol(2);
    for (i = 0; i <= 11; i++) {
	theta = 30.0 * i;
	dx = cos(dtr * theta);
	dy = sin(dtr * theta);

/* Draw radial spokes for polar grid */

	pljoin(0.0, 0.0, dx, dy);
	sprintf(text, "%d", ROUND(theta));

/* Write labels for angle */

	if (dx >= 0)
	    plptex(dx, dy, dx, dy, -0.15, text);
	else
	    plptex(dx, dy, -dx, -dy, 1.15, text);
    }

/* Draw the graph */

    for (i = 0; i <= 360; i++) {
	r = sin(dtr * (5 * i));
	x[i] = x0[i] * r;
	y[i] = y0[i] * r;
    }
    plcol(3);
    plline(361, x, y);

    plcol(4);
    plmtex("t", 2.0, 0.5, 0.5,
	   "#frPLplot Example 3 - r(#gh)=sin 5#gh");
    plflush();
}

 /* =============================================================== */

/* Demonstration of contour plotting */

#define XPTS      35
#define YPTS      46
#define XSPA      2./(XPTS-1)
#define YSPA      2./(YPTS-1)

PLFLT tr[6] =
{XSPA, 0.0, -1.0, 0.0, YSPA, -1.0};

void
mypltr(PLFLT x, PLFLT y, PLFLT *tx, PLFLT *ty, void *pltr_data)
{
    *tx = tr[0] * x + tr[1] * y + tr[2];
    *ty = tr[3] * x + tr[4] * y + tr[5];
}

static PLFLT clevel[11] =
{-1., -.8, -.6, -.4, -.2, 0, .2, .4, .6, .8, 1.};

void
plot5(void)
{
    int i, j;
    PLFLT xx, yy;
    PLFLT **z, **w;
    static PLINT mark = 1500, space = 1500;

/* Set up function arrays */

    plAlloc2dGrid(&z, XPTS, YPTS);
    plAlloc2dGrid(&w, XPTS, YPTS);

    for (i = 0; i < XPTS; i++) {
	xx = (double) (i - (XPTS / 2)) / (double) (XPTS / 2);
	for (j = 0; j < YPTS; j++) {
	    yy = (double) (j - (YPTS / 2)) / (double) (YPTS / 2) - 1.0;
	    z[i][j] = xx * xx - yy * yy;
	    w[i][j] = 2 * xx * yy;
	}
    }

    plenv(-1.0, 1.0, -1.0, 1.0, 0, 0);
    plcol(2);
    plcont(z, XPTS, YPTS, 1, XPTS, 1, YPTS, clevel, 11, mypltr, NULL);
    plstyl(1, &mark, &space);
    plcol(3);
    plcont(w, XPTS, YPTS, 1, XPTS, 1, YPTS, clevel, 11, mypltr, NULL);
    plcol(1);
    pllab("X Coordinate", "Y Coordinate", "Streamlines of flow");
    plflush();
}
