#!/bin/sh

if [ "$#" = "0" ] ; then
  echo usage: $0 INSTALL_DIR
  exit 1
fi

dest_dir="$1"
root_build_dir="build-ios"
rm -rf $root_build_dir || exit 1

# Usage:
#   build SDK ABI
#
# SDK:
#   "iphoneos"
#   "iphonesimulator"
#
# ABI:
#   "arm64"
#   "x86_64"
#
# Note that the combination of iphoneos and x86_64 makes no sense so far.
build() {
    sdk="$1"
    abi="$2"
    build_dir="$root_build_dir/$sdk-$abi"

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
    # Note that although it is possible to specify
    #   -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
    # and create the fat file at once, we will not do so here because we will
    # compile with different configurations for each architecture. Instead, we
    # will use lipo to create the fat file with non-fat files later.
    cmake -S . -B $build_dir -G Xcode \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_SYSTEM_NAME="iOS" \
        -DCMAKE_OSX_ARCHITECTURES="$abi" \
        -DCMAKE_OSX_DEPLOYMENT_TARGET=13.0 \
        -DCMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH=NO \
        -DCMAKE_IOS_INSTALL_COMBINED=NO \
        -DCMAKE_POSITION_INDEPENDENT_CODE="ON" \
        -DCMAKE_INSTALL_PREFIX:PATH="$PWD/$build_dir/exports" \
        -DBUILD_TESTSUITE=OFF \
        || exit 1
    # Note that the following command does not work:
    #   cmake --install $build_dir
    cmake --build $build_dir --target install --config Release -v \
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
#   install_include SDK ABI
install_include() {
    sdk="$1"
    abi="$2"
    from="$root_build_dir/$sdk-$abi/exports/include"
    to="$dest_dir/$sdk/include"
    mkdir -p $to || exit 1
    (cd $from && find . -type d -print0) \
        | xargs -0 -I "{}" install -m 755 -d "${to}/{}" || exit 1
    (cd $from && find . -type f -print0) \
        | xargs -0 -I "{}" install -m 644 "$from/{}" "${to}/{}" || exit 1
}

combo_list="iphoneos-arm64 iphonesimulator-arm64 iphonesimulator-x86_64"

for combo in $combo_list ; do
    sdk=${combo%-*}
    abi=${combo#*-}
    build $sdk $abi || exit 1
done

for combo in $combo_list ; do
    sdk=${combo%-*}
    abi=${combo#*-}
    build_dir="$root_build_dir/$sdk-$abi"
    lipo -info $build_dir/exports/lib/libmimicssl-aes128-cbc-decrypt.a || exit 1
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

sdk_list="iphoneos iphonesimulator"
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
