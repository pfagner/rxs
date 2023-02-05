#!/bin/bash
set -e

# Prefix for message
PREFIX_ERRN="ERRN:"
PREFIX_INFO="INFO:"

# Check parameters
if [ ! -d "$1" ] || [ -z "$1" ]; then
  echo "$PREFIX_ERRN path '$1' is not folder or doesn't specified"
  echo "$PREFIX_INFO perhaps don't added the user to the group. Example:'$ sudo usermod -aG vboxsf $(whoami)"
  exit 1
fi

SRC_PATH="$1"
LAST_LEXEME=$(echo "$SRC_PATH" | sed -n 's/\/$//p' )
if [ -z "$LAST_LEXEME" ]; then
  LAST_LEXEME="$SRC_PATH"
fi
# Build name
BUILD_NAME=$(echo "$LAST_LEXEME" | sed -n 's:.*/\(.*\)*.*:\1:p')
BUILD_RELEASE_FILE="${SRC_PATH}/scripts/srv/build_release.sh"
NAME_RELEASE_ALL="$(pwd)/${BUILD_NAME}_release"

ARCHIVE_FILE="${BUILD_NAME}.tar"
INSTALL_FILE="install.sh"

# Remove any files and folders with build release
rm -rf "$NAME_RELEASE_ALL"*
# Build release
sh "${BUILD_RELEASE_FILE}" "$SRC_PATH"
# Only one release exist
cd "$NAME_RELEASE_ALL"*
sh ./"${INSTALL_FILE}" ./"${ARCHIVE_FILE}"
cd ..
# Remove build release
rm -rf "$NAME_RELEASE_ALL"*

exit 0
