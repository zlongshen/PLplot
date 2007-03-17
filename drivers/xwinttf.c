/* March 13, 2007

   PLplot driver for X Windows that uses the Cairo graphics library for
   rendering graphics and Pango for rendering text.
 
   Copyright (C) 2007 Hazen Babcock

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
   	
*/

//---------------------------------------------
// Header files, defines and local variables
//---------------------------------------------

#include <stdio.h>
#include <string.h>
#include <math.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <cairo.h>
#include <cairo-xlib.h>

#include <pango/pangocairo.h>

/* PLplot header files */

#include "plplotP.h"
#include "drivers.h"

/* constants */

#define DPI 72
#define XWINTTF_DEFAULT_X 720
#define XWINTTF_DEFAULT_Y 540
#define MAX_STRING_LEN 500
#define MAX_MARKUP_LEN MAX_STRING_LEN * 10

/* local variables */

char* plD_DEVICE_INFO_xwinttf = "xwinttf:TrueType X Windows Driver:1:xwinttf:59:xwinttf";

static int XScreen;
static Display *XDisplay = NULL;
static Window rootWindow ;
static Window XWindow;

static int windowXSize = XWINTTF_DEFAULT_X;
static int windowYSize = XWINTTF_DEFAULT_Y;

static cairo_surface_t *cairoSurface;
static cairo_t *cairoContext;

//-----------------------------------------------
// Font style and weight lookup tables (copied
// from the psttf driver).
//-----------------------------------------------

#define NPANGOLOOKUP 5

const char *defaultFamilyLookup[NPANGOLOOKUP] = {
  "Arial,Bitstream-Vera-Sans,sans",
  "Times-Roman,Bitstream-Vera-Serif,serif",
  "Courier,Bitstream-Vera-Sans-Mono,monospace",
  "Arial,Bitstream-Vera-Sans,sans,serif",
  "Arial,Bitstream-Vera-Sans,sans,serif"
};

const char *envFamilyLookup[NPANGOLOOKUP] = {
  "PLPLOT_FREETYPE_SANS_FAMILY",
  "PLPLOT_FREETYPE_SERIF_FAMILY",
  "PLPLOT_FREETYPE_MONO_FAMILY",
  "PLPLOT_FREETYPE_SCRIPT_FAMILY",
  "PLPLOT_FREETYPE_SYMBOL_FAMILY"
};

char familyLookup[NPANGOLOOKUP][1024];

const char *weightLookup[2] = {
  "normal",
  "bold"
};

const char *styleLookup[3] = {
  "normal",
  "italic",
  "oblique"
};

//-----------------------------------------------
// function declarations
//-----------------------------------------------

/* String processing */

static void proc_str(PLStream *, EscText *);
static char *ucs4_to_pango_markup_format(PLUNICODE *, int, float);

/* Graphics */

static void set_current_context(PLStream *);
static void poly_line(PLStream *, short *, short *, PLINT);

/* PLplot interface functions */

void plD_dispatch_init_xwinttf      (PLDispatchTable *pdt);
void plD_init_xwinttf               (PLStream *);
void plD_line_xwinttf               (PLStream *, short, short, short, short);
void plD_polyline_xwinttf   	    (PLStream *, short *, short *, PLINT);
void plD_eop_xwinttf                (PLStream *);
void plD_bop_xwinttf                (PLStream *);
void plD_tidy_xwinttf               (PLStream *);
void plD_state_xwinttf              (PLStream *, PLINT);
void plD_esc_xwinttf                (PLStream *, PLINT, void *);

//---------------------------------------------------------------------
// dispatch_init_init()
//
// Initialize device dispatch table
//----------------------------------------------------------------------

void plD_dispatch_init_xwinttf(PLDispatchTable *pdt)
{
#ifndef ENABLE_DYNDRIVERS
   pdt->pl_MenuStr  = "TrueType X Windows Driver";
   pdt->pl_DevName  = "xwinttf";
#endif
   pdt->pl_type     = plDevType_Interactive;
   pdt->pl_seq      = 59;
   pdt->pl_init     = (plD_init_fp)     plD_init_xwinttf;
   pdt->pl_line     = (plD_line_fp)     plD_line_xwinttf;
   pdt->pl_polyline = (plD_polyline_fp) plD_polyline_xwinttf;
   pdt->pl_eop      = (plD_eop_fp)      plD_eop_xwinttf;
   pdt->pl_bop      = (plD_bop_fp)      plD_bop_xwinttf;
   pdt->pl_tidy     = (plD_tidy_fp)     plD_tidy_xwinttf;
   pdt->pl_state    = (plD_state_fp)    plD_state_xwinttf;
   pdt->pl_esc      = (plD_esc_fp)      plD_esc_xwinttf;
}

