#!/bin/sh

if [ "$#" = 0 ] ; then
  echo usage: $0 ABI [CMAKE_OPTIONS...]
  exit 1
fi

# Environment variables:
#   ANDROID_NDK (e.g., "$HOME/Library/Android/sdk/ndk/25.2.9519653")
#   ANDROID_HOME (e.g., "$HOME/Library/Android/sdk")

# ABI:
#   "x86"
#   "x86_64"
#   "arm64-v8a"
#   "armeabi-v7a"

# Options:
#   -DCMAKE_INSTALL_PREFIX:PATH="$HOME/android/$ABI"
#   -DBUILD_TESTSUITE=OFF

ABI=$1
if [ "$ABI" = "armeabi-v7a" ] ; then
    EXTRA_ARGS='-DCMAKE_ANDROID_ARM_NEON=ON'
fi
shift
rm -rf "build-$ABI" || exit 1
cmake -S . -B "build-$ABI" -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_SYSTEM_NAME=Android \
    -DCMAKE_CROSSCOMPILING_EMULATOR="$PWD/emulator-android.sh" \
    -DCMAKE_SYSTEM_VERSION="21" \
    -DCMAKE_ANDROID_ARCH_ABI="$ABI" \
    -DCMAKE_ANDROID_NDK="$ANDROID_NDK" \
    $EXTRA_ARGS "$@" || exit 1
cmake --build "build-$ABI" --config Release -v || exit 1
if test -f "build-$ABI/testsuite/testsuite" ; then
    sh emulator-android.sh --adb-push "build-$ABI/testsuite"
    ctest --test-dir "build-$ABI" -C Release -V || exit 1
    sh emulator-android.sh --adb-pull "build-$ABI/testsuite"
fi
