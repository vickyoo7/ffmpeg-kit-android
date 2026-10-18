#!/bin/bash

# ALWAYS CLEAN THE PREVIOUS BUILD
make distclean 2>/dev/null 1>/dev/null

# REGENERATE BUILD FILES IF NECESSARY OR REQUESTED
if [[ ! -f "${BASEDIR}"/src/"${LIB_NAME}"/configure ]] || [[ ${RECONF_libuuid} -eq 1 ]]; then
  autoreconf_library "${LIB_NAME}" 1>>"${BASEDIR}"/build.log 2>&1 || return 1
fi

./configure \
  --prefix="${LIB_INSTALL_PREFIX}" \
  --with-pic \
  --with-sysroot="${ANDROID_SYSROOT}" \
  --enable-static \
  --disable-shared \
  --disable-fast-install \
  --host="${HOST}" || return 1

if ! grep -q '^#include "config.h"' gen_uuid.c; then
  ${SED_INLINE} '/#define _SVID_SOURCE/a\
#ifdef HAVE_CONFIG_H\
#include "config.h"\
#endif' gen_uuid.c || return 1
fi

if ! grep -q 'int flock(int __fd, int __op);' gen_uuid.c; then
  ${SED_INLINE} '/#include <sys\/file.h>/a\
#if defined(__ANDROID__)\
int flock(int __fd, int __op);\
#endif' gen_uuid.c || return 1
fi

make -j$(get_cpu_count) || return 1

make install || return 1

# CREATE PACKAGE CONFIG MANUALLY
create_uuid_package_config "1.0.3" || return 1
