#!/bin/sh

if [ "$#" = "0" ] ; then
    echo usage: $0 BUILD_DIR configure [CMAKE_OPTIONS...]
    echo usage: $0 BUILD_DIR build [CMAKE_OPTIONS...]
    echo usage: $0 BUILD_DIR install INSTALL_DIR [CMAKE_OPTIONS...]
    echo usage: $0 BUILD_DIR test ARCH [CTEST_OPTOINS...]
    echo usage: $0 BUILD_DIR build-non-fat SDK ARCH BUILD_TYPE [CMAKE_OPTIONS...]
    exit 1
fi
root_build_dir="$1"
shift

# Usage:
#   configure SDK ARCH [CMAKE_OPTIONS...]
#
# SDK:
#   "iphoneos"
#   "iphonesimulator"
#
# ARCH:
#   "arm64"
#   "x86_64"
#
# Note that the combination of iphoneos and x86_64 makes no sense so far.
configure() {
    sdk="$1"
    arch="$2"
    shift 2
    build_dir="$root_build_dir/$sdk-$arch"
    install_dir="$build_dir/exports"
    mkdir -p "$install_dir"
    abs_install_dir="$(cd $install_dir && pwd)"
    # Note that although it is possible to specify
    #   -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
    # and create the fat file at once, we will not do so here because we will
    # compile with different configurations for each architecture. Instead, we
    # will use lipo to create the fat file with non-fat files later.
    cmake -S . -B "$build_dir" -G Xcode \
        -DCMAKE_SYSTEM_NAME="iOS" \
        -DCMAKE_OSX_ARCHITECTURES="$arch" \
        -DCMAKE_OSX_DEPLOYMENT_TARGET=13.0 \
        -DCMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH=NO \
        -DCMAKE_IOS_INSTALL_COMBINED=NO \
        -DCMAKE_INSTALL_PREFIX:PATH="$abs_install_dir" \
        -DCMAKE_CROSSCOMPILING_EMULATOR="$PWD/emulator-ios.sh" \
        "$@" || exit 1
    # WORKAROUND for EFFECTIVE_PLATFORM_NAME
    sed -i .bak -e 's/\${EFFECTIVE_PLATFORM_NAME}/-'$sdk'/' \
        "$root_build_dir/$sdk-$arch/testsuite/testsuite[1]_include-Release.cmake" \
        "$root_build_dir/$sdk-$arch/testsuite/testsuite[1]_include-Debug.cmake"
}

# Usage:
#   print_destination SDK
print_destination() {
    sdk="$1"
    case "$sdk" in
    iphoneos)
        destination="generic/platform=iOS"
        ;;
    iphonesimulator)
        destination="generic/platform=iOS Simulator"
        ;;
    *)
        echo unknown SDK: $sdk
        exit 1
    esac
    echo $destination
}

# Usage:
#   build SDK ARCH [CMAKE_OPTIONS...]
#
# SDK:
#   "iphoneos"
#   "iphonesimulator"
#
# ARCH:
#   "arm64"
#   "x86_64"
#
# Note that the combination of iphoneos and x86_64 makes no sense so far.
build() {
    sdk="$1"
    arch="$2"
    shift 2
    build_dir="$root_build_dir/$sdk-$arch"
    destination="$(print_destination $sdk)"
    # Note that the following command does not work:
    #   cmake --install $build_dir
    cmake --build $build_dir -v "$@" \
        -- -sdk $sdk -destination "$destination" || exit 1
}

# Usage:
#   install_lib LIBRARY
#
# LIBRARY must be either a ".a" or ".dylib" file.
install_lib() {
    lib="$1"
    install $root_build_dir/iphoneos-arm64/exports/lib/$lib \
        $dest_dir/iphoneos/lib/ || exit 1
    rm -f $dest_dir/iphonesimulator/lib/$lib
    lipo -create \
        $root_build_dir/iphonesimulator-arm64/exports/lib/$lib \
        $root_build_dir/iphonesimulator-x86_64/exports/lib/$lib \
        -output $dest_dir/iphonesimulator/lib/$lib || exit 1
}

# Usage:
#   install_include SDK ARCH
install_include() {
    sdk="$1"
    arch="$2"
    from="$root_build_dir/$sdk-$arch/exports/include"
    to="$dest_dir/$sdk/include"
    mkdir -p $to || exit 1
    (cd $from && find . -type d -print0) \
        | xargs -0 -I "{}" install -m 755 -d "${to}/{}" || exit 1
    (cd $from && find . -type f -print0) \
        | xargs -0 -I "{}" install -m 644 "$from/{}" "${to}/{}" || exit 1
}

