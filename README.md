# Outline

## Goals

-   Educate people about the ability to extend Bash via builtins.
-   Provide motivation for why builtins are helpful.
-   Teach people how to write a builtin

# Writing a Bash Builtin in C to Parse INI Configs

## Motivation: stackoverflow post on parsing INIs in bash

Shell languages such as Bash excel at certain tasks, such as gluing
programs together or quickly automating a set of command line steps. In
contrast to those strenghs using a Shell to parse an
[INI](https://en.wikipedia.org/wiki/INI_file) config file, is a bit like
writing a poem in the snow, you might succeed, but the result will
probably be inscrutable and your [swear
jar](https://en.wikipedia.org/wiki/Swear_jar) will be full! As this
wonderful [stackoverflow
post](https://stackoverflow.com/questions/6318809/how-do-i-grab-an-ini-value-within-a-shell-script)
attests there any many different ways to parse an INI file in Bash, but
none are very elegant!

So if you have a task poorly suited to Bash, what are your options!!

1.  Choose another language, perhaps sensible, but not always fun?!

2.  Use a custom Bash builtin to extend Bash!? (Spoiler this is the
    route we will choose!)

## What is a Bash Builtin?

A builtin is a command in Bash that is implemented in the shell itself,
rather than as a separate program. Some of Bash builtins are also
usually available as separate commands. For example `printf` is a Bash
builtin, but is also usually available on a Linux box as a separate
program, try `which printf` to find out. If you type `help` in Bash you
get a list of all the currently loaded builtins. Builtins are preferred
over external programs, as if they were placed first in your `$PATH`.
Bash allows you to write your own custom builtins and load them into the
shell, as does Zsh and the KornShell.

## Why Would You Write a Builtin?

Why are builtins helpful? Why not just rely entirely on external
commands? You could build a shell with a minimal set of builtins, but
certain builtins are still necessary. For example the `cd` command must
be a builtin, since calling `chdir(2)` in a forked process will have no
effect on the parrent shell. The shell must execute the `cd` and thus
the `chdir` call in its own process. There are at least three cases
where builtins are necessary or useful:

1.  Avoiding the need to fork an external process

2.  Calling a system function that affect the shell process itself, e.g.
    `chdir(2)`

3.  Modifying a shell's internal state, e.g. adding a variable

Our INI config parser builtin will demonstrate reason number (3).
However, before we implement that builtin, let us look at implementing a
`sleep` builtin as a challege akin to printing `Hello World!`.

## Minimal Builtin, Implementing `sleep`

Everyone needs sleep, but it can be costly in Bash. We had a program
slow down by seconds after a
[spinner](https://en.wikipedia.org/wiki/Throbber#Spinning_wheel) was
added. The spinner called `sleep 0.04` between printing to the screen
and the 25 forked processes per second actually slowed down the program!
Bash does have a sleep builtin, but it is not enabled by default, a
simple implementation would look like this:

``` C
#include "loadables.h"
#include <errno.h>

char *sleep_doc[] = {"Patience please, wait for a bit!", (char *)NULL};

int sleep_builtin(list) WORD_LIST *list;
{
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
```

The `struct builtin` is what informs Bash about our builtin. Notably we
provide a function here, `sleep_builtin`, which is essentially our
builtin's `main`. This function is supplied any args provided to our
builtin. In our `sleep_builtin` function we check if we have an arg,
then if we do we try to convert the arg to an integer and sleep for that
number of seconds. Let's try it out!

``` bash
$ enable -f ./sleep.so sleep
$ help sleep
sleep: sleep NUMBER
    Patience please, wait for a bit!
$ time sleep 1
real    0m1.000s
user    0m0.000s
sys 0m0.000s
$ sleep ⏰
bash: sleep: Unable to convert `⏰` to an integer
```

Fabulous, so with a small amount of code and minimal boilerplate we have
created a dynamically loaded Bash builtin! The sleep builtin satisfies
reason number (1) on why you would write a builtin, i.e. to eliminate
the need to fork a process. But, it does not satisfy reason number (3),
changing Bash's internal state. Let's implement the INI parser to
satisfy reason number (3) and provide a more complete example of
creating a Bash builtin.

## Writing an INI Parser Builtin

### Generating Help Output

First we will create our `help ini` doc which provides the builtin
documentation inside of Bash. This help text provides an overview of how
our INI parser will affect Bash's state:

``` C
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
    "file descriptor rather than from stdin.",
    (char *)NULL};
```

### Parsing Options and Reading Stdin

Bash provides an `internal_getopt` function which is akin to
`getopt(3)`, but uses Bash's internal `WORD_LIST` structure. We parse
our mandatory argument `-t` for the name of the associative array which
will contain our INI section names. We also parse our optional `-u`
argument which specifies an alternative [file
descriptor](https://en.wikipedia.org/wiki/File_descriptor) to read from
rather than the default of stdin. Once we have our file descriptor we
call `fdopen(3)` to obtain a `FILE` stream structure which we pass to
our INI parser.

``` C
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
    builtin_error("Unable to read from fd: %d", fd);
    return (EXECUTION_FAILURE);
  }
  return (EXECUTION_SUCCESS);
}
```

### Modifying Bash's State, Injecting Data

### Building, testing

## Closing thoughts

## Further reading

This is a Bash builtin to parse ini config files using the
[inih](https://github.com/benhoyt/inih) library.

## Build

``` bash
$ git clone https://github.com/lollipopman/bash-ini-builtin-blog-post.git
$ cd bash-ini-builtin-blog-post
$ make
```

## Use

``` bash
$ enable -f ./ini.so ini
$ ini -a conf <test.ini
$ declare -p conf
declare -A conf=([protocol]="true" [user]="true" )
$ declare -p conf_protocol
declare -A conf_protocol=([version]="6" )
$ declare -p conf_user
declare -A conf_user=([active]="true" [pi]="3.14159" [email]="bob@smith.com" [name]="Bob Smith" )
```
