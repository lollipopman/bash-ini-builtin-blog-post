#!/bin/bash
set -o errexit

enable -f ./ini.so ini
ini <test.ini
declare -p INI
