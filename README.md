# Bash Builtin to Parse ini Configs

This is a Bash builtin to parse ini config files using the
[inih](https://github.com/benhoyt/inih) library.

## Build

``` bash
git clone https://github.com/lollipopman/bash-ini-builtin-blog-post.git
cd bash-ini-builtin-blog-post
make
```

## Use

``` bash
enable -f ./ini.so ini
ini -a conf <test.ini
```
