#!/bin/sh

if [ "$#" = 0 ] ; then
    echo usage: $0 BUILD_DIR configure ABI [CMAKE_OPTIONS...]
    echo usage: $0 BUILD_DIR test ABI
    exit 1
fi
root_build_dir="$1"
shift

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

check_abi() {
    abi=$1
    case "$abi" in
    x86|x86_64|arm64-v8a|armeabi-v7a)
        ;;
    *)
        echo unknown ABI: $abi
        exit 1
    esac
}

if [ "$#" = "0" ] ; then
    echo COMMAND not specified
    exit 1
fi
command="$1"
shift

case "$command" in
configure)
    if [ "$#" = "0" ] ; then
        echo ABI not specified
        exit 1
    fi
    abi="$1"
    shift
    check_abi $abi
    build_dir="$root_build_dir/$abi"
    rm -rf $build_dir || exit 1
    if [ "$abi" = "armeabi-v7a" ] ; then
        EXTRA_ARGS='-DCMAKE_ANDROID_ARM_NEON=ON'
    fi
    cmake -S . -B "$build_dir" -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_SYSTEM_NAME=Android \
        -DCMAKE_CROSSCOMPILING_EMULATOR="$PWD/emulator-android.sh" \
        -DCMAKE_SYSTEM_VERSION="21" \
        -DCMAKE_ANDROID_ARCH_ABI="$abi" \
        -DCMAKE_ANDROID_NDK="$ANDROID_NDK" \
        $EXTRA_ARGS "$@" || exit 1
    ;;
test)
    if [ "$#" = "0" ] ; then
        echo ABI not specified
        exit 1
    fi
    abi="$1"
    shift
    check_abi $abi
    build_dir="$root_build_dir/$abi"
    sh emulator-android.sh --adb-push "$build_dir/testsuite"
    ctest --test-dir "$build_dir" -C Release -V || exit 1
    sh emulator-android.sh --adb-pull "$build_dir/testsuite"
    ;;
*)
    echo unknown command: $command
    exit 1
esac
exit 0
