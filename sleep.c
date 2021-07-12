#include "loadables.h"
#include <errno.h>

char *sleep_doc[] = {"Patience please, wait for a bit!", (char *)NULL};

int sleep_builtin(WORD_LIST *list) {
  if (!list) {
    builtin_usage();
    return (EX_USAGE);
  }
  char *endptr;
  char *secs_arg = list->word->word;
  uintmax_t secs = strtoumax(list->word->word, &endptr, 10);
  if ((secs == UINTMAX_MAX && errno == ERANGE) || (*endptr != '\0')) {
    builtin_error("Unable to convert `%s` to an integer", secs_arg);
    return (EXECUTION_FAILURE);
  }
  sleep(secs);
  return (EXECUTION_SUCCESS);
}

/* Provides Bash with information about the builtin */
struct builtin sleep_struct = {
    "sleep",         /* Builtin name */
    sleep_builtin,   /* Function implementing the builtin */
    BUILTIN_ENABLED, /* Initial flags for builtin */
    sleep_doc,       /* Array of long documentation strings. */
    "sleep NUMBER",  /* Usage synopsis; becomes short_doc */
    0                /* Reserved for internal use */
};
