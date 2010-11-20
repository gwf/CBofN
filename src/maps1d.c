
/* NAME
 *   maps1d.c - one-dimensional maps
 * NOTES
 *   The functions in this file are one-dimensional maps.  This file
 *   is written in such a way that it should be included in the other
 *   programs that need the maps, and not compiled directly.
 *   
 *   The function get_named_function() expects that all functions
 *   that it may return reside in this file.
 * AUTHOR
 *   Copyright (c) 1997, Gary William Flake.
 *   
 *   Permission granted for any use according to the standard GNU
 *   ``copyleft'' agreement provided that the author's comments are
 *   neither modified nor removed.  No warranty is given or implied.
 */

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

double fsin(double x, double r)
{
  return(sin(x * M_PI * aux * 2.0 * r) * 0.5 + 0.5);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

double ftent(double x, double r)
{
  return((x <= 0.5) ? 2.0 * r * x : 2 * r * (1.0 - x));
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

double flog(double x, double r)
{
  return(4.0 * r * x * (1.0 - x));
}


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

double fgauss(double x, double r)
{
  return(r * exp(-aux * (x - 0.5) * (x - 0.5)));
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Add a new map function here, e.g.

double your_function(double x, double r)
{
  return(????);
}

*/

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

double (*get_named_function(char *name))(double x, double r)
{
  if(!strcmp(func, "sin"))
    return(fsin);
  else if(!strcmp(func, "tent"))
    return(ftent);
  else if(!strcmp(func, "log"))
    return(flog);
  else if(!strcmp(func, "gauss"))
    return(fgauss);

  /* Add a new function name here, e.g.

  else if(!strcmp(func, "your_function_name")
    return(your_function);
   
   */
  
  return(flog);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
