/* $Id$
   $Log$
   Revision 1.2  1993/02/23 04:58:33  mjl
   Added to the stream data structure: a device name, and the value of the
   escape character.  Added function prototype for plgpls and some minor
   documentation changes.

 * Revision 1.1  1993/01/23  05:39:16  mjl
 * Added to hold definition of PLStream data structure.  Included by all files
 * dealing with streams (and is included by driver.h).
 *
*/

/*	plstream.h

	Contains declarations for PLStream and PLDev structs.
	Also prototypes for stream & device utility functions.
*/

#ifndef __PLSTREAM_H__
#define __PLSTREAM_H__

/*----------------------------------------------------------------------*\
*  Define the PLDev data structure.
*
*  These are all quantities that must be saved on a per-device basis.
\*----------------------------------------------------------------------*/

typedef struct {
    PLFLT pxlx, pxly;
    PLINT xold, yold;
    PLINT xmin, xmax, xlen;
    PLINT ymin, ymax, ylen;
    PLINT sclxmin, sclxmax;
    PLINT sclymin, sclymax;
    PLFLT rsclx, rscly;
} PLDev;

/*----------------------------------------------------------------------*\
* Define the PLStream data structure.
*
* This contains a copy of every variable that is stream dependent, which
* tends to be those which are settable by the user and persist for longer
* than one plot.
*
* Only those quantities listed in the PLStream struct will be preserved
* for an individual stream.  Note that the lack of every plplot constant
* means that one should only switch streams at a fairly high level, i.e.
* on the scale of an entire plot.  Otherwise intermediate quantities
* will be confused between the streams, possibly resulting in garbage plots.
* This structure may be expanded in time to improve intra-stream independence,
* but it is doubtful streams will ever be fully independent (perhaps
* neither necessary nor desirable? -- time will tell).
*
* There are undoubtably some inconsistencies in the declaration & use of
* the variables below.  This is a result of the continuing evolution of
* plplot and the numerous authors involved.  Hopefully in time the function
* of all variables can be fully documented and made more consistent.
*
* The quantities in each stream are as follows:
*
* ipls		Stream number
* ipld		Device number
* DevName	Device name
*
* OutFile	Output file pointer
* FamilyName	Output family name (i.e. stem)
* FileName	Output file name
*
* plbuf_enable	Set if driver needs to use the plot buffer
* plbuf_read	Set if reading from the plot buffer
* plbuf_write	Set if writing to the plot buffer
* plbufFile	Plot buffer file pointer
* plbufFnam	Plot buffer file name
*
* orientset	Set if orientation was specified
* fileset	Set if file name or file pointer was specified
* pageset	Set if page dimensions were specified
* widthset	Set if pen width was specified
* aspectset	Set if the aspect ratio was specified
*
* bytecnt	Byte count for output stream
* page		Page count for output stream
* linepos	Line count for output stream
* lp_offset	Byte position for previous page (metafile output)
*
***********************************************************************
* PLPLOT uses two user-alterable palettes:
*
* Color map 0 is intended for static objects, such as boxes, lines, points,
* labels, etc.  These should be allocated before calling plstar (else you
* get all 16 by default, which can be undesirable on some platforms).
* These are then explicitly selected by number (in order of allocation,
* starting with 1).  Color 0 is reserved for the background color.
*
* Color map 1 is for height-field plots, where color is used to represent
* function height (intensity).  These are set in a relative way only, 
* for increased portability and flexibility.  The actual number of colors
* used to represent height levels is determined by the driver.  Note that
* it is only necessary to specify height (on a scale from 0 to 1) to get
* an appropriate color.  Drivers capable of finer shading can interpolate
* RGB values to precisely map height to color.
*
* Eventually a palette selection tool for both palettes will be provided
* for some drivers (e.g. Motif).  Direct writing of RGB values (i.e.
* banging on the hardware) is supported but highly discouraged (colors so
* written will be invisible to the palette tools).
*
* color		PLINT	Set if color is available
* icol0		PLINT	Color map 0 entry, current color (0 <= icol0 <= 15)
* ncol0		PLINT	Number of colors allocated in color map 0.
* htlvl		PLINT	Color map 1 current height level (0.0 <= htlvl <= 1.0)
* fgcolor	RGB	Foreground color, if specified
* curcolor	RGB[]	Current color, for convenience
* cmap0 	RGB[]	Color map 0: maximum of 16 RGB 8-bit values
* cmap1 	RGB[]	Color map 1: maximum of 256 RGB 8-bit values
* cmap0setcol	int[]	Set for every color allocated in cmap0.
* cmap1set	PLINT	Set if cmap1 allocated.
*
***********************************************************************
*
* The X-based drivers support a user-supplied event handler, that
* can be used to take various actions depending on input.  Currently
* only a keyboard event handler is supported.
*
* KeyEH		(void *) Keyboard event handler
* KeyEH_data	(void *) pointer to client data to pass
*
***********************************************************************
*
* level		Initialization level
* device	Graphics device id number
* termin	Zero for noninteractive device, non-zero for terminal
* graphx	Zero if currently in text mode, non-zero for graphics mode
* nopause	Zero if we are pausing between frames
* family	Non-zero if familying is enabled
* member	Number of current family member file open
* bytemax	Number of bytes maximum per member file
* famadv        Non-zero to advance to the next family member
* nsub...	Number of subpages on physical device
* cursub	Current subpage
* width		Current pen width
* esc		Text string escape character
* scale		Scaling factor for chr, sym, maj, min.
* chr...	Character default height and current (scaled) height
* sym...	Symbol    default height and current (scaled) height
* maj...	Major tick default height and current (scaled) height
* min...	Minor tick default height and current (scaled) height
* setpre	Non-zero to set precision using "prec"
* precis	User-specified precision
* xdigmax..	Allowed #digits in axes labels
* xdigits..	Actual field widths (returned)
*
* pscale 	Physical coordinate scaling flag
* aspect	Global aspect ratio
* lpbp..	Local plot boundaries in physical coordinates
* vpp..		Viewport boundaries in physical coordinates
* spp..		Subpage  boundaries in physical coordinates
* clp..		Clip     boundaries in physical coordinates
* phy...	Physical device limits in physical coordinates
* um.		Number of micrometres in a pixel
*
* currx		Physical x-coordinate of current point
* curry		Physical y-coordinate of current point
* mark		Array of mark lengths in micrometres for broken lines
* space		Array of space lengths in micrometres for broken lines
* nms		Number of elements for current broken line style
* timecnt	Timer for broken lines
* alarm		Alarm indicating change of broken line status
* pendn		Flag indicating if pen is up or down
* curel		Current element within broken line
* inclin	Array of inclinations in tenths of degree for fill lines
* delta		Array of spacings in micrometers between fill lines
* nps		Number of distinct line styles for fills
*
* xdpi..	Device DPI settings in x and y (if supported)
* xlength..	Device output lengths in x and y (if supported)
* xoffset..	Device offsets from upper left hand corner (if supported)
*
* spd...	Subpage  boundaries in normalized device coordinates
* vpd...	Viewport boundaries in normalized device coordinates
* vpw...	Viewport boundaries in world coordinates
* pmm		Number of pixels to a millimetre
* wp....	Transformation variables for world  to physical conversion
* dp....	Transformation variables for device to physical conversion
* mp....	Transformation variables for millimetres from bottom left
*		hand corner to physical coordinates
* wm....	Transformation variables for world coordinates to mm
* base3.	World coordinate size of base for 3-d plot
* basec.	Position of centre of base for 3-d plot
* dom...	Minimum and maximum values for domain
* zzscl		Vertical (z) scale for 3-d plot
* ran..		Minimum and maximum z values for 3-d plot
* c..		Coordinate transformation from 3-d to 2-d
*
\*----------------------------------------------------------------------*/

