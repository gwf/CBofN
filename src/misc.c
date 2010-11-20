
/* Copyright (c) 1995 Gary W. Flake -- Permission granted for any use
 * provied that the author's comments are neither modified nor removed.
 * No warranty is given or implied.
 *
 * NAME
 *   misc.c
 * PURPOSE
 *   Miscellaneous routines for program examples.  Most of the programs
 *   will need to be linked to this code.
 */

#include <math.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "misc.h"

/* A little buffer for formatting option help string entries. */

#define BUFFERLEN 4096

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static void format_string(char *str, int leftind, int rightind, int firstind)
{
  int i, start, stop, len, save, first = 1;
  char *indent;
  
  indent = xmalloc(sizeof(char) * (MAX(leftind, firstind) + 1));
  for(i = 0; i < firstind; i++)
    indent[i] = ' ';
  indent[firstind] = 0;
  fprintf(stderr, indent);
  for(i = 0; i < leftind; i++)
    indent[i] = ' ';
  indent[leftind] = 0;
  len = strlen(str);
  start = 0;
  while(1) {
    if(start >= len) break;
    if(!first) {
      while(str[start] == ' ') start++;
      fprintf(stderr, indent);
      stop = start + (80 - leftind - rightind);
    }
    else
      stop = start + (80 - firstind - rightind);
    if(stop > len)
      stop = len - 1;
    else
      while(!isspace(str[stop]) && stop > start) stop--;
    if(stop <= start) break;
    save = str[stop]; str[stop] = 0;
    fprintf(stderr, "%s", str + start);
    str[stop] = save;
    if(str[stop] != ' ')
      fputc(str[stop], stderr);
    start = stop + 1;
    fputc('\n', stderr);
    first = 0;
  }
  free(indent);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#define HELP_INDENT 4

static void display_options(char **argv, OPTION *options, char *help)
{
  int i, j, maxlen, len;
  char buffer[BUFFERLEN], temp[256];
  
  fprintf(stderr, "Usage: %s [ options ]\n\n", argv[0]);
  format_string(help, HELP_INDENT, HELP_INDENT, HELP_INDENT);
  fprintf(stderr, "\nOptions with defaults in parentheses are:\n\n");
  
  i = maxlen = 0;
  while(options[i].name != NULL) {
    if(maxlen < (len = strlen(options[i].name)))
      maxlen = len;
    i++;
  }
  
  i = 0;
  while(options[i].name != NULL) {
    buffer[0] = 0;
    strcat(buffer, options[i].name);    
    len = strlen(options[i].name);
    for(j = 0; j < maxlen - len + 2; j++)
      buffer[len + j] = ' ';
    buffer[maxlen + 2] = 0;
    strcat(buffer, options[i].help);
    switch(options[i].type) {
      case OPT_INT:
        sprintf(temp, " (%d)", *(int *)options[i].ptr);
        strcat(buffer, temp);
        format_string(buffer, maxlen + 6, 4, 4);
        break;
      case OPT_DOUBLE:
        sprintf(temp, " (%g)", *(double *)options[i].ptr);
        strcat(buffer, temp);
        format_string(buffer, maxlen + 6, 4, 4);
        break;
      case OPT_STRING:
        sprintf(temp, " (\"%s\")", *(char **)options[i].ptr);
        strcat(buffer, temp);
        format_string(buffer, maxlen + 6, 4, 4);
        break;
      case OPT_SWITCH:
        sprintf(temp, (*(int *)options[i].ptr) ? " (ON)" : " (OFF)");
        strcat(buffer, temp);
        format_string(buffer, maxlen + 6, 4, 4);
        break;
      case OPT_OTHER:
        sprintf(temp, " (no default)");
        strcat(buffer, temp);
        format_string(buffer, maxlen + 6, 4, 4);
        break;
      default:
        break;
    }
    i++;
  }
  fprintf(stderr, "\n");
  exit(1);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void bad_option(char **argv, int badopt)
{
  fprintf(stderr, "%s: unknown or incorrectly used option \"%s\".\n",
          argv[0], argv[badopt]);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void get_options(int argc, char **argv, OPTION *options, char *help)
{
  int i, j, found;
  
/*MRM begin*/

#if __dest_os == __mac_os
argc = ccommand (&argv);
#endif

/*MRM end*/

  /* For each argument in the command line.
   */
  i = 1;
  while(i < argc) {
    found = 0, j = 0;
    /*
     * Search for the current word in the option list.
     */
    while(!found) {
      /*
       * If the option was not found.
       */
      if(strcmp(argv[i], "-help") == 0) {
        display_options(argv, options, help);
      }
      else if(options[j].name == NULL) {
        bad_option(argv, i);
        display_options(argv, options, help);
      }
      /*
       * If the word is found in the option list.
       */ 
      else if(strcmp(argv[i], options[j].name) == 0) {
        /*
         * Check the type, make sure we have another arg
         * coming if needed, eat up the next arg, set
         * the value of the parameter, and display help
         * if needed.
         */
        found = 1;
        switch(options[j].type) {
          case OPT_INT:
            if(i + 1 >= argc) {
              bad_option(argv, i);
              display_options(argv, options, help);
            }
            *(int *)options[j].ptr = atoi(argv[i + 1]);
            i += 2;
            break;
          case OPT_DOUBLE:
            if(i + 1 >= argc) {
              bad_option(argv, i);
              display_options(argv, options, help);
            }
            *(double *)options[j].ptr = atof(argv[i + 1]);
            i += 2;
            break;
          case OPT_STRING:
            if(i + 1 >= argc) {
              bad_option(argv, i);
              display_options(argv, options, help);
            }
            *(char **)options[j].ptr = argv[i + 1];
            i += 2;
            break;
          case OPT_SWITCH:
            *(int *)options[j].ptr = !*(int *)options[j].ptr;
            i += 1;
            break;
          case OPT_OTHER:
            if(((int (*)())options[j].ptr)(argv, argc, options, &i, j)) {
              bad_option(argv, i);
              display_options(argv, options, help);
            }
            break;
          default:
            break;
        }
      }
      j++;
    }
  }
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

SCANNER *scan_init(FILE *fp, char *delimiters, char *whites, char *comments)
{
  SCANNER *s;

  s = xmalloc(sizeof(SCANNER));
  s->fp = fp;
  s->delims = delimiters;
  s->whites = whites;
  s->comments = comments;
  s->buffer = xmalloc(MAXSCANLINELEN * sizeof(char));
  s->token = xmalloc(MAXSCANLINELEN * sizeof(char));
  s->ptr = NULL;
  return(s);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void scan_flush(SCANNER *s)
{
  s->ptr = NULL;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static char *scan_get_or_peek(SCANNER *s, int get_flag)
{
  char *mark, *save;
  int i;
 
  while(1) {
    /* Get a new line of necessary.  The four conditions below are:
     * 1) this is a new line or a flush has been requested;
     * 2) we are looking at a comment character;
     * 3) end of line; and 4) end of line (probably end of file as well)
     */    
    if(s->ptr == NULL || strchr(s->comments, *s->ptr) ||
       *s->ptr == '\n'|| *s->ptr == 0) {
      
      /* Read a line.  If it's EOF then return an empty string */
      if((s->ptr = fgets(s->buffer, MAXSCANLINELEN, s->fp)) == NULL) {
        s->token[0] = 0;
        return(s->token);
      }

      /* Loop until we no longer have to get more lines. */
      continue;
    }

    /* Skip white space. */
    while(strchr(s->whites, *s->ptr)) {
      s->ptr++;
      if(*s->ptr == 0) break;
    }

    /* Skip EOL and comments. */
    if(strchr(s->comments, *s->ptr) || *s->ptr == '\n' || *s->ptr == 0)
      continue;

    /* Look for dilimiters. */
    if(strchr(s->delims, *s->ptr)) {
      s->token[0] = *s->ptr;
      s->token[1] = 0;
      
      if(get_flag)
        s->ptr++;
      return(s->token);
    }

    /* As long as the current characters is not a delimiter, white space,
     * comment, nor end of line, then increment the ptr.
     */
    mark = save = s->ptr;
    while(*s->ptr && *s->ptr != '\n' && !strchr(s->delims, *s->ptr) 
          && !strchr(s->whites, *s->ptr) && !strchr(s->comments, *s->ptr))
      s->ptr++;
    
    /* Copy everything between mark and ptr into token. */
    for(i = 0; mark < s->ptr; mark++, i++)
      s->token[i] = *mark;
    s->token[i] = 0;

    /* If this is a peek then resore the pointer. */
    if(!get_flag)
      s->ptr = save;
    return(s->token);
} }

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

char *scan_get(SCANNER *s)
{
  return(scan_get_or_peek(s, 1));
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

char *scan_peek(SCANNER *s)
{
  return(scan_get_or_peek(s, 0));
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void *xmalloc(size_t bytes)
{
  void *ptr;

  if((ptr = malloc(bytes)) == NULL) {
    fprintf(stderr, "malloc failed to get %d bytes.\n", bytes);
    exit(1);
  }
  return(ptr);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

double random_range(double low, double high)
{
  double tmp;

  tmp = fabs((double)(random()) / ((double) RAND_MAX + 1));
  tmp = tmp * (high - low) + low;
  return(tmp);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

double random_gauss(void)
{
  static int holding = 0;  /* Are we holding an old value? */
  static double hold;      /* The last value that we save. */
  double factor, r, v1, v2;

  if(holding) {
    holding = 0;
    return(hold);
  }
  else {
    do {
      v1 = random_range(-1.0, 1.0);
      v2 = random_range(-1.0, 1.0);
      r = v1 * v1 + v2 * v2;
    } while(r >= 1.0);
    factor = sqrt(-2.0 * log(r) / r);
    hold = v1 * factor;
    holding = 1;
    return(v2 * factor);
  }
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

int **read_pbm_file(char *fname, int *w, int *h)
{
  int **data, i, j;
  SCANNER *scan;
  FILE *fp;
  char *s;

  if((fp = fopen(fname, "r")) == NULL) {
    fprintf(stderr, "Unable to open \"%s\"", fname);
    perror(":");
    exit(1);
  }
  scan = scan_init(fp, "", " \t\n", "#");
  s = scan_get(scan);
  if(strcmp("P1", s) != 0) {
    fprintf(stderr, "File \"%s\" is not a pbm file.\n", fname);
    exit(1);
  }
  s = scan_get(scan);
  *w = atoi(s);
  s = scan_get(scan);
  *h = atoi(s);
  scan->delims = "01";
  data = xmalloc(sizeof(int *) * *h);
  for(i = 0; i < *h; i++) {
    data[i] = xmalloc(sizeof(int) * *w);
    for(j = 0; j < *w; j++) {
      s = scan_get(scan);
      if(*s != '0' && *s != '1')
        fprintf(stderr, "error: (%d, %d) read ascii(%d) = '%c'\n",
                i, j, *s, *s);
      data[i][j] = s[0] - '0';
    }
  }
  return(data);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */



