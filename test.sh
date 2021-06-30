#!/bin/bash
set -o errexit

enable -f ./ini.so ini
ini -a BUTTER <test.ini

declare -p BUTTER
for ini_var in "${!BUTTER[@]}"; do
	declare -n ini='BUTTER_'"$ini_var"
	declare -p "${!ini}"
done

# alternate fd
exec {ini_fd}<test.ini
echo $ini_fd
ini -a FOO -u $ini_fd
declare -p FOO
