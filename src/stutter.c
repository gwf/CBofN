
/* NAME
 *   stutter - a simple lisp interpreter
 * NOTES 
 *   When an error condition is caught a message is printed and the
 *   error_cell is returned.  This is a clue to calling routines that
 *   1) an error has occured; 2) they should print nothing; 3) they
 *   should also return the error_cell.
 *   
 *   The source code can be roughly divided as follows:
 *
 *     Memory and Name Managment:
 *       new_cell()      - allocate a new cell
 *       new_atom()      - allocate a new atom
 *       initialize()    - defines some primitive cells
 *
 *     Parsing:
 *       parse_list()    - parses a single list
 *       parse()         - parses a single expression
 *
 *     Value Functions:
 *       cons()          - makes a list
 *       car()           - gives the head of a list
 *       cdr()           - gives the tail of a list
 *       set()           - binds atoms to values
 *       equal()         - are two atoms the same?
 *
 *     Special Functions:
 *       quote()         - the identity function
 *       lambda()        - creates a user defined function
 *       lisp_if()       - simple control structure
 *
 *     Read, Eval, Print Loop:
 *       read_lisp()     - prints a prompt and reads from parse()
 *       eval_atom()     - looks up bindings
 *       eval_lambda()   - evaluates user function calls
 *       eval_lisp()     - handles other types of expressions
 *       print_lisp()    - prints a cell
 *       main()          - does the read, eval, print loop.
 *
 * BUGS
 *   If STUTTER can free no cell after a call to the garbage collector,
 *   it will exit.
 * SEE ALSO
 *   In the data directory are the STUTTER examples demo.slp, sample.slp
 *   and float.slp, which may be helpful.
 * AUTHOR
 *   Copyright (c) 1997, Gary William Flake.
 *   
 *   Permission granted for any use according to the standard GNU
 *   ``copyleft'' agreement provided that the author's comments are
 *   neither modified nor removed.  No warranty is given or implied.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef WIN32
#define isatty(x) 1
#else
#include <unistd.h>
#endif

char help_string[] = "\
This is a simple lisp interpreter that only understands car, cdr, \
cons, if, set, equal, quote, and lambda, but is still Turing-complete. \
It uses stop-and-copy garbage collection and has an adjustable heap \
size.  Besides the primitive functions listed above, STUTTER only knows \
about two cells, t and nil.  See the examples for how integer \
arithmetic and more complicated programming constructs can be formed \
with the primitives. \
";

#include "misc.h"

/* Five type of cells:  lambda expressions, special functions (which don't 
   necessarily evaluate thier arguments), value functions (builtin functions
   which have all of thier arguments evaluated before the call), lists, and
   atoms. */

typedef enum CELL_ENUM {
  CELL_LAMBDA, CELL_SFUNC, CELL_VFUNC, CELL_LIST, CELL_ATOM
} CELL_TYPE;


/* Cells are simply the type flag, the car, and the cdr.  */

typedef struct CELL_STRUCT {
  void *car, *cdr;
  unsigned type : 7;
  unsigned mark : 1;
  char empty[3];
} CELL;


/* Handy macros.  Car and cdr are for lists, name is for atoms, and
   func is for sfuncs and vfuncs.  Lambdas use the car and cdr for
   the parameter list and the body. */

#define cell_car(c)      ((CELL *)(c)->car)
#define cell_cdr(c)      ((CELL *)(c)->cdr)
#define cell_name(c)     ((char *)(c)->car)
#define cell_func(c)     ((CELL *(*)())(c)->car)
/*MRM begin*/
#if __dest_os == __mac_os
#define cell_type(c)     ((c)->type)
#else
#define cell_type(c)     ((CELL_TYPE)(c)->type)
#endif
/*MRM end*/
#define cell_mark(c)     ((c)->mark)

#define cell_car_assign(c, v)  (((c)->car) = v)
#define cell_cdr_assign(c, v)  (((c)->cdr) = v)
#define cell_name_assign(c, v) (((c)->car) = v)
#define cell_func_assign(c, v) (((c)->car) = v)
#define cell_type_assign(c, v) (((c)->type) = v)


/* Global cells that we'll need at some point.  Binding_list holds all
   of the symbol bindings in an association list. */

