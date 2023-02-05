#!/bin/bash
set -e
##########################################################################
# Prefix for message
##########################################################################
PREFIX_ERRN="ERRN:"
PREFIX_INFO="INFO:"

##########################################################################
# Check parameters
##########################################################################
if [ ! -d "$1" ] || [ -z "$1" ]; then
  echo "$PREFIX_ERRN path '$1' is not folder or doesn't specified"
  echo "$PREFIX_INFO perhaps don't added the user to the group. Example:'$ sudo usermod -aG vboxsf $(whoami)"
  exit 1
fi

GIT_REPO="https://github.com/pfagner/clc"

DST_FOLDER="$1"
DST_BUILD_MAKE="$DST_FOLDER/scripts/srv/build_make.sh"
DST_INC_CONTAINER="$DST_FOLDER/include/container"
DST_LIB_CONTAINER="$DST_FOLDER/lib/container"
SRC_FOLDER="$(pwd)/xxx"
SRC_INC_CONTAINER="$SRC_FOLDER/clc/include/container"
SRC_LIB_CONTAINER="$SRC_FOLDER/clc/lib/container"

if [ ! -d "$SRC_FOLDER" ]; then
  mkdir -p "$SRC_FOLDER"
fi
if [ ! -d "$DST_INC_CONTAINER" ]; then
  mkdir -p "$DST_INC_CONTAINER"
fi
if [ ! -d "$DST_LIB_CONTAINER" ]; then
  mkdir -p "$DST_LIB_CONTAINER"
fi

cd "$SRC_FOLDER"
git clone "$GIT_REPO"
cp "$SRC_INC_CONTAINER/list.h" "$DST_INC_CONTAINER/list.h"
cp "$SRC_LIB_CONTAINER/list.c" "$DST_LIB_CONTAINER/list.c"
cd ..
rm -rf "$SRC_FOLDER"
# Build makefile
sh "$DST_BUILD_MAKE" "$DST_LIB_CONTAINER" rxs_container

exit 0