typedef struct {
    PLINT ipls, ipld;
    char DevName[80];

    FILE *OutFile;
    char FamilyName[80], FileName[90];

    int		plbuf_enable;
    int		plbuf_read;
    int		plbuf_write;
    FILE	*plbufFile;
    char	plbufFnam[90];

    PLINT orientset;
    PLINT fileset;
    PLINT pageset;
    PLINT widthset;
    PLINT aspectset;

    PLINT bytecnt;
    PLINT page;
    PLINT linepos;
    U_LONG lp_offset;

/* Colormaps */

    PLINT color, colorset, icol0, ncol0, bgcolset;
    PLFLT htlvl;
    int   cmap0setcol[16], cmap1set;

    PLColor fgcolor;
    PLColor curcolor;
    PLColor cmap0[16];
    PLColor cmap1[256];

/* Event handler */

    void (*KeyEH)	(PLKey *, void *, int *);
    void *KeyEH_data;

/* Could use some logical grouping here! */

    PLINT level;
    PLINT device, termin, graphx;
    PLINT nopause;
    PLINT family, member, bytemax, famadv;
    PLFLT sclx, scly;
    PLINT nsubx, nsuby, cursub;
    PLINT width, style;
    PLINT orient;
    char  esc;
    PLFLT xdpi, ydpi;
    PLINT xlength, ylength;
    PLINT xoffset, yoffset;
    PLINT setpre, precis;
    PLINT xdigmax, ydigmax, zdigmax;
    PLINT xdigits, ydigits, zdigits;
    PLFLT scale;
    PLFLT chrdef, chrht;
    PLFLT symdef, symht;
    PLFLT majdef, majht;
    PLFLT mindef, minht;
    PLINT pscale;
    PLFLT aspect;
    PLINT vppxmi, vppxma, vppymi, vppyma;
    PLINT sppxmi, sppxma, sppymi, sppyma;
    PLINT clpxmi, clpxma, clpymi, clpyma;
    PLINT phyxmi, phyxma, phyymi, phyyma;
    PLINT lpbpxmi, lpbpxma, lpbpymi, lpbpyma;
    PLINT umx, umy;
    PLINT currx, curry;
    PLINT mark[10], space[10], nms;
    PLINT timecnt, alarm, pendn, curel;
    PLINT inclin[2], delta[2], nps;

    PLFLT spdxmi, spdxma, spdymi, spdyma;
    PLFLT vpdxmi, vpdxma, vpdymi, vpdyma;
    PLFLT vpwxmi, vpwxma, vpwymi, vpwyma;
    PLFLT xpmm, ypmm;
    PLFLT wpxscl, wpxoff, wpyscl, wpyoff;
    PLFLT dpxscl, dpxoff, dpyscl, dpyoff;
    PLFLT mpxscl, mpxoff, mpyscl, mpyoff;
    PLFLT wmxscl, wmxoff, wmyscl, wmyoff;
    PLFLT base3x, base3y, basecx, basecy;
    PLFLT domxmi, domxma, domymi, domyma;
    PLFLT zzscl, ranmi, ranma;
    PLFLT cxx, cxy, cyx, cyy, cyz;
} PLStream;

/*----------------------------------------------------------------------*\
* Prototypes for stream & device utility functions.
\*----------------------------------------------------------------------*/

void  plgpls		(PLStream *);
void  plCmaps_init	(PLStream *);
void  plOpenFile	(PLStream *);
void  plFamInit		(PLStream *);
PLINT plGetInt		(char *);
PLFLT plGetFlt		(char *);
void  plGetFam		(PLStream *);
void  plSclPhy		(PLStream *, PLDev *, 
				int *, int *, int *, int *);
void  plRotPhy		(PLINT, PLDev *, 
				int *, int *, int *, int *);

#endif	/* __PLSTREAM_H__ */
