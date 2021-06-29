#!/bin/bash
set -o errexit

enable -f ./ini.so ini
ini <test.ini

declare -p INI
for ini_var in "${!INI[@]}"; do
	declare -n ini='INI_'"$ini_var"
	declare -p "${!ini}"
done
