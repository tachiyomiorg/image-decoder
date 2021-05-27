#!/bin/bash

while getopts "a:" opt; do
  case $opt in
    a)
      ARCH=$OPTARG ;;
    :)
      echo "Option -$OPTARG requires an argument." >&2
      exit 1
      ;;
  esac
done

if [[ -z "${ARCH}" ]] ; then
  echo 'You need to input arch with -a ARCH.'
  echo 'Supported archs are:'
  echo -e '\tarm arm64 x86 x86_64'
  exit 1
elif [[ -z "${ANDROID_NDK}" ]] ; then
  echo "You need to provide the ANDROID_NDK environment variable"
  exit 1
fi

ANDROID_API=21

case "${ARCH}" in
  'arm')
    ARCH_TRIPLET='arm-linux-androideabi'
    ARCH_TRIPLET_VARIANT='armv7a-linux-androideabi'
    ABI='armeabi-v7a'
    CPU_FAMILY='arm'
    ARCH_CFLAGS='-march=armv7-a -mfpu=neon -mfloat-abi=softfp -mthumb'
    ARCH_LDFLAGS='-march=armv7-a -Wl,--fix-cortex-a8'
    B_ARCH='arm'
    B_ADDRESS_MODEL=32 ;;
  'arm64')
    ARCH_TRIPLET='aarch64-linux-android'
    ARCH_TRIPLET_VARIANT=$ARCH_TRIPLET
    ABI='arm64-v8a'
    CPU_FAMILY='aarch64'
    B_ARCH='arm'
    B_ADDRESS_MODEL=64 ;;
  'x86')
    ARCH_TRIPLET='i686-linux-android'
    ARCH_TRIPLET_VARIANT=$ARCH_TRIPLET
    ARCH_CFLAGS='-march=i686 -mtune=intel -mssse3 -mfpmath=sse -m32'
    ABI='x86'
    CPU_FAMILY='x86'
    B_ARCH='x86'
    B_ADDRESS_MODEL=32 ;;
  'x86_64')
    ARCH_TRIPLET='x86_64-linux-android'
    ARCH_TRIPLET_VARIANT=$ARCH_TRIPLET
    ABI='x86_64'
    CPU_FAMILY='x86_64'
    ARCH_CFLAGS='-march=x86-64 -msse4.2 -mpopcnt -m64 -mtune=intel'
    B_ARCH='x86'
    B_ADDRESS_MODEL=64 ;;
  *)
    echo "Arch ${ARCH} is not supported."
    exit 1 ;;
esac

os=$(uname -s | tr '[:upper:]' '[:lower:]')
CROSS_PREFIX="${ANDROID_NDK}"/toolchains/llvm/prebuilt/${os}-x86_64/bin

set -eu

echo "Generating toolchain description..."
user_config=android_cross.txt
rm -f $user_config
cat <<EOF > $user_config
[binaries]
name = 'android'
c     = '${CROSS_PREFIX}/${ARCH_TRIPLET_VARIANT}${ANDROID_API}-clang'
cpp   = '${CROSS_PREFIX}/${ARCH_TRIPLET_VARIANT}${ANDROID_API}-clang++'
ar    = '${CROSS_PREFIX}/${ARCH_TRIPLET}-ar'
ld    = '${CROSS_PREFIX}/${ARCH_TRIPLET}-ld'
strip = '${CROSS_PREFIX}/${ARCH_TRIPLET}-strip'

[properties]
needs_exe_wrapper = true

[host_machine]
system = 'linux'
cpu_family = '${CPU_FAMILY}'
cpu = '${CPU_FAMILY}'
endian = 'little'

EOF