//---------------------------------------------------------------------
// xwinttf_init()
//
// Initialize device
//----------------------------------------------------------------------

void plD_init_xwinttf(PLStream *pls)
{
  int i;
  char *a;

  pls->termin = 1;			/* interactive device */
  pls->color = 1;			/* supports color */
  pls->width = 1;
  pls->verbose = 1;
  pls->bytecnt = 0;
  pls->debug = 1;
  pls->dev_text = 1;		/* handles text */
  pls->dev_unicode = 1; 	/* wants text as unicode */
  pls->page = 0;
  pls->dev_fill0 = 1;		/* supports hardware solid fills */
  pls->dev_fill1 = 1;
  
  pls->graphx = GRAPHICS_MODE;
  
  if (!pls->colorset) pls->color = 1;
  
  plP_setpxl(DPI/25.4, DPI/25.4);
  
  if (pls->xlength <= 0 || pls->ylength <= 0){
    plP_setphy((PLINT) 0, (PLINT) XWINTTF_DEFAULT_X, (PLINT) 0, XWINTTF_DEFAULT_Y);
  } else {
    windowXSize = pls->xlength;
    windowYSize = pls->ylength;
    plP_setphy((PLINT) 0, (PLINT) pls->xlength, (PLINT) 0, (PLINT) pls->ylength);
  }

  XDisplay = XOpenDisplay(NULL);
  if(XDisplay == NULL){
    // some sort of error here
  }
  XScreen = DefaultScreen(XDisplay);
  rootWindow = RootWindow(XDisplay, XScreen);

  // Initialize font table with either enviroment variables or defaults.
  // This was copied from the psttf driver.
  for(i=0;i<NPANGOLOOKUP;i++){
    if((a = getenv(envFamilyLookup[i])) != NULL){
      strcpy(familyLookup[i],a);
    }
    else {
      strcpy(familyLookup[i],defaultFamilyLookup[i]);
    }
  }
}

//----------------------------------------------------------------------
// xwinttf_bop()
//
// Set up for the next page.
//----------------------------------------------------------------------

void plD_bop_xwinttf(PLStream *pls)
{
  cairo_matrix_t *flipVertical;
  Visual *defaultVisual;
  XGCValues values;
  
  // Create a X Window.
  XWindow = XCreateSimpleWindow(XDisplay, rootWindow, 0, 0, windowXSize, windowYSize, 
				1, BlackPixel(XDisplay, XScreen), BlackPixel(XDisplay, XScreen));
  XStoreName(XDisplay, XWindow, "PLplot");
  XSelectInput(XDisplay, XWindow, ExposureMask|ButtonPressMask);
  XMapWindow(XDisplay, XWindow);

  // Create an cairo surface & context that are associated with the window.
  defaultVisual = DefaultVisual(XDisplay, 0);
  cairoSurface = cairo_xlib_surface_create(XDisplay, XWindow, defaultVisual, windowXSize, windowYSize);
  cairoContext = cairo_create(cairoSurface);

  // Fill in the window with the background color.
  cairo_rectangle(cairoContext, 0.0, 0.0, windowXSize, windowYSize);
  cairo_set_source_rgb(cairoContext,
		       pls->cmap0[0].r,
		       pls->cmap0[0].g,
		       pls->cmap0[0].b);
  cairo_fill(cairoContext);

  // Invert the coordinate system so the graphs are drawn right side up.
  flipVertical = (cairo_matrix_t *) malloc (sizeof(cairo_matrix_t));
  cairo_matrix_init(flipVertical, 1.0, 0.0, 0.0, -1.0, 0.0, windowYSize);
  cairo_set_matrix(cairoContext, flipVertical);
  free(flipVertical);

  // Increment the page number.
  pls->page++;
}

//---------------------------------------------------------------------
// xwinttf_line()
//
// Draw a line in the current color from (x1,y1) to (x2,y2).
//----------------------------------------------------------------------

void plD_line_xwinttf(PLStream *pls, short x1a, short y1a, short x2a, short y2a)
{
  set_current_context(pls);

  cairo_move_to(cairoContext, (double) x1a, (double) y1a);
  cairo_line_to(cairoContext, (double) x2a, (double) y2a);
  cairo_stroke(cairoContext);
}

//---------------------------------------------------------------------
// xwinttf_polyline()
//
// Draw a polyline in the current color.
//---------------------------------------------------------------------

void plD_polyline_xwinttf(PLStream *pls, short *xa, short *ya, PLINT npts)
{
  poly_line(pls, xa, ya, npts);
  cairo_stroke(cairoContext);
}

//---------------------------------------------------------------------
// xwinttf_eop()
//
// End of page
//---------------------------------------------------------------------

void plD_eop_xwinttf(PLStream *pls)
{
  XFlush(XDisplay);
}

