#!/bin/bash
set -e

# Prefix for message
PREFIX_ERRN="ERRN:"
PREFIX_INFO="INFO:"

# Check parameters
# $ path_to_project lib_name target_lib1 target_lib2 ... target_libn
if [ ! -d "$1" ] || [ -z "$1" ]; then
  echo "$PREFIX_ERRN path '$1' is not folder or doesn't specified"
  exit 1
fi
if [ -z "$2" ]; then
  echo "$PREFIX_ERRN second arg 'lib_name' doesn't specified"
  exit 1
fi

ENTRY_PATH=$(pwd)
cd "$1"

# Create makefile
CMAKEFILE="CMakeLists.txt"
touch ./"$CMAKEFILE"
echo "" > ./"$CMAKEFILE"

# Parse parameters: $ lib_name target_lib1 target_lib2 target_lib3 ... target_libn
LINK_LIBS=""
i=1
STEPS=$#
while [ $i -le $STEPS ]
do
  eval "SUBSTEP${i}=${1}";
  # first arg is path to source, second 'name of lib', other args are target libs
  if [ $i -gt 2 ]; then
    LINK_LIBS="${LINK_LIBS}\n${1}"
  fi
  shift;
  i=$(( i + 1 ));
done;

# Output incoming parameters
LIB_NAME=$SUBSTEP2
echo "lib name: '${LIB_NAME}'"
echo "target link libs: '${LINK_LIBS}'"

# Load files from folder
LIB_FILES=""
for f in *.c *.cpp
do
  [ -e "$f" ] || continue
  LIB_FILES="${LIB_FILES}\n${f}"
done
echo "lib files: '${LIB_FILES}'"

# Add to library
LEXEMES="add_library(${LIB_NAME}\
  ${LIB_FILES}\
  )\n\
  \n\
  target_link_libraries(${LIB_NAME}\
	${LINK_LIBS}\
  )\n"

sed -i "a\ ${LEXEMES}" "${CMAKEFILE}"

echo "$PREFIX_INFO cmake file for lib '${LIB_NAME}' has been builded"
# Restore pwd
cd "$ENTRY_PATH"
exit 0
