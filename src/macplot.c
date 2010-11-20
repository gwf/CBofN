/*MRM begin*/

/* Copyright (c) 1998 Michael R. Miller -- Permission granted for any use
 * provided that the author's comments are neither modified nor removed.
 * No warranty is given or implied.
 *
 * NAME
 *   macplot.c
 * PURPOSE
 *   Provides Macintosh plotting capabilities for the CBN
 *   C source code framework (color and gray-scale).
 */

#include "misc.h"

#define COLORS 256

#define WIND_RES 128

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Number of color levels, width, and height actually requested. */

int macplot_levels, macplot_width, macplot_height;

WindowPtr theWindow;
RGBColor theColors[COLORS];
const unsigned char *WIND_TITLE = (const unsigned char *) "\pCBN:  plotting...";
const unsigned char *WIND_TITLE_END = (const unsigned char *) "\pCBN:  done.  Click mouse to exit.";

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void initializeToolbox (void);
void initializeColors (Boolean color);
static void hsb_to_rgb(double hue, int *R, int *G, int *B);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void initializeToolbox (void) {
	InitGraf (&qd.thePort);
	InitFonts();
	InitWindows();
	InitMenus();
	TEInit();
	InitDialogs (0L);
	FlushEvents (everyEvent, 0);
	InitCursor();
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void initializeColors (Boolean color) {
	int i, R, G, B;
	double hue;

	if (color) {
		if(macplot_levels == 2) {
			for(i = 0; i < COLORS / 2; i++)
				theColors[i].red = theColors[i].green = theColors[i].blue = 0;
			for(i = COLORS / 2; i < COLORS; i++)
				theColors[i].red = theColors[i].green = theColors[i].blue = USHRT_MAX;
		} else {
			for(i = 0; i < COLORS; i++) {
				if(i > 0) {
					hue = i / (COLORS - 1.0);
					hsb_to_rgb (hue, &R, &G, &B);
					theColors[i].red = R;
					theColors[i].green = G;
					theColors[i].blue = B;
				}
				else if(macplot_levels >= 32)
					theColors[i].red = theColors[i].green = theColors[i].blue = 0;
				else if(macplot_levels < 32 && i == 0)
					theColors[i].red = theColors[i].green = theColors[i].blue = 0;
				else if(macplot_levels < 32 && i == COLORS - 1)
					theColors[i].red = theColors[i].green = theColors[i].blue = USHRT_MAX;
/*printf("%d: %u %u %u\n",i,theColors[i].red,theColors[i].green,theColors[i].blue);*/
			}
		}
	} else { /* gray-scale */
		if(macplot_levels == 2) {
			for(i = 0; i < COLORS / 2; i++)
				theColors[i].red = theColors[i].green = theColors[i].blue = 0;
			for(i = COLORS / 2; i < COLORS; i++)
				theColors[i].red = theColors[i].green = theColors[i].blue = USHRT_MAX;
		} else {
			for(i = 0; i < COLORS; i++) {
				if(i > 0) {
					hue = i / (COLORS - 1.0);
					theColors[i].red = theColors[i].green = theColors[i].blue =
						(unsigned short) (hue*USHRT_MAX);
				}
				else if(macplot_levels >= 32)
					theColors[i].red = theColors[i].green = theColors[i].blue = 0;
				else if(macplot_levels < 32 && i == 0)
					theColors[i].red = theColors[i].green = theColors[i].blue = 0;
				else if(macplot_levels < 32 && i == COLORS - 1)
					theColors[i].red = theColors[i].green = theColors[i].blue = USHRT_MAX;
/*printf("%d: %u %u %u\n",i,theColors[i].red,theColors[i].green,theColors[i].blue);*/
			}
		}
	}
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static double hue_shift(double x)
{
  return(x - floor(x));
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#define H2V(x) ((x)<1.0/6?6*(x):(x)<0.5?1.0:(x)<4.0/6?4-6*(x):0.0)

/* hue should go from 0 to 1. */

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static void hsb_to_rgb(double hue, int *R, int *G, int *B)
{
  double hueshift;

  hueshift = hue_shift(hue + 2.0/6);
  *R = (USHRT_MAX) * H2V(hueshift) + 0.5;
  hueshift = hue_shift(hue);
  *G = (USHRT_MAX) * H2V(hueshift) + 0.5;
  hueshift = hue_shift(hue - 2.0/6);
  *B = (USHRT_MAX) * H2V(hueshift) + 0.5;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void MacOS_plot_init (int width, int height, int levels, Boolean color)
{
  levels = (levels > COLORS) ? COLORS : levels;
  macplot_levels = levels;
  macplot_width = width;
  macplot_height = height;
  
  initializeToolbox();
  initializeColors (color);
  theWindow = GetNewCWindow (WIND_RES, nil, (WindowPtr) -1L);
  
  SizeWindow (theWindow, macplot_width*plot_mag, macplot_height*plot_mag, true);
  SetWTitle (theWindow, WIND_TITLE);
  ShowWindow (theWindow);
  SetPort (theWindow);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void macplot_init(int width, int height, int levels)
{
	MacOS_plot_init (width, height, levels, false);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void Macplot_init(int width, int height, int levels)
{
	MacOS_plot_init (width, height, levels, true);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void Macplot_point(int i, int j, int val)
{
	int cval, ii, jj;
	Rect r;
	
	val = (val < 0) ? 0 : (val >= macplot_levels) ? macplot_levels - 1: val;
	cval = ((double)val / (macplot_levels - 1)) * (COLORS - 1) + 0.5;
	RGBForeColor (&theColors[cval]);
	
	for (ii = 0; ii < plot_mag; ii++)
		for (jj = 0; jj < plot_mag; jj++) {
			SetRect (&r, i*plot_mag+ii, j*plot_mag+jj, i*plot_mag+ii+1, j*plot_mag+jj+1);
			PaintRect (&r);
		}
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void Macplot_line(int i1, int j1, int i2, int j2, int val)
{
	int cval, ii, jj;
	
	val = (val < 0) ? 0 : (val >= macplot_levels) ? macplot_levels : val;
	cval = ((double)val / macplot_levels) * (COLORS - 1) + 0.5;
	RGBForeColor (&theColors[cval]);

	if (plot_mag == 1) {
		MoveTo (i1, j1);
		LineTo (i2, j2);
	} else {
		double tx, ty, t, dt;
		int len;

		if ((i1 == i2) && (j1 == j2)) {
			Macplot_point (i1, j1, val);
		} else {
			len = MAX(ABS(i1 - i2), ABS(j1 - j2));
			dt = 1.0 / len;
			for (ii = 0, t = 0.0; ii < len + 1; ii++, t += dt) {
				tx = t * i1 + (1.0 - t) * i2 + 0.5;
				ty = t * j1 + (1.0 - t) * j2 + 0.5;
				Macplot_point (tx, ty, val);
			}
		}
	}
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void Macplot_finish(void)
{
	SetWTitle (theWindow, WIND_TITLE_END);
	
	while (!Button()) { }
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*MRM end*/