//---------------------------------------------------------------------
// xwinttf_tidy()
//
// Close graphics file or otherwise clean up.
//---------------------------------------------------------------------

void plD_tidy_xwinttf(PLStream *pls)
{
  printf("Key <Return> to finish\n");
  getc(stdin);

  cairo_destroy(cairoContext);
  cairo_surface_destroy(cairoSurface);
  XCloseDisplay(XDisplay);
  free(XDisplay);
}

//---------------------------------------------------------------------
// plD_state_xwinttf()
//
// Handle change in PLStream state (color, pen width, fill attribute, etc).
//
// Nothing is done here because these attributes are aquired from 
// PLStream for each element that is drawn.
//---------------------------------------------------------------------

void plD_state_xwinttf(PLStream *pls, PLINT op)
{
}

//---------------------------------------------------------------------
// xwinttf_esc()
//
// Escape function.
//---------------------------------------------------------------------

void plD_esc_xwinttf(PLStream *pls, PLINT op, void *ptr)
{
  switch(op)
    {
    case PLESC_FILL:  // filled polygon
      poly_line(pls, pls->dev_x, pls->dev_y, pls->dev_npts);
      cairo_fill(cairoContext);
      break;
    case PLESC_HAS_TEXT: // render rext
      proc_str(pls, (EscText *) ptr);
      break;
    case PLESC_FLUSH: // forced update of the window
      XFlush(XDisplay);
      break;
    }
}

//---------------------------------------------------------------------
// proc_str()
//
// Processes strings for display.
//---------------------------------------------------------------------

void proc_str(PLStream *pls, EscText *args)
{
  int i;
  float fontSize;
  PLUNICODE fci;
  unsigned char fontFamily, fontStyle, fontWeight;
  int textXExtent, textYExtent;
  char fontString[200];
  char *textWithPangoMarkup;
  PLFLT rotation, shear;
  PLFLT *plplotTransformMatrix = args->xform;
  cairo_matrix_t *cairoTransformMatrix;
  PangoLayout *layout;
  PangoFontDescription *fontDescription;

  set_current_context(pls);

  // Check that we got unicode, warning message and return if not
  if(args->unicode_array_len == 0){
    printf("Non unicode string passed to xwinttf driver, ignoring\n");
    return;
  }
	
  // Check that unicode string isn't longer then the max we allow	
  if(args->unicode_array_len >= MAX_STRING_LEN){
    printf("Sorry, the xwinttf driver only handles strings of length < %d\n", MAX_STRING_LEN);
    return;
  }

  // Calculate the font size (in pixels)
  fontSize = pls->chrht * DPI/25.4;

  // Determine the font family, style and weight
  plgfci(&fci);
  plP_fci2hex(fci, &fontFamily, PL_FCI_FAMILY);
  plP_fci2hex(fci, &fontStyle, PL_FCI_STYLE);
  plP_fci2hex(fci, &fontWeight, PL_FCI_WEIGHT);
  for(i=0;i<200;i++){
    fontString[i] = 0;
  }
  sprintf(fontString, "%s,%s,%s", familyLookup[fontFamily], styleLookup[fontStyle], weightLookup[fontWeight]);

  // Convert the escape characters into the appropriate Pango markup
  textWithPangoMarkup =  ucs4_to_pango_markup_format(args->unicode_array, args->unicode_array_len, fontSize);

  // Create the Pango text layout so we can figure out how big it is
  layout = pango_cairo_create_layout(cairoContext);
  pango_layout_set_markup(layout, textWithPangoMarkup, -1);
  fontDescription = pango_font_description_from_string(fontString);
  pango_layout_set_font_description(layout, fontDescription);
  pango_layout_get_pixel_size(layout, &textXExtent, &textYExtent);

  // Move to the string reference point
  cairo_move_to(cairoContext, (double) args->x, (double) args->y);

  // Save current transform matrix
  cairo_save(cairoContext);

  // Invert the coordinate system so that the text is drawn right side up
  cairoTransformMatrix = (cairo_matrix_t *) malloc (sizeof(cairo_matrix_t));
  cairo_matrix_init(cairoTransformMatrix, 1.0, 0.0, 0.0, -1.0, 0.0, 0.0);
  cairo_transform(cairoContext, cairoTransformMatrix);

  // Extract rotation angle and shear from the PLplot tranformation matrix.
  // NB: The "transformation" matrix only defines the rotation and shear.
  //     Why do we refer to it as a tranformation matrix?

  rotation = acos(plplotTransformMatrix[0]);
  if(plplotTransformMatrix[2] < 0.0){
    rotation = -rotation;
  }

  shear = -asin(plplotTransformMatrix[0]*plplotTransformMatrix[1] +
		plplotTransformMatrix[2]*plplotTransformMatrix[3]);

  // Apply the transform matrix
  cairo_matrix_init(cairoTransformMatrix,
		    cos(rotation),
		    -sin(rotation),
		    cos(rotation) * sin(shear) + sin(rotation),
		    -sin(rotation) * sin(shear) + cos(rotation),
		    0,0);
  cairo_transform(cairoContext, cairoTransformMatrix);
  free(cairoTransformMatrix);

  // Move to the text starting point
  cairo_rel_move_to(cairoContext, 
		    (double)(-1.0 * args->just * (double)textXExtent), 
		    (double)(-0.5 * textYExtent));

  // Render the text
  pango_cairo_show_layout(cairoContext, layout);

  // Restore the transform matrix to its state prior to the text transform
  cairo_restore(cairoContext);
  
  // Free the layout object & the text
  g_object_unref(layout);
  free(textWithPangoMarkup);
}