# Usage:
#   cmake_install SDK ARCH [CMAKE_OPTIONS...]
#
# SDK:
#   "iphoneos"
#   "iphonesimulator"
#
# ARCH:
#   "arm64"
#   "x86_64"
#
# Note that the combination of iphoneos and x86_64 makes no sense so far.
cmake_install() {
    sdk="$1"
    arch="$2"
    shift 2
    build_dir="$root_build_dir/$sdk-$arch"
    destination="$(print_destination $sdk)"
    # Note that the following command does not work:
    #   cmake --install $build_dir
    cmake --build $build_dir --target install -v "$@" \
        -- -sdk $sdk -destination "$destination" || exit 1
}

check_sdk() {
    sdk="$1"
    case "$sdk" in
    iphoneos|iphonesimulator)
        ;;
    *)
        echo unknown SDK: $sdk
        exit 1
    esac
}

check_arch() {
    arch="$1"
    case "$arch" in
    arm64|x86_64)
        ;;
    *)
        echo unknown ARCH: $arch
        exit 1
    esac
}

check_build_type() {
    type="$1"
    case "$type" in
    Release|Debug)
        ;;
    *)
        echo unknown BUILD_TYPE: $arch
        exit 1
    esac
}

if [ "$#" = "0" ] ; then
    echo COMMAND not specified
    exit 1
fi
command="$1"
shift

sdk_list="iphoneos iphonesimulator"
all="iphoneos-arm64 iphonesimulator-arm64 iphonesimulator-x86_64"

case "$command" in
build-non-fat)
    if [ "$#" -lt 3 ] ; then
        echo SDK, ARCH, and BUILD_TYPE not specified
        exit 1
    fi
    sdk="$1"
    arch="$2"
    build_type="$3"
    shift 3
    check_sdk $sdk
    check_arch $arch
    check_build_type $build_type
    rm -rf $root_build_dir || exit 1
    configure $sdk $arch "$@" || exit 1
    build $sdk $arch --config "$build_type" || exit 1
    ;;
configure)
    rm -rf $root_build_dir || exit 1
    for i in $all ; do
        sdk=${i%-*}
        arch=${i#*-}
        configure $sdk $arch "$@" || exit 1
    done
    ;;
build)
    for i in $all ; do
        sdk=${i%-*}
        arch=${i#*-}
        build $sdk $arch "$@" || exit 1
    done
    ;;
install)
    if [ "$#" = "0" ] ; then
        echo INSTALL_DIR not specified
        exit 1
    fi
    dest_dir="$1"
    shift
    for i in $all ; do
        sdk=${i%-*}
        arch=${i#*-}
        cmake_install $sdk $arch "$@" || exit 1
    done
    mkdir -p $dest_dir \
        $dest_dir/iphoneos/include \
        $dest_dir/iphoneos/lib \
        $dest_dir/iphonesimulator/include \
        $dest_dir/iphonesimulator/lib \
        $dest_dir/xcframeworks || exit 1
    staticlib=libmimicssl-aes128-cbc-decrypt.a
    sharedlib=libmimicssl-aes128-cbc-decrypt.dylib
    for lib in $staticlib $sharedlib ; do
        install_lib $lib || exit 1
    done
    for sdk in $sdk_list ; do
        # Here, we consider header files of the "iphonesimulator" identical
        # regardless of the architecture.
        install_include $sdk arm64 || exit 1
    done
    xcframework=mimicssl-aes128-cbc-decrypt.xcframework
    rm -rf $dest_dir/xcframeworks/$xcframework
    xcodebuild -create-xcframework \
    -library $dest_dir/iphoneos/lib/$staticlib \
        -headers $dest_dir/iphoneos/include \
    -library $dest_dir/iphonesimulator/lib/$staticlib \
        -headers $dest_dir/iphonesimulator/include \
    -output $dest_dir/xcframeworks/$xcframework || exit 1
    ;;
test)
    if [ "$#" = "0" ] ; then
        echo ARCH not specified
        exit 1
    fi
    arch="$1"
    shift
    check_arch $arch
    runtime=$(xcrun simctl list runtimes iOS -j \
        | jq '.runtimes[].identifier' | tail -1)
    udid=$(xcrun simctl list devices iPhone available -j \
        | jq '.devices['$runtime']|.[].udid' | tail -1)
    xcrun simctl bootstatus $(eval echo $udid) -b
    ctest --test-dir "$root_build_dir/iphonesimulator-$arch" "$@"
    ;;
*)
    echo unknown command: $command
    exit 1
esac
