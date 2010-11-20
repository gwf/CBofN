
/* Copyright (c) 1995 Gary W. Flake -- Permission granted for any use
 * provied that the author's comments are neither modified nor removed.
 * No warranty is given or implied.
 *
 * NAME
 *   misc.h
 * PURPOSE
 *   Miscellaneous routines for program examples.  Most of the programs
 *   will need to be linked to this code.
 */

/* Insures that this file is only loaded once. */

#ifndef __MISC_H__
#define __MISC_H__

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

#ifdef sun
long lrand48();
void srand48(long);
#define random lrand48
#define RANDOM_MAX (LONG_MAX)
#define srandom srand48
#else
#ifndef RAND_MAX
#define RAND_MAX (INT_MAX)
#endif
#endif

#ifdef WIN32
#define inline 
#define random rand
#define srandom srand
#ifndef  __cplusplus
#define exit(val) \
  do { \
        int i = getchar(); \
        _exit(val); \
  } while(0)
#define M_PI 3.14159265358979323846
#endif
#endif

/*MRM begin*/
#if __dest_os == __mac_os
#include <string.h>
#include <math.h>
#include <console.h>
#define random rand
#define srandom srand
#define M_PI 3.14159265358979323846
#endif
/*MRM end*/

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Command line option routines... */

/* Possible command line option types.  OPT_NULL is a tag to indicate
   the end of options. */

typedef enum OPT_TYPE {
  OPT_INT, OPT_DOUBLE, OPT_STRING, OPT_SWITCH, OPT_OTHER, OPT_NULL
} OPT_TYPE;


/* An option consists of a name, a type enum, and a ptr to either the
   location of the storage or a ptr to a special user supplied function
   that partially parses the line for that one type. */

typedef struct OPTION {
  char *name;
  OPT_TYPE type;
  void *ptr;
  char *help;
} OPTION;


/* Function to automagically parse command line arguments. */

void get_options(int argc, char **argv, OPTION *options, char *help);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Input scanner routines... */

/* The scanner type makes it possible for all of the scanner routines to
   be reentrant.  This is not srtictly required for the sample code, but
   it is good programming style. */

typedef struct SCANNER {
  char *buffer, *ptr, *token;
  char *delims, *whites, *comments;
  FILE *fp;
} SCANNER;

#define MAXSCANLINELEN 256

/* Functions for scanning input files. */

SCANNER *scan_init(FILE *fp, char *delimiters, char *whites, char *comments);
char    *scan_get(SCANNER *scanner);
char    *scan_peek(SCANNER *scanner);
void     scan_flush(SCANNER *scanner);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Prototypes for plot routines that are device independent. */

void plot_init(int width, int height, int levels, char *term);
void plot_point(double x, double y, int val);
void plot_set_range(double xmin, double xmax, double ymin, double ymax);
void plot_set_all(int val);
void plot_box(double ulx, double uly, double lrx, double lry, int lwidth);
void plot_line(double x1, double y1, double x2, double y2, int val);
void plot_finish(void);

extern int plot_inverse, plot_mag;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Miscelaneous things... */

/* Random number functions. */

double random_range(double low, double high);
double random_gauss(void);


/* Function to get memory with check for failure built in. */

void *xmalloc(size_t bytes);


/* Function to read ascii pbm files. */

int **read_pbm_file(char *fname, int *w, int *h);

/* Miscelaneous macros. */

#define MIN(x, y)     ((x) < (y) ? (x) : (y))
#define MAX(x, y)     ((x) > (y) ? (x) : (y))
#define ABS(x)        ((x) >= 0 ? (x) : -(x))
#define SIGN(x)       ((x) >= 0 ? 1 : -1)
#define SQR(x)        ((x)*(x))
#define GETBIT(c,i)   (((1 << (i)) & (c)) ? 1 : 0)
#define SETBIT(c,i,b) ((b) ? ((c) | (1 << (i))) : ((c) & ~(1 << (i))))

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifdef  __cplusplus
}
#endif


#endif /* __MISC_H__ */
