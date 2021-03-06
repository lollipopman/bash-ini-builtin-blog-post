# Writing a Bash Builtin in C to Parse INI Configs

## Why Not Just Parse INI Configs With Bash?

Shell languages such as Bash excel at certain tasks, such as gluing
programs together or quickly automating a set of command line steps. In
contrast to those strengths, using a Shell to parse an
[INI](https://en.wikipedia.org/wiki/INI_file) config file is a bit like
writing a poem in mud, you might succeed, but the result will probably
be inscrutable and your [swear
jar](https://en.wikipedia.org/wiki/Swear_jar) will be full! As this
wonderful Stack Overflow
[post](https://stackoverflow.com/questions/6318809/how-do-i-grab-an-ini-value-within-a-shell-script)
attests there are many different ways to parse an INI file in Bash, but
few of the answers provided are elegant.

So if you have a task poorly suited to Bash, what are your options?

1.  Choose another language for the task? (Perhaps sensible, but not
    always fun.)

2.  Write a custom Bash builtin to extend Bash for the task? (Spoiler,
    this is the route we will choose!)

## What is a Bash Builtin?

A builtin is a command in Bash that is implemented in the shell itself,
rather than as a separate program. They are the batteries included with
Bash. If you type `help` in Bash you get a list of all the currently
enabled builtins, or you can use `type printf` to check if a specific
command is a builtin. Many of the commands you use regularly are
builtins, e.g. `echo`, `printf`, and `cd`. They are typically
implemented in the language used to write the shell itself, so in the
case of Bash, C. Some of Bash's builtins are also available as separate
commands, depending on how your operating system is configured. For
example `printf` is a Bash builtin, but it is also usually available on
a Linux box as a separate program, try `which printf` to find out.
Builtins are preferred in Bash over external programs, as if they were
placed first in your `PATH`. Bash also allows you to write your own
custom builtins and load them into the shell, as does Zsh and the
KornShell.

## Why Would You Write a Builtin?

-   Why are builtins helpful?
-   Why not just rely entirely on external commands?

You could build a shell with a minimal set of builtins, but certain
builtins are still necessary. For example the `cd` command must be a
builtin, since calling `chdir(2)` in a forked process will have no
effect on the parent shell process. The shell must execute the `cd` and
thus the `chdir(2)` call in its own process. There are at least three
cases, including the `cd` example, where builtins are necessary or
useful:

1.  Avoiding the need to fork an external process.

2.  Calling a system function that affects the shell process itself,
    e.g. `chdir(2)`.

3.  Modifying a shell's internal state, e.g. adding a variable.

Our INI config parser builtin will demonstrate the utility of reason
number *(3)*. However, before we implement that builtin, let us try
implementing `sleep` as a builtin. Implementing `sleep` is a custom
builtin challenge akin to printing `Hello World!` in a new language.

## Minimal Builtin, Implementing `sleep`

Everyone needs sleep, but it can be costly in Bash. We had a program
that ironically slowed down after a
[spinner](https://en.wikipedia.org/wiki/Throbber#Spinning_wheel) was
added to provide feedback to the user that the program was still
running. The spinner called `sleep 0.04` in a loop while printing the
spinner characters to the screen. The creation of 25 forked processes
per second actually slowed down the program! Bash does have a sleep
builtin, but it is not enabled by default, let's create a simple
implementation:

``` C
#include "builtins.h"
#include "shell.h"
#include "bashgetopt.h"
#include "common.h"
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
```

The `struct builtin` is what informs Bash about our builtin. Notably we
provide a function here, `sleep_builtin`, which is essentially our
builtin's `main`. This function is supplied with any args provided to
our builtin. In our `sleep_builtin` function we check if we have an arg,
if so, we try to convert the arg to an integer and `sleep(3)` for that
number of seconds. Let's try it out:

``` bash
$ enable -f ./sleep.so sleep
$ help sleep
sleep: sleep NUMBER
    Patience please, wait for a bit!
$ time sleep 1
real    0m1.000s
user    0m0.000s
sys 0m0.000s
$ sleep ???
bash: sleep: Unable to convert `???` to an integer
```

Fabulous, so with a small amount of code and minimal boilerplate we have
created a dynamically loaded Bash builtin! The sleep builtin satisfies
reason number *(1)* on why you might write a builtin by eliminating the
need to fork a process for each `sleep` execution. With `sleep` as a
builtin each call is a function call rather than a process `fork(2)`,
i.e. bring back the spinner! But, it does not satisfy reason number
*(3)*, changing Bash's internal state. Let's implement an INI parser to
satisfy reason number *(3)* and provide a more complete example of
creating a Bash builtin.

## Writing an INI Parser Builtin

### Generating Help Output

First we'll create our `help ini` doc which provides the builtin
documentation inside of Bash. This help text provides an overview of how
our INI parser will affect Bash's state:

``` C
char *ini_doc[] = {
    "Reads an INI config from stdin input into a set of associative arrays.",
    "",
    "Reads an INI config from stdin input into a set of associative arrays.",
    "The sections of the INI config are added to an associative array",
    "specified by the `-a TOC` argument. The keys and values are then added to",
    "associative arrays prefixed by the `TOC` name and suffixed by their INI",
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
    NULL};
```

### Informing Bash About our Builtin

``` C
struct builtin ini_struct = {
    .name = "ini",            /* Builtin name */
    .function = ini_builtin,  /* Function implementing the builtin */
    .flags = BUILTIN_ENABLED, /* Initial flags for builtin */
    .long_doc = ini_doc,      /* Array of long documentation strings. */
    .short_doc =
        "ini -a TOC [-u FD] [-g]", /* Usage synopsis; becomes short_doc */
    .handle = 0                    /* Reserved for internal use */
};
```

As we did with the `sleep` builtin we initialize a `struct builtin` that
includes our `ini_doc` array as well as our short doc string. The second
member of the struct is the `sh_builtin_func_t` which is the `main`
function of our builtin.

### Parsing Options and Reading Stdin

Bash provides an `internal_getopt` function which is akin to
`getopt(3)`, but uses Bash's internal `WORD_LIST` structure. We parse
our mandatory argument `-a` for the name of the associative array which
will contain our INI section names. We parse our optional `-u` argument
which specifies an alternative [file
descriptor](https://en.wikipedia.org/wiki/File_descriptor) to read from
rather than the default of stdin. Once we have our file descriptor we
call `fdopen(3)` to obtain a `FILE` stream structure which we pass to
our INI parser.

``` C
int ini_builtin(WORD_LIST *list) {
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
        return EXECUTION_FAILURE;
      }
      fd = (int)intval;
      if (sh_validfd(fd) == 0) {
        builtin_error("%d: invalid file descriptor: %s", fd, strerror(errno));
        return EXECUTION_FAILURE;
      }
      break;
    case GETOPT_HELP:
      builtin_help();
      return EX_USAGE;
    default:
      builtin_usage();
      return EX_USAGE;
    }
  }
  if (!toc_var_name) {
    builtin_usage();
    return EX_USAGE;
  }
  FILE *file = fdopen(fd, "r");
  if (!file) {
    builtin_error("%d: unable to open file descriptor: %s", fd,
                  strerror(errno));
    return EXECUTION_FAILURE;
  }
  /* snip */
}
```

### Modifying Bash's Internal State, Injecting Data

The INI builtin creates a `TOC` or table of contents associative array
specifying which INI sections were found. Then for each INI section it
creates a `<TOC>_<INI_SECTION_NAME>` associative array. First we create
the `TOC` var:

``` C
/* snip */
ini_conf conf = {};
conf.toc_var_name = toc_var_name;
if (variable_context && !global_vars) {
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
  return EXECUTION_FAILURE;
}
if (ini_parse_file(file, handler, &conf) < 0) {
  builtin_error("Unable to read from fd: %d", fd);
  return EXECUTION_FAILURE;
}
return EXECUTION_SUCCESS;
/* snip */
```

We check Bash's `variable_context` to see if it is greater than zero
which indicates we are in a function. If we are in a function we create
local variables, unless the `-g` option was passed to our builtin. We
then setup our config for our INI parser. Bash provides functions to
create local, `make_local_assoc_variable` and global variables,
`make_new_assoc_variable`. Once we have created our `TOC` variable we
call the `ini_parse_file` function with our file, config, and handler
function. We are using the excellent
[inih](https://github.com/benhoyt/inih) library to do the complicated
parsing of the INI, but we do need to implement the inih handler
function:

``` C
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
                    1; // +1 for the NUL character
  char *sec_var_name = xmalloc(sec_size);
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
      free(sec_var_name);
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
    free(sec_var_name);
    return 1;
  }
  if (!name) {
    free(sec_var_name);
    builtin_error("Malformed ini, name is NULL!");
    return 0;
  }
  if (!value) {
    free(sec_var_name);
    builtin_error("Malformed ini, value is NULL!");
    return 0;
  }
  SHELL_VAR *sec_var = find_variable(sec_var_name);
  bind_assoc_variable(sec_var, sec_var_name, strdup(name), strdup(value), 0);
  free(sec_var_name);
  return 1;
}
```

In the handler we create our `sec_var_name` or
`<TOC>_<INI_SECTION_NAME>` string. Then if the handler was called at the
start of a new section we create an associative array for that section.
Otherwise, we use Bash's `find_variable` function to retrieve our
existing variable. Once we have our variable, Bash provides functions to
alter a variable's value. Here we use `bind_assoc_variable` to populate
an entry in our associative array with the name and value from the INI
parser. With our handler function complete our Bash builtin is ready to
parse some INI configs.

### Building & Testing

We put together a little `Makefile` to build and test our builtins:

``` make
SHELL=/bin/bash
CC:=gcc
CFLAGS:=-c -Wall -Wextra -fPIC
BASH_FLAGS:=$(shell pkgconf --cflags bash)
LDFLAGS:=--shared
INIH_FLAGS:=-DINI_CALL_HANDLER_ON_NEW_SECTION=1 -DINI_STOP_ON_FIRST_ERROR=1 \
    -DINI_USE_STACK=0

ini.so: inih/ini.o

%.so: %.o
    $(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
    $(CC) $(CFLAGS) -o $@ $^

inih/ini.o: CFLAGS += $(INIH_FLAGS)
ini.o: CFLAGS += $(BASH_FLAGS)
sleep.o: CFLAGS += $(BASH_FLAGS)

inih/ini.c:
    git submodule update --init

.PHONY: test
test: ini.so
    ./test
    @echo Tests Passed

.PHONY: clean
clean:
    shopt -s globstar; rm -f **/*.o **/*.so
```

Here we compile our `ini.c` file as well as the `inih` library, then we
link them together into a shared library. Our testing methodology is
rudimentary, we have a Bash script which exercises the features of our
builtin and we compare its output against a canonical copy:

``` bash
#!/bin/bash

set -o errexit
set -o nounset

new=$(mktemp)
bash test.sh >"$new"
diff -u test_output "$new"
```

### Let's Try It!

Now that we have compiled and tested our INI builtin, let's feed it a
config for a fictional RSS reader and see how it performs.

``` bash
$ enable -f ./ini.so ini
$ ini -a rss_conf <<'EOF'
 > [Computers]
 > Vidar's Blog = http://www.vidarholen.net/contents/blog/?feed=rss2
 > Two-Bit History = https://twobithistory.org/feed.xml
 > www.linusakesson.net = http://www.linusakesson.net/rssfeed.php
 > 
 > [Comics]
 > xkcd = http://xkcd.com/rss.xml
 > 
 > [Books]
 > The Marlowe Bookshelf = http://themarlowebookshelf.blogspot.com/feeds/posts/default
 > EOF
$ for section_name in "${!rss_conf[@]}"; do
 >   printf '## %s\n' "$section_name"
 >   declare -n section='rss_conf_'"$section_name"
 >   for key in "${!section[@]}"; do
 >     printf ' - %s: %s\n' "$key" "${section[$key]}"
 >   done
 > done
## Books
 - The Marlowe Bookshelf: http://themarlowebookshelf.blogspot.com/feeds/posts/default
## Comics
 - xkcd: http://xkcd.com/rss.xml
## Computers
 - www.linusakesson.net: http://www.linusakesson.net/rssfeed.php
 - Two-Bit History: https://twobithistory.org/feed.xml
 - Vidar's Blog: http://www.vidarholen.net/contents/blog/?feed=rss2
```

Our `TOC` var `rss_conf` holds our section names, then we use Bash's
nameref functionality to point a variable to each associative array for
a given INI section from the `rss_conf` associative array and iterate
over the section associative arrays. We have the RSS config loaded into
our Bash program, now we *just* need to build the application to consume
it!

## Closing Thoughts

Bash builtins provide a number of positive features. They provide an
interesting avenue for extending Bash to perform tasks which are perhaps
poorly suited to be written in the Bash language itself. Builtins also
allow Bash to leverage the vast quantity of well tested and established
C libraries. Lastly, as was hopefully demonstrated in the examples, Bash
provides a good framework for builtins and a set of functions that makes
modifying Bash's internal state straightforward.

Given the positives of Bash builtins, why aren't there
[more](https://github.com/search?o=desc&q=bash+builtin+language%3AC&s=stars&type=Repositories)
of them? There are two possibilities that stand out:

1.  The intersection of people that write Bash and C is rather small?
2.  The distribution of custom Bash builtins is not well paved, limiting
    their utility?

The second possibility rings the most true to me, I would love to see
innovation and improvement on the use and distribution of Bash builtins.
Ideally you could enable your builtins and they would be fetched along
with your main script and packed into a single binary, perhaps something
akin to [ruby-packer](https://github.com/pmq20/ruby-packer) but for
Bash. Bring your ideas!

## Further Reading

1.  [The full source code of this ini
    builtin](https://github.com/lollipopman/bash-ini-builtin-blog-post)
2.  [The inih library used to parse the INI
    configs](https://github.com/benhoyt/inih)
3.  [Bash builtin examples from Bash's source
    code](https://git.savannah.gnu.org/cgit/bash.git/tree/examples/loadables)

## Acknowledgments

Thanks to [Randall Mason](https://github.com/ClashTheBunny) for
providing feedback on this post.
