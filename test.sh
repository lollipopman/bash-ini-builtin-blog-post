#!/bin/bash

set -o errexit
set -o nounset
set -o pipefail

enable -f ./ini.so ini

ini -a BUTTER <test.ini
declare -p BUTTER
for ini_var in "${!BUTTER[@]}"; do
	declare -n ini='BUTTER_'"$ini_var"
	declare -p "${!ini}"
done

# alternate fd
exec {ini_fd}<test.ini
ini -a BUBBLES -u $ini_fd
declare -p BUBBLES
for ini_var in "${!BUBBLES[@]}"; do
	declare -n ini='BUBBLES_'"$ini_var"
	declare -p "${!ini}"
done
