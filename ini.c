/* ini - read an ini file from standard input */

#include <config.h>
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#include "bashansi.h"
#include "loadables.h"
#include <ini.h>
#include <stdio.h>

static int handler(void *user, const char *section, const char *name,
                   const char *value) {
  SHELL_VAR *entry;
  unsigned int array_index;
  char *array_name;
  int vflags = 0;
  vflags |= (1 << 0);
  vflags |= (1 << 1);
  array_name = "INI";
  entry = find_or_make_array_variable(array_name, vflags);
  if (entry == 0 || readonly_p(entry) || noassign_p(entry)) {
    if (entry && readonly_p(entry)) {
      err_readonly(array_name);
    }
    return (EXECUTION_FAILURE);
    /* } else if (array_p(entry) == 0) { */
    /*   builtin_error("%s: not an indexed array", array_name); */
    /*   return (EXECUTION_FAILURE); */
  } else if (invisible_p(entry))
    VUNSETATTR(entry, att_invisible); /* no longer invisible */

  /* if (flags & MAPF_CLEARARRAY) */
  /*   array_flush(array_cell(entry)); */

  /* bind_array_element(entry, array_index, line, 0); */
  SHELL_VAR *v;
  bind_assoc_variable(entry, array_name, strdup(name), strdup(value), 0);
  return 0;
}

int ini_builtin(list) WORD_LIST *list;
{
  int opt, rval;
  char *array_name, *inistring;
  SHELL_VAR *v;

  array_name = 0;
  rval = EXECUTION_SUCCESS;

  reset_internal_getopt();
  while ((opt = internal_getopt(list, "a:")) != -1) {
    switch (opt) {
    case 'a':
      array_name = list_optarg;
      break;
      CASE_HELPOPT;
    default:
      builtin_usage();
      return (EX_USAGE);
    }
  }
  list = loptend;

  int fd;
  FILE *file;

  fd = 0;
  file = fdopen(fd, "r");
  if (ini_parse_file(file, handler, NULL) < 0) {
    printf("Unable to read file from stdin\n");
    return 1;
  }
  return (rval);
}

/* Called when builtin is enabled and loaded from the shared object.  If this
   function returns 0, the load fails. */
int ini_builtin_load(name) char *name;
{ return (1); }

/* Called when builtin is disabled. */
void ini_builtin_unload(name) char *name;
{}

char *ini_doc[] = {
    "Read ini config from from standard input into an associtive array.",
    "",
    "Read ini config from standard input into a set of associative arrays",
    "from file descriptor FD if the -u option is supplied. The variable",
    "MAPFILE",
    "is the default ARRAY.",
    (char *)NULL};

struct builtin ini_struct = {
    "ini",                 /* builtin name */
    ini_builtin,           /* function implementing the builtin */
    BUILTIN_ENABLED,       /* initial flags for builtin */
    ini_doc,               /* array of long documentation strings. */
    "ini -p array_prefix", /* usage synopsis; becomes short_doc */
    0                      /* reserved for internal use */
};
