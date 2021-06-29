/* ini - read an ini file from standard input */

#include <config.h>
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#include "bashansi.h"
#include "inih/ini.h"
#include "loadables.h"
#include <stdio.h>

static int handler(void *user, const char *section, const char *name,
                   const char *value) {

  char *sec_prefix = "INI_";
  char *toc_var_name = "INI";
  size_t sec_size =
      strlen(sec_prefix) + strlen(section) + 1; // +1 for the null-terminator
  char *sec_var_name = malloc(sec_size);
  char *p = memccpy(sec_var_name, sec_prefix, '\0', sec_size);
  sec_size -= (p - sec_var_name - 1);
  if (sec_size <= 0) {
    free(sec_var_name);
    return 0;
  }
  char *q = memccpy(p - 1, section, '\0', sec_size);
  sec_size -= (q - (p - 1) - 1);
  if (sec_size <= 0) {
    free(sec_var_name);
    return 0;
  }
  if (legal_identifier(sec_var_name) == 0) {
    sh_invalidid(strdup(sec_var_name));
    free(sec_var_name);
    return 0;
  }

  int vflags = 0;
  vflags |= (1 << 0);
  vflags |= (1 << 1);
  SHELL_VAR *sec_var = find_or_make_array_variable(sec_var_name, vflags);
  if (sec_var == 0) {
    free(sec_var_name);
    return 0;
  }
  if (name && value) {
    bind_assoc_variable(sec_var, sec_var_name, strdup(name), strdup(value), 0);
  } else {
    vflags = 0;
    vflags |= (1 << 0);
    vflags |= (1 << 1);
    SHELL_VAR *toc_var = find_or_make_array_variable(toc_var_name, vflags);
    if (toc_var == 0) {
      free(sec_var_name);
      return 0;
    }
    bind_assoc_variable(toc_var, toc_var_name, strdup(section), strdup("true"),
                        0);
  }
  free(sec_var_name);
  return 1;
}

int ini_builtin(list) WORD_LIST *list;
{
  int opt;
  char *array_name, *inistring;
  SHELL_VAR *v;

  array_name = 0;

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
    return (EXECUTION_FAILURE);
  }
  return (EXECUTION_SUCCESS);
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
