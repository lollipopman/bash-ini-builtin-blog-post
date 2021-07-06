#include "inih/ini.h"
#include "loadables.h"
#include <errno.h>
#include <stdbool.h>

char *ini_doc[] = {
    "Reads an INI config from stdin input into a set of associative arrays.",
    "",
    "Reads an INI config from stdin input into a set of associative arrays.",
    "The sections of the INI config are added to an associative array",
    "specified by the `-a TOC` argument. The keys and values are then added to",
    "associate arrays prefixed by the `TOC` name and suffixed by their INI",
    "section name, `<TOC>_<INI_SECTION_NAME>`. The parsed INI section names",
    "must be valid Bash variable names, otherwise an error is returned.",
    "",
    "Example:",
    "",
    "  Input input.ini:",
    "    [sec1]",
    "    foo = bar",
    "",
    "    [sec2]",
    "    biz = baz",
    "",
    "  Result:",
    "    $ ini -a conf < input.ini",
    "    $ declare -p conf",
    "    declare -A conf=([sec1]=\"true\" [sec2]=\"true\" )",
    "    $ declare -p conf_sec1",
    "    declare -A conf_sec1=([foo]=\"bar\" )",
    "    $ declare -p conf_sec2",
    "    declare -A conf_sec2=([biz]=\"baz\" )",
    "",
    "If the `-u FD` argument is passed the INI config is read from the `FD`",
    "file descriptor rather than from stdin. Variables are created with local",
    "scope inside a function unless the `-g` option is specified.",
    (char *)NULL};

/* User data for inih callback handler */
typedef struct {
  char *toc_var_name;
  bool local_vars;
} ini_conf;

/* This is the inih handler called for every new section and for every name and
 * value in a section. This function creates and populates our associative
 * arrays in Bash. Both for the TOC array as well as for the individual section
 * arrays, <TOC>_<INI_SECTION_NAME> */
static int handler(void *user, const char *section, const char *name,
                   const char *value) {
  ini_conf *conf = (ini_conf *)user;
  char *toc_var_name = conf->toc_var_name;
  /* Create <TOC>_<INI_SECTION_NAME> */
  char *sep = "_";
  size_t sec_size = strlen(toc_var_name) + strlen(section) + strlen(sep) +
                    1; // +1 for the null-terminator
  char *sec_var_name = malloc(sec_size);
  char *sec_end = sec_var_name + sec_size - 1;
  char *p = memccpy(sec_var_name, toc_var_name, '\0', sec_size);
  if (!p) {
    builtin_error("Unable to create section name");
    return 0;
  }
  p = memccpy(p - 1, sep, '\0', sec_end - p + 2);
  if (!p) {
    builtin_error("Unable to create section name");
    return 0;
  }
  p = memccpy(p - 1, section, '\0', sec_end - p + 2);
  if (!p) {
    builtin_error("Unable to create section name");
    return 0;
  }
  if (!legal_identifier(sec_var_name)) {
    sh_invalidid(sec_var_name);
    free(sec_var_name);
    return 0;
  }
  /* New section parsed */
  if (!name && !value) {
    SHELL_VAR *toc_var = find_variable(toc_var_name);
    if (!toc_var) {
      builtin_error("Could not find %s", toc_var_name);
      return 0;
    }
    bind_assoc_variable(toc_var, toc_var_name, strdup(section), "true", 0);
    SHELL_VAR *sec_var = NULL;
    if (conf->local_vars) {
      int vflags = 0;
      sec_var = make_local_assoc_variable(sec_var_name, vflags);
    } else {
      sec_var = make_new_assoc_variable(sec_var_name);
    }
    if (!sec_var) {
      builtin_error("Could not make %s", sec_var_name);
      free(sec_var_name);
      return 0;
    }
    return 1;
  }
  if (!name) {
    builtin_error("Malformed ini, name is NULL!");
    return 0;
  }
  if (!value) {
    builtin_error("Malformed ini, value is NULL!");
    return 0;
  }
  SHELL_VAR *sec_var = find_variable(sec_var_name);
  bind_assoc_variable(sec_var, sec_var_name, strdup(name), strdup(value), 0);
  free(sec_var_name);
  return 1;
}

/* This is essentially the main function for the ini builtin, it does arg
 * parsing and then calls the inih function to parse the provided ini FD */
int ini_builtin(list) WORD_LIST *list;
{
  intmax_t intval;
  int opt, code;
  int fd = 0;
  bool global_vars = false;
  char *toc_var_name = NULL;
  reset_internal_getopt();
  while ((opt = internal_getopt(list, "a:gu:")) != -1) {
    switch (opt) {
    case 'a':
      toc_var_name = list_optarg;
      break;
    case 'g':
      global_vars = true;
      break;
    case 'u':
      code = legal_number(list_optarg, &intval);
      if (code == 0 || intval < 0 || intval != (int)intval) {
        builtin_error("%s: invalid file descriptor specification", list_optarg);
        return (EXECUTION_FAILURE);
      }
      fd = (int)intval;
      if (sh_validfd(fd) == 0) {
        builtin_error("%d: invalid file descriptor: %s", fd, strerror(errno));
        return (EXECUTION_FAILURE);
      }
      break;
    case GETOPT_HELP:
      builtin_help();
      return (EX_USAGE);
    default:
      builtin_usage();
      return (EX_USAGE);
    }
  }
  if (!toc_var_name) {
    builtin_usage();
    return (EX_USAGE);
  }
  ini_conf conf = {};
  conf.toc_var_name = toc_var_name;
  if ((variable_context > 0) && (global_vars == false)) {
    conf.local_vars = true;
  } else {
    conf.local_vars = false;
  }
  SHELL_VAR *toc_var = NULL;
  if (conf.local_vars) {
    int vflags = 0;
    toc_var = make_local_assoc_variable(toc_var_name, vflags);
  } else {
    toc_var = make_new_assoc_variable(toc_var_name);
  }
  if (!toc_var) {
    builtin_error("Could not make %s", toc_var_name);
    return 0;
  }
  FILE *file = fdopen(fd, "r");
  if (ini_parse_file(file, handler, &conf) < 0) {
    builtin_error("Unable to read from fd: %d", fd);
    return (EXECUTION_FAILURE);
  }
  return (EXECUTION_SUCCESS);
}

/* Provides Bash with information about the builtin */
struct builtin ini_struct = {
    "ini",                     /* Builtin name */
    ini_builtin,               /* Function implementing the builtin */
    BUILTIN_ENABLED,           /* Initial flags for builtin */
    ini_doc,                   /* Array of long documentation strings. */
    "ini -a TOC [-u FD] [-g]", /* Usage synopsis; becomes short_doc */
    0                          /* Reserved for internal use */
};
