# Bash Builtin to Parse ini Configs

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
