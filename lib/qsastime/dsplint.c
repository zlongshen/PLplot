/*
  Copyright (C) 2009 Alan W. Irwin

  This file is part of PLplot.

  PLplot is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Library Public License as published
  by the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  PLplot is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with PLplot; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

  Provenance: This code was originally developed under the GPL as part of
  the FreeEOS project (revision 121).  This code has been converted from
  Fortran to C with the aid of f2c and relicensed for PLplot under the LGPL
  with the permission of the FreeEOS copyright holder (Alan W. Irwin).
*/

int dsplint_(doublereal *xa, doublereal *ya, doublereal *y2a,
	 integer *n, doublereal *x, doublereal *y, doublereal *dy, doublereal 
	*d2y)
{
    /* Initialized data */

    static integer nsave = 0;

    /* System generated locals */
    integer i__1, i__2;

    /* Builtin functions */
    /* Subroutine */ int s_stop(char *, ftnlen);

    /* Local variables */
    static doublereal a, b, h__;
    static integer k, khi, klo;

/*      evaluate spline = y and its derivatives dy and d2y at x given */
/*      xa, ya, y2a from dspline. */
    /* Parameter adjustments */
    --y2a;
    --ya;
    --xa;

    /* Function Body */
    if (*n != nsave) {
/*        if call with different n value, then redo range */
	nsave = *n;
	klo = 1;
	khi = *n;
	if (xa[klo] > *x) {
	    s_stop("dsplint: x too low", (ftnlen)18);
	}
	if (xa[khi] < *x) {
	    s_stop("dsplint: x too high", (ftnlen)19);
	}
    } else {
/*        optimize range assuming continuous (ascending or */
/*        descending x calls. */
	if (xa[klo] > *x) {
/*          x is descending so try next range. */
	    khi = max(2,klo);
	    klo = khi - 1;
/*          if x smaller than next range try lower limit. */
	    if (xa[klo] > *x) {
		klo = 1;
	    }
	    if (xa[klo] > *x) {
		s_stop("dsplint: x too low", (ftnlen)18);
	    }
	} else if (xa[khi] <= *x) {
/*          x is ascending so try next range. */
/* Computing MIN */
	    i__1 = khi, i__2 = *n - 1;
	    klo = min(i__1,i__2);
	    khi = klo + 1;
/*          if x larger than next range try upper limit. */
	    if (xa[khi] <= *x) {
		khi = *n;
	    }
	    if (xa[khi] < *x) {
		s_stop("dsplint: x too high", (ftnlen)19);
	    }
	}
    }
    while(khi - klo > 1) {
	k = (khi + klo) / 2;
	if (xa[k] > *x) {
	    khi = k;
	} else {
	    klo = k;
	}
    }
    h__ = xa[khi] - xa[klo];
    if (h__ <= 0.) {
	s_stop("dsplint: bad xa input.", (ftnlen)22);
    }
    a = (xa[khi] - *x) / h__;
    b = (*x - xa[klo]) / h__;
    *y = a * ya[klo] + b * ya[khi] + (a * (a * a - 1.) * y2a[klo] + b * (b * 
	    b - 1.) * y2a[khi]) * (h__ * h__) / 6.;
    *dy = (-ya[klo] + ya[khi] + (-(a * 3. * a - 1.) * y2a[klo] + (b * 3. * b 
	    - 1.) * y2a[khi]) * (h__ * h__) / 6.) / h__;
    *d2y = a * y2a[klo] + b * y2a[khi];
    return 0;
} /* dsplint_ */
