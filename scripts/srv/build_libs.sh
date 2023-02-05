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
  exit 1
fi

##########################################################################
# Directories predefined
##########################################################################
SRC_PATH="$1"

LIB_X_NAME="librxs_protocol.so"
#LIB_Y_NAME="librxs_container.so"
LIB_X_SRC="./lib/protocol/${LIB_X_NAME}"
#LIB_Y_SRC="./lib/container/${LIB_Y_NAME}"
LD_CONF="/etc/ld.so.conf"
DST_PATH="/usr/local/lib/rxs"

# cmake
cmake -DBUILD_SHARED_LIBS=ON "${SRC_PATH}"
# make
make
# Check directory
if [ ! -d "${DST_PATH}" ]; then
  sudo mkdir -p "${DST_PATH}"
fi

#chown $(whoami) $DST_PATH
# Copy lib
sudo cp "${LIB_X_SRC}" "${DST_PATH}"
# Copy lib
#sudo cp "${LIB_Y_SRC}" "${DST_PATH}"
##########################################################################
# Add the library to ldconfig and update the dynamic linking
##########################################################################
# Add to file "/etc/ld.so.conf" libs
sudo sed -i 'a '$DST_PATH'' "$LD_CONF"
# Refresh libs
sudo ldconfig
##########################################################################
# Output string
##########################################################################
echo "##########################################################################"
echo "$PREFIX_INFO libs: '$LIB_X_NAME' has been installed to"
echo "     '$DST_PATH' successfully"
