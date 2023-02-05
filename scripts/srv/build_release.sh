#!/bin/bash
set -e

# Prefix for message
PREFIX_ERRN="ERRN:"
PREFIX_INFO="INFO:"

# Check parameters
if [ ! -d "$1" ] || [ -z "$1" ]; then
  echo "$PREFIX_ERRN path '$1' is not folder or doesn't specified"
  exit 1
fi

# Directories predefined
SRC_PATH="$1"
LAST_LEXEME=$(echo "$SRC_PATH" | sed -n 's/\/$//p' )
if [ -z "$LAST_LEXEME" ]; then
  LAST_LEXEME="$SRC_PATH"
fi
# Current dir
ENTRY_DIR=$(pwd)

# Build folder
BUILD_NAME=$(echo "$LAST_LEXEME" | sed -n 's:.*/\(.*\)*.*:\1:p')
BUILD_DATE=$(date +%Y%m%d_%H%M%S)
BUILD_DIR="${ENTRY_DIR}/${BUILD_NAME}_build_${BUILD_DATE}"
CODE_COPY_SRC="${ENTRY_DIR}/$BUILD_NAME"
# Release folder
RELEASE_DIR="${BUILD_NAME}_release_${BUILD_DATE}"
# Archive file
ARCHIVE_FILE="${BUILD_NAME}.tar"
IMPORT_LIBS="${CODE_COPY_SRC}/scripts/srv/import_libs.sh"
# Install files
INSTALL_FILE="${CODE_COPY_SRC}/scripts/srv/install.sh"
CONFIG_SRC="${CODE_COPY_SRC}/config"
SCRIPTS_SRC="${CODE_COPY_SRC}/scripts"

EXE_FILE="rxsd"
EXE_FILE_SRC="${BUILD_DIR}/tools/server/${EXE_FILE}"
EXE_FILE_DST="${BUILD_DIR}/${EXE_FILE}"

# Compose paths for archive
BUILD_FILES="${EXE_FILE}"
BUILD_FILES="${BUILD_FILES} config/rxs_users"
BUILD_FILES="${BUILD_FILES} scripts/srv/rxsd.sh"

##########################################################################
#
##########################################################################
# Copy src code
cp -r "$SRC_PATH" "$CODE_COPY_SRC"
chown -hR "$(whoami)" "$CODE_COPY_SRC"

# Import include libs
cd "$CODE_COPY_SRC"
sh "${IMPORT_LIBS}" "$(pwd)"
cd "$ENTRY_DIR"

# Set version
FILE_VERSION="$CODE_COPY_SRC/include/protocol/version.h"
LEXEM_VERSION="git_version"
set +e
GIT_VERSION=$(git -C "$CODE_COPY_SRC" describe)
res=$?
set -e
if [ $res -ne 0 ]; then
    GIT_VERSION="na"
fi
BUILD_VERSION="${LEXEM_VERSION}[]=\"$GIT_VERSION\";"
# Replace version value
sed -i 's/'"$LEXEM_VERSION"'.*/'"$BUILD_VERSION"'/' "$FILE_VERSION"

# Compile
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
cmake "${CODE_COPY_SRC}/"
make
cd "$ENTRY_DIR"
# Rename file
mv "${EXE_FILE_SRC}" "${EXE_FILE_DST}"

# Move folders to
cp -r "${CONFIG_SRC}" "${BUILD_DIR}"
cp -r "${SCRIPTS_SRC}" "${BUILD_DIR}"

# Create archive
tar -C "${BUILD_DIR}" -cf "$ARCHIVE_FILE" $BUILD_FILES
# Release folder
mkdir -p "${RELEASE_DIR}"
mv "${ARCHIVE_FILE}" "${RELEASE_DIR}"
mv "${INSTALL_FILE}" "${RELEASE_DIR}"
# Delete folders
rm -rf "${CODE_COPY_SRC}"
rm -rf "${BUILD_DIR}"
# Output info
echo "${PREFIX_INFO} build '${RELEASE_DIR}' has been created"

exit 0