CELL *nil_cell, *quote_cell, *error_cell, *true_cell;
CELL *binding_list, *heap, *free_list, **protect_table;

int heap_size = 10240;
int protect_size = 256, protect_used = 0;

SCANNER *scan;

/* Forward declarations. */

CELL *cons(CELL *a, CELL *b), *new_atom(char *name), *new_cell(CELL_TYPE);
CELL *set(CELL *, CELL *), *read_lisp(void), *eval_lisp(CELL *);
void print_lisp(CELL *);

/*MRM begin*/
#if __dest_os == __mac_os
char *strdup (const char *str) {
	char *p = xmalloc (strlen(str)+1);
	strcpy (p, str);
	return p;
}
#endif
/*MRM end*/

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void mark(CELL *cell)
{
  if(!cell || cell_mark(cell))
    return;

  cell_mark(cell) = 1;
  if(cell_type(cell) == CELL_LAMBDA || cell_type(cell) == CELL_LIST) {
    mark(cell_car(cell));
    mark(cell_cdr(cell));
  }
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#define unprotect() protect_used--

void protect(CELL *cell)
{
  if(protect_used >= protect_size) {
    protect_table = realloc(protect_table,
                            sizeof(CELL *) * protect_size * 2);
    if(protect_table == NULL) {
      fprintf(stderr, "protect: unable to realloc %d bytes.\n",
              sizeof(CELL *) * protect_size * 2);
      exit(1);
    }
    protect_size *= 2;
  }
  protect_table[protect_used++] = cell;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void garbage_collect(void)
{
  CELL *cell;
  int i, count = 0;
  
  fprintf(stderr, "Garbage collecting...");
  fflush(stderr); 
  mark(binding_list);
  for(i = 0; i < protect_used; i++)
    mark(protect_table[i]);
  for(cell = heap, i = 0; i < heap_size; cell++, i++) {
    if(!cell_mark(cell)) {
      cell_car_assign(cell, free_list);
      free_list = cell;
      count++;
    }
    cell_mark(cell) = 0;
  }
  if(!count) {
    fprintf(stderr, "\nGarbage collection failed!\n");
    exit(1);
  }
  fprintf(stderr, "harvested %d cells.\n", count);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Return a new lisp cell of the specified type. */

CELL *new_cell(CELL_TYPE type)
{
  static int init = 0;
  CELL *cell;
  int i;

  if(!init) {
    init = 1;
    free_list = heap = xmalloc(sizeof(CELL) * heap_size);
    for(cell = heap, i = 0; i < heap_size - 1; cell++, i++)
      cell_car_assign(cell, cell + 1);
    cell_car_assign(cell, NULL);
    protect_table = xmalloc(sizeof(CELL *) * protect_size);
  }
  if(free_list == NULL)
    garbage_collect();
  cell = free_list;
  free_list = cell_car(free_list);
  cell_type_assign(cell, type);
  cell_car_assign(cell, cell_cdr_assign(cell, NULL));
  cell_mark(cell) = 0;
  return(cell);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Search for the atom with the given name.  If it is found then that
   atom cell is returned.  If it is not found then a new atom with the
   supplied name is created and inserted into the binding-list. */

CELL *new_atom(char *name)
{
  CELL *current, *list, *atom;

  current = binding_list;
  while(current != nil_cell && current != NULL) {
    if(strcmp(cell_name(cell_car(cell_car(current))), name) == 0) {
      return(cell_car(cell_car(current)));
    }
    current = cell_cdr(current);
  }
  /* Not found so insert it in front. */
  current = new_cell(CELL_LIST);
  cell_car_assign(current, atom = new_cell(CELL_ATOM));
  cell_cdr_assign(current, NULL);
  cell_name_assign(atom, strdup(name));
  protect(current);
  list = new_cell(CELL_LIST);
  unprotect();
  cell_car_assign(list, current);
  cell_cdr_assign(list, binding_list);
  binding_list = list;
  return(atom);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Parses everything in a list but the first '('.  Stops when it sees
   a trailing ')'.   Calls parse() to get the subexpressions. */

CELL *parse_list()
{
  CELL *expr, *list, *parse(void);
  char *token;
  
  /* Take a peek at the next token. */
  token = scan_peek(scan);
  if(*token == ')') {
    scan_get(scan); /* Yum! */
    return(nil_cell);
  }
  if(*token == 0) {
    printf("parse error: unexpected EOF.\n");
    return(error_cell);
  }
  if((expr = parse()) == error_cell)
    return(error_cell);
  protect(expr);
  if((list = parse_list()) == error_cell) {
    unprotect();
    return(error_cell);
  }
  unprotect();
  return(cons(expr, list));
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* The main parse routine.  Very little error checking. */

CELL *parse(void)
{
  CELL *expr;
  char *token;
  
  token = scan_get(scan);

  if(*token == '(')
    return(parse_list());

  /* Token is a quote.  Get the next expression and return (quote expr). */
  if(*token == '\'') {
    if((expr = parse()) != error_cell)
      return(cons(quote_cell, cons(expr, nil_cell)));
    else
      return(error_cell);
  }
  if(*token == ')') {
    printf("parse error: unexpected ')'\n");
    /* Flush the current input line. */
    scan_flush(scan);
    return(error_cell);
  }
  if(*token == 0)
    return(NULL);

  return(new_atom(token));    
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Given a cell and a list returns a new list of the form:
   (cell ... contents of list).  (cons 'cell nil) = '(cell). */

CELL *cons(CELL *a, CELL *b)
{
  CELL *c;

  if(a == error_cell || b == error_cell)
    return(error_cell);
  if(!(cell_type(b) == CELL_LIST || b == nil_cell)) {
    printf("Error: cons: second argument is not a list.");
    return(error_cell);
  }
  protect(a); protect(b);
  c = new_cell(CELL_LIST);
  unprotect(); unprotect();
  cell_car_assign(c, a);
  if(b == nil_cell)
    cell_cdr_assign(c, NULL);
  else
    cell_cdr_assign(c, b);
  return(c);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Returns the firts element in a list.  On nil it returns nil, unlike
   other implementations. */

CELL *car(CELL *a, CELL *b)
{
  if(a == error_cell)
    return(error_cell);
  if(a == nil_cell)
    return(nil_cell);
  if(cell_type(a) != CELL_LIST) {
    printf("Error: car: argument is not a list.");
    return(error_cell);
  }
  return(cell_car(a));
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Returns the trailing portion of a list (all but the first element). */

CELL *cdr(CELL *a, CELL *b)
{
  if(a == error_cell)
    return(error_cell);
  if(a == nil_cell)
    return(nil_cell);
  if(cell_type(a) != CELL_LIST) {
    printf("Error: cdr: argument is not a list.");
    return(error_cell);
  }
  if(cell_cdr(a) == NULL)
    return(nil_cell);
  return(cell_cdr(a));
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Sets atoms to supplied values by updating the binding list. */

CELL *set(CELL *a, CELL *b)
{
  CELL *current, *list;

  if(a == error_cell || b == error_cell)
    return(error_cell);
  if(cell_type(a) != CELL_ATOM) {
    printf("Error: set: first argument is not an atom.");
    return(error_cell);
  }
  current = binding_list;
  while(current != nil_cell && current != NULL) {
    if(cell_car(cell_car(current)) == a) {
      cell_cdr_assign(cell_car(current), b);
      return(b);
    }
    current = cell_cdr(current);
  }
  /* Not found so insert it in front. */
  protect(a); protect(b);
  current = new_cell(CELL_LIST);
  unprotect(); unprotect();
  cell_car_assign(current, a);
  cell_cdr_assign(current, b);
  protect(current);
  list = new_cell(CELL_LIST);
  unprotect();
  cell_car_assign(list, current);
  cell_cdr_assign(list, binding_list);
  binding_list = list;
  return(b);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* If both arguments are atoms and they are the same atom then (and only
   then) those atoms are equal. */

CELL *equal(CELL *a, CELL *b)
{
  if(a && cell_type(a) == CELL_ATOM && a == b)
    return(true_cell);
  return(nil_cell);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Returns it's argument un evauluated. */

CELL *quote(CELL *expr)
{
  return(car(expr, NULL));
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Defines a user function.  Sets the car and the cdr fields to the
   argument list and the body. */

CELL *lambda(CELL *expr)
{
  CELL *ufunc, *c;
  
  protect(expr);
  ufunc = new_cell(CELL_LAMBDA);
  unprotect();
  cell_car_assign(ufunc, car(expr, NULL));
  cell_cdr_assign(ufunc, car(cdr(expr, NULL), NULL));
  if(cell_car(ufunc) == error_cell || cell_cdr(ufunc) == error_cell)
    return(error_cell);
  c = cell_car(ufunc);
  if(cell_type(c) != CELL_LIST && c != nil_cell) {
    printf("Error: bad argument list supplied.");
    return(error_cell);
  }
  /* Check to make sure that the formal argument list is a simple list. */
  if(c != nil_cell)
    while(c) {
      if(cell_type(cell_car(c)) != CELL_ATOM) {
        printf("Error: bad argument list supplied.");
        return(error_cell);
      }
      c = cell_cdr(c);
    }
  return(ufunc);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Evaluates the first argument, bool.  If bool is true then we return
   the evaluation of the second argument, else we return the evaluation of
   the third argument. */

CELL *lisp_if(CELL *expr)
{
  CELL *bool_expr, *then_expr, *else_expr, *bool_rslt;

  bool_expr = car(expr, NULL);
  then_expr = car(cdr(expr, NULL), NULL);
  else_expr = car(cdr(cdr(expr, NULL), NULL), NULL);
  if(bool_expr == error_cell || then_expr == error_cell
     || else_expr == error_cell)
    return(error_cell);
  protect(expr);
  bool_rslt = eval_lisp(bool_expr);
  unprotect();
  if(bool_rslt != nil_cell)
    return(eval_lisp(then_expr));
  return(eval_lisp(else_expr));
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Prints a prompt and parses an expression.  If this is not a tty (i.e.
   input is comming from a pipe) then we print the input expression. */

CELL *read_lisp(void)
{
  CELL *expr;

  printf("> ");
  expr = parse();
  if(expr && !isatty(0)) {
    print_lisp(expr);
    printf("\n");
  }
  return(expr);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Searches for th binding of an atom. */

CELL *eval_atom(CELL *expr)
{
  CELL *cell = binding_list;

  while(cell != NULL) {
    if(cell_car(cell_car(cell)) == expr) {
      if(cell_cdr(cell_car(cell)) == NULL) {
        printf("Error: unbound atom \"%s\".", cell_name(expr));
        return(error_cell);
      }
      return(cell_cdr(cell_car(cell)));
    }
    cell = cell_cdr(cell);
  }
  printf("Error: unbound atom \"%s\".", cell_name(expr));
  return(error_cell);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Evaluates a lambda expression in the context of the actual arguments. */

CELL *eval_lambda(CELL *expr, CELL *func)
{
  CELL *bind, *blist, *cell = NULL;
  CELL *argn = cell_car(func), *argv = cell_cdr(expr);
  int count = 0, i;

  /* Evaluate all of the parameters and hold them in a temporary list. */
  blist = nil_cell;
  protect(expr); protect(func);
  while(argn != NULL && argn != nil_cell) {
    protect(blist);
    if(argv == NULL)
      cell = nil_cell;
    else if((cell = eval_lisp(car(argv, NULL))) == error_cell) {
      count *= -1;
      unprotect();
      break;
    }
    protect(cell);
    bind = new_cell(CELL_LIST);
    unprotect(); /* cell */
    unprotect(); /* blist */
    cell_car_assign(bind, cell_car(argn));
    cell_cdr_assign(bind, cell);
    blist = cons(bind, blist);
    count++;
    argn = cell_cdr(argn);
    if(argv) argv = cell_cdr(argv);
  }

  /* Now that all of the formals are evaluated push the bindings on to
   * the global binding list.
   */
  protect(blist);
  while(blist != NULL && blist != nil_cell) {
    binding_list = cons(cell_car(blist), binding_list);
    blist = cell_cdr(blist);
  }
  
  /* Stupid kludge: if we had an error then the count is negative. */
  if(count >= 0 && cell != error_cell)
    cell = eval_lisp(cell_cdr(func));
  else {
    count *= -1;
  }
  
  /* Remove the bindings from this function call. */
  for(i = 0; i < count; i++)
    binding_list = cell_cdr(binding_list);

  unprotect(); unprotect(); unprotect(); /* blist, expr, func */
  return(cell);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Mother evaluation routine.  Simply dispatches other routines. */

CELL *eval_lisp(CELL *expr)
{
  CELL *func, *a, *b;

  if(expr == error_cell)
    return(error_cell);
  if(cell_type(expr) == CELL_ATOM)
    return(eval_atom(expr));
  else if(cell_type(expr) == CELL_LIST) {
    protect(expr);
    if((func = eval_lisp(cell_car(expr))) == error_cell) {
      unprotect();
      return(error_cell);
    }
    else if(cell_type(func) == CELL_VFUNC) {
      protect(a = eval_lisp(car(cell_cdr(expr), NULL)));
      b = eval_lisp(car(cdr(cell_cdr(expr), NULL), NULL));
      unprotect(); /* a */
      unprotect(); /* expr */
      return(cell_func(func)(a, b));
    }
    else if(cell_type(func) == CELL_SFUNC) {
      unprotect();
      return(cell_func(func)(cell_cdr(expr)));
    }
    else if(cell_type(func) == CELL_LAMBDA) {
      unprotect();
      return(eval_lambda(expr, func));
    }
  }
  return(expr);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Simple print routine. */

void print_lisp(CELL *cell)
{
  CELL *c;

  if(cell == NULL) {
    printf("<NULL>");
    return;
  }
  switch(cell_type(cell)) {
    case CELL_LAMBDA:
      printf("(lambda ");
      print_lisp(cell_car(cell));
      printf(" ");
      print_lisp(cell_cdr(cell));
      printf(")");
      break;
    case CELL_SFUNC:
      printf("<internal-special-function>");
      break;
    case CELL_VFUNC:
      printf("<internal-value-function>");
      break;
    case CELL_LIST:
      printf("(");
      while((c = cell_cdr(cell)) != NULL) {
        print_lisp(cell_car(cell));
        printf(" ");
        cell = c;
      }
      print_lisp(cell_car(cell));
      printf(")");
      break;
    case CELL_ATOM:
      if(cell != error_cell)
        printf("%s", cell_name(cell));
      break;
  }
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Define some cells that we need to know about internally, and bind the
   primitive functions to some meaningful names. */

void initialize(void)
{
  CELL *c;

  binding_list = NULL;

  nil_cell = new_atom("nil");
  set(nil_cell, nil_cell);

  true_cell = new_atom("t");
  set(true_cell, true_cell);

  error_cell = new_atom("<error>");
  set(error_cell, error_cell);

  set(new_atom("car"), c = new_cell(CELL_VFUNC));
  cell_func_assign(c, car);

  set(new_atom("cdr"), c = new_cell(CELL_VFUNC));
  cell_func_assign(c, cdr);

  set(new_atom("cons"), c = new_cell(CELL_VFUNC));
  cell_func_assign(c, cons);

  set(new_atom("set"), c = new_cell(CELL_VFUNC));
  cell_func_assign(c, set);

  set(new_atom("equal"), c = new_cell(CELL_VFUNC));
  cell_func_assign(c, equal);

  quote_cell = new_atom("quote");
  set(quote_cell, c = new_cell(CELL_SFUNC));
  cell_func_assign(c, quote);

  set(new_atom("lambda"), c = new_cell(CELL_SFUNC));
  cell_func_assign(c, lambda);

  set(new_atom("if"), c = new_cell(CELL_SFUNC));
  cell_func_assign(c, lisp_if);

  scan = scan_init(stdin, "()'", " \t", ";");
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Call initialize then go into the read, eval, print loop. */

int main(int argc, char **argv)
{
  CELL *expr;
  OPTION options[] = {
    { "-heap",   OPT_INT,     &heap_size,   "Number of cells in the heap." },
    { NULL,      OPT_NULL,    NULL,    NULL                    }
  };

  get_options(argc, argv, options, help_string);
  initialize();
  while (1) {
    expr = read_lisp();
    if(expr) {
      expr = eval_lisp(expr);
      print_lisp(expr);
      printf("\n");
    }
    else
      break;
  }
  printf("\n");
  exit(0);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
