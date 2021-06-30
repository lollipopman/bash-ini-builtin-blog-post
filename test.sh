#!/bin/bash
set -o errexit

enable -f ./ini.so ini
ini -a BUTTER <test.ini

declare -p BUTTER
for ini_var in "${!BUTTER[@]}"; do
	declare -n ini='BUTTER_'"$ini_var"
	declare -p "${!ini}"
done