//---------------------------------------------------------------------
// ucs4_to_pango_markup_format()
//
// Converts the plplot string (in ucs4) to a utf8 string that includes
// pango markup.
//
// http://developer.gnome.org/doc/API/2.0/pango/PangoMarkupFormat.html
//---------------------------------------------------------------------

char *ucs4_to_pango_markup_format(PLUNICODE *ucs4, int ucs4Len, float fontSize)
{
  char plplotEsc;
  int i;
  int upDown = 0;
  PLUNICODE fci;
  char utf8[5];
  char *pangoMarkupString;

  // Will this be big enough? We might have lots of markup.
  pangoMarkupString = (char *) malloc (sizeof(char) * MAX_MARKUP_LEN);
  for(i = 0; i < MAX_MARKUP_LEN; i++){
    pangoMarkupString[i] = 0;
  }

  // Get PLplot escape character
  plgesc(&plplotEsc);

  // Set the font size
  sprintf(pangoMarkupString, "<span font_desc=\"%.2f\">", fontSize);

  // Parse the string to generate the tags
  i = 0;
  while (i < ucs4Len){
    // Try to avoid going off the end of the string
    if(strlen(pangoMarkupString) > (MAX_MARKUP_LEN - 50)){
      continue;
    }
    if (ucs4[i] < PL_FCI_MARK){	// not a font change
      if (ucs4[i] != (PLUNICODE)plplotEsc) {  // a character to display
	ucs4_to_utf8(ucs4[i],utf8);
	strcat(pangoMarkupString, utf8);
	i++;
	continue;
      }
      i++;
      if (ucs4[i] == (PLUNICODE)plplotEsc){   // a escape character to display
	strcat(pangoMarkupString, utf8);
	i++;
	continue;
      }
      else {
	if(ucs4[i] == (PLUNICODE)'u'){	// Superscript
	  if(upDown < 0){
	    strcat(pangoMarkupString, "</sub>");
	  } else {
	    strcat(pangoMarkupString, "<sup>");
	  }
	  upDown++;
	}
	if(ucs4[i] == (PLUNICODE)'d'){	// Subscript
	  if(upDown > 0){
	    strcat(pangoMarkupString, "</sup>");
	  } else {
	    strcat(pangoMarkupString, "<sub>");
	  }
	  upDown--;
	}
	i++;
      }
    }
    else { // a font change
      // FIXME: These are ignored
      i++;
    }
  }

  // Close out sub and superscript tags
  if(upDown > 0){
    while(upDown > 0){
      strcat(pangoMarkupString, "</sup>");
      upDown--;
    }
  }
  if(upDown < 0){
    while(upDown < 0){
      strcat(pangoMarkupString, "</sub>");
      upDown++;
    }
  }

  strcat(pangoMarkupString, "</span>");

  //  printf("%s\n", pangoMarkupString);

  return pangoMarkupString;
}
				
//---------------------------------------------------------------------
// set_current_context()
//
// Updates the cairo graphics context with the current values in
// PLStream.
//---------------------------------------------------------------------

void set_current_context(PLStream *pls)
{
  cairo_set_source_rgb(cairoContext, 
		       (double)pls->curcolor.r/255.0, 
  		       (double)pls->curcolor.g/255.0,
  		       (double)pls->curcolor.b/255.0);
  cairo_set_line_width(cairoContext, (double) pls->width);
}

//---------------------------------------------------------------------
// poly_line()
//
// Draws a multi-segmented line. It is then up to the calling function
// to decide whether to just draw the line, or fill in the area
// enclosed by the line.
//---------------------------------------------------------------------

void poly_line(PLStream *pls, short *xa, short *ya, PLINT npts)
{
  int i;

  set_current_context(pls);
  
  cairo_move_to(cairoContext, (double) xa[0], (double) ya[0]);
  for(i=1;i<npts;i++){
    cairo_line_to(cairoContext, (double) xa[i], (double) ya[i]);
  }
}
