#!/bin/bash

set -o errexit
set -o nounset
set -o pipefail

enable -f ./ini.so ini

ini -a conf <test.ini
declare -p conf
for ini_var in "${!conf[@]}"; do
	declare -n ini='conf_'"$ini_var"
	declare -p "${!ini}"
done

# alternate fd
exec {ini_fd}<test.ini
ini -a alt_fd -u $ini_fd
declare -p alt_fd
for ini_var in "${!alt_fd[@]}"; do
	declare -n ini='alt_fd_'"$ini_var"
	declare -p "${!ini}"
done

# function
function parse-config() {
	local ini_file=$1
	local var_type=$2
	if [[ "$var_type" == 'global' ]]; then
		ini -ga inside_func <"$ini_file"
	else
		ini -a inside_func <"$ini_file"
	fi
	declare -p inside_func
	for ini_var in "${!inside_func[@]}"; do
		declare -n ini='inside_func_'"$ini_var"
		declare -p "${!ini}"
	done
	if local | grep -q 'inside_func='; then
		printf "inside_func is local!\n"
	else
		printf "inside_func is global!\n"
	fi
}

parse-config 'test.ini' 'local'
parse-config 'test.ini' 'global'
