/* ini - read an ini file from standard input */

#include <config.h>
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#include "inih/ini.h"
#include "loadables.h"
#include <errno.h>
#include <stdio.h>

char *ini_doc[] = {
    "Read an ini config from stdin input into a set of associative arrays.",
    "",
    "Reads an ini config from stdin input into a set of associative arrays.",
    "The sections of the ini config are added to an associative array",
    "specified by the `-a TOC` argument. The keys and values are then added to",
    "associate arrays prefixed by the `TOC` name and suffixed by their ini",
    "section name, `<TOC>_<INI_SECTION_NAME>`.",
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
    "If the `-u FD` argument is passed the ini config is read from the `FD`",
    "file descriptor rather than from stdin.",
    (char *)NULL};

typedef struct {
  char *toc_var_name;
} ini_conf;

static int handler(void *user, const char *section, const char *name,
                   const char *value) {
  ini_conf *conf = (ini_conf *)user;
  char *toc_var_name = conf->toc_var_name;
  int vflags;
  if (!name && !value) {
    vflags = 0;
    vflags |= (1 << 0);
    vflags |= (1 << 1);
    SHELL_VAR *toc_var = find_or_make_array_variable(toc_var_name, vflags);
    if (!toc_var) {
      report_error("Could not make %s", toc_var_name);
      return 0;
    }
    bind_assoc_variable(toc_var, toc_var_name, strdup(section), "true", 0);
    return 1;
  }
  if (!name) {
    report_error("Malformed ini, name is NULL!");
    return 0;
  }
  if (!value) {
    report_error("Malformed ini, value is NULL!");
    return 0;
  }
  char *sep = "_";
  size_t sec_size = strlen(toc_var_name) + strlen(section) +
                    2; // +1 for the null-terminator +1 for sep="_"
  char *sec_var_name = malloc(sec_size);
  char *sec_end = sec_var_name + sec_size;
  char *p = memccpy(sec_var_name, toc_var_name, '\0', sec_size);
  if (p) {
    p = memccpy(p - 1, sep, '\0', sec_end - p);
  }
  if (p) {
    memccpy(p - 1, section, '\0', sec_end - p);
  }
  if (!legal_identifier(sec_var_name)) {
    sh_invalidid(sec_var_name);
    free(sec_var_name);
    return 0;
  }
  vflags = 0;
  vflags |= (1 << 0);
  vflags |= (1 << 1);
  SHELL_VAR *sec_var = find_or_make_array_variable(sec_var_name, vflags);
  if (!sec_var) {
    report_error("Could not make %s", sec_var_name);
    free(sec_var_name);
    return 0;
  }
  bind_assoc_variable(sec_var, sec_var_name, strdup(name), strdup(value), 0);
  free(sec_var_name);
  return 1;
}

int ini_builtin(list) WORD_LIST *list;
{
  intmax_t intval;
  int opt, code;
  int fd = 0;
  char *toc_var_name = NULL;
  reset_internal_getopt();
  while ((opt = internal_getopt(list, "a:u:")) != -1) {
    switch (opt) {
    case 'a':
      toc_var_name = list_optarg;
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
  conf.toc_var_name = strdup(toc_var_name);
  FILE *file = fdopen(fd, "r");
  if (ini_parse_file(file, handler, &conf) < 0) {
    report_error("Unable to read from fd: %d", fd);
    return (EXECUTION_FAILURE);
  }
  return (EXECUTION_SUCCESS);
}

struct builtin ini_struct = {
    "ini",                /* builtin name */
    ini_builtin,          /* function implementing the builtin */
    BUILTIN_ENABLED,      /* initial flags for builtin */
    ini_doc,              /* array of long documentation strings. */
    "ini -a TOC [-u FD]", /* usage synopsis; becomes short_doc */
    0                     /* reserved for internal use */
};

/* Called when builtin is enabled and loaded from the shared object. If this
 * function returns 0, the load fails. */
int ini_builtin_load(name) char *name;
{ return (1); }

/* Called when builtin is disabled. */
void ini_builtin_unload(name) char *name;
{}
