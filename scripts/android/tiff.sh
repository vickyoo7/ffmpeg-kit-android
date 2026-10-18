#!/bin/bash

# ALWAYS CLEAN THE PREVIOUS BUILD
make distclean 2>/dev/null 1>/dev/null

# REGENERATE BUILD FILES IF NECESSARY OR REQUESTED
if [[ ! -f "${BASEDIR}"/src/"${LIB_NAME}"/configure ]] || [[ ${RECONF_tiff} -eq 1 ]]; then
  autoreconf_library "${LIB_NAME}" 1>>"${BASEDIR}"/build.log 2>&1 || return 1
fi

./configure \
  --prefix="${LIB_INSTALL_PREFIX}" \
  --with-pic \
  --with-sysroot="${ANDROID_SYSROOT}" \
  --with-jpeg-include-dir="${LIB_INSTALL_BASE}"/jpeg/include \
  --with-jpeg-lib-dir="${LIB_INSTALL_BASE}"/jpeg/lib \
  --enable-static \
  --disable-shared \
  --disable-fast-install \
  --disable-maintainer-mode \
  --disable-cxx \
  --disable-win32-io \
  --disable-lzma \
  --host="${HOST}" || return 1

# Android only needs the library payload. Building the desktop-oriented tool
# binaries pulls in extra file APIs that are unavailable on older API levels.
make -C port -j$(get_cpu_count) || return 1
make -C libtiff -j$(get_cpu_count) || return 1

make -C libtiff install || return 1

# MANUALLY COPY PKG-CONFIG FILES
cp ./*.pc "${INSTALL_PKG_CONFIG_DIR}" || return 1
