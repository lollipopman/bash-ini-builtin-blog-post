#include "loadables.h"
#include <errno.h>

char *sleep_doc[] = {"Patience please, wait for a bit!", NULL};

int sleep_builtin(WORD_LIST *list) {
  if (!list) {
    builtin_usage();
    return EX_USAGE;
  }
  char *endptr;
  char *secs_arg = list->word->word;
  uintmax_t secs = strtoumax(list->word->word, &endptr, 10);
  if ((secs == UINTMAX_MAX && errno == ERANGE) || (*endptr != '\0')) {
    builtin_error("Unable to convert `%s` to an integer", secs_arg);
    return EXECUTION_FAILURE;
  }
  unsigned int rem = sleep(secs);
  if (rem == 0) {
    return EXECUTION_SUCCESS;
  } else {
    builtin_error("Sleep interrupted, %d secs remaining", rem);
    return EXECUTION_FAILURE;
  }
}

/* Provides Bash with information about the builtin */
struct builtin sleep_struct = {
    .name = "sleep",             /* Builtin name */
    .function = sleep_builtin,   /* Function implementing the builtin */
    .flags = BUILTIN_ENABLED,    /* Initial flags for builtin */
    .long_doc = sleep_doc,       /* Array of long documentation strings. */
    .short_doc = "sleep NUMBER", /* Usage synopsis; becomes short_doc */
    .handle = 0                  /* Reserved for internal use */
};
