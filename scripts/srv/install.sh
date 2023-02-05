#!/bin/bash
set -e
# Prefix for message
PREFIX_ERRN="ERRN:"
PREFIX_INFO="INFO:"

# Check parameters
if [ ! -f "$1" ] || [ -z "$1" ]; then
  echo "${PREFIX_ERRN} path '$1' is not file or doesn't specified"
  echo "${PREFIX_ERRN} Example:'$ install.sh ./distr.tar'"
  exit 1
fi

# Archive file
ARCHIVE_FILE="$1"
# Entry point
ENTRY_DIR=$(pwd)
BUILD_DIR="${ENTRY_DIR}/$(date +%Y%m%d_%H%M%S)"

CONFIG_DIR_SRC="${BUILD_DIR}/config" # Folder configuration source
CONFIG_FILE_01="rxs_users" # File users config
CONFIG_DIR_DST="/etc/rxs" # Folder config
CONFIG_FILE_SRC="${CONFIG_DIR_SRC}/${CONFIG_FILE_01}"
CONFIG_FILE_DST="${CONFIG_DIR_DST}/${CONFIG_FILE_01}"

SCRIPTS_DIR_SRC="${BUILD_DIR}/scripts/srv" # Folder scripts source
SCRIPT_FILE_01="rxsd.sh" # File script
SCRIPTS_DIR_DST="/etc/rc.d/init.d"
SCRIPT_FILE_SRC="${SCRIPTS_DIR_SRC}/${SCRIPT_FILE_01}"
SCRIPT_FILE_DST="${SCRIPTS_DIR_DST}/${SCRIPT_FILE_01}"

INSTALL_DIR="/usr/sbin" # Folder install
EXE_FILE="rxsd" # File binary
EXE_FILE_SRC="$BUILD_DIR/$EXE_FILE"
EXE_FILE_DST="${INSTALL_DIR}/${EXE_FILE}"
##########################################################################
#
##########################################################################
# Folder for install
if [ ! -d "$INSTALL_DIR" ]; then
  sudo mkdir -p "$INSTALL_DIR"
fi
# Folder for scripts
if [ ! -d "$SCRIPTS_DIR_DST" ]; then
  sudo mkdir -p "$SCRIPTS_DIR_DST"
fi
# Folder for config
if [ ! -d "$CONFIG_DIR_DST" ]; then
  sudo mkdir -p "$CONFIG_DIR_DST"
fi
# Install
mkdir -p "$BUILD_DIR"
# Extract archive
tar xf "${ARCHIVE_FILE}" -C "${BUILD_DIR}"
cd "$BUILD_DIR"

# Set owner
sudo chown -R "$(whoami)" "${CONFIG_DIR_DST}"
sudo chown -R "$(whoami)" "${SCRIPTS_DIR_SRC}"
sudo chown -R "$(whoami)" "${INSTALL_DIR}"

# Copy configs files
cp "${CONFIG_FILE_SRC}" "${CONFIG_FILE_DST}"
# Copy scripts files
cp "${SCRIPT_FILE_SRC}" "${SCRIPT_FILE_DST}"
# Copy binary file
cp "$EXE_FILE_SRC" "$EXE_FILE_DST"

# Set permissions
# Binary file
sudo chmod a+rwx "$EXE_FILE_DST"
# Configs
sudo chmod -R a+rwx "$CONFIG_DIR_DST"
# Scripts
sudo chmod a+rwx "$SCRIPT_FILE_DST"

# Delete build folder
cd "$ENTRY_DIR"
sudo rm -rf "$BUILD_DIR"

# Examine binary file
RED='\033[0;31m'
GREEN='\033[0;32m'
NOCOLOR='\033[0m' # No Color
# Wait while background process will be started
( exec "${EXE_FILE_DST}">/dev/null; wait ) &
C_PID=$!
#P_PID=$$
if [ -e "$EXE_FILE_DST" ]; then
  sleep 0.1
set +e
  kill $C_PID
set -e
  res=$?
  # process found
  if [ $res -eq 0 ]; then
    echo "$PREFIX_INFO ${GREEN}File has been executed${NOCOLOR}"
  # process not found
  else
    # return code exit from child
    wait "$C_PID"
    status=$?
    # exit with code 'No such file or directory'
    if [ $status -eq 2 ] || [ $status -eq 127 ]; then
      echo "$PREFIX_ERRN ${RED}File hasn't been executed${NOCOLOR}"
    # exit with code 0 or code have values: 'Invalid argument', etc
    else
      echo "$PREFIX_INFO ${GREEN}File has been executed${NOCOLOR}"
    fi
  fi
else
  echo "$PREFIX_ERRN ${RED}binary file hasn't been executed${NOCOLOR}"
fi

# Output info
echo "${PREFIX_INFO} File has been installed to '$EXE_FILE_DST'"
echo "${PREFIX_INFO} Config files has been installed to '${CONFIG_FILE_DST}'"
echo "${PREFIX_INFO} Script files has been installed to '${SCRIPT_FILE_DST}'"
exit 0
