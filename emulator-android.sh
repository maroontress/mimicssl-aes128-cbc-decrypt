#!/bin/sh

# Usage:
#   emulator-android.sh --adb-push DIR
#   emulator-android.sh --adb-pull DIR
#   emulator-android.sh TESTCASE [ARGS...]

ADB="$ANDROID_HOME/platform-tools/adb"
REMOTE_DIR="/data/local/tmp"
LOG="log.txt"

setupLog() {
    echo "---" >> $LOG
    echo args: "$@" >> $LOG
    echo pwd: $PWD >> $LOG
    echo log: >> $LOG
}

if [ "$1" = "--adb-push" ] ; then
    cd "$2"
    setupLog "$@"
    $ADB push alice.md.decrypted $REMOTE_DIR/alice.md.decrypted >> $LOG
    $ADB push alice.md.encrypted $REMOTE_DIR/alice.md.encrypted >> $LOG
    exit
fi
if [ "$1" = "--adb-pull" ] ; then
    cd "$2"
    setupLog "$@"
    echo "--adb-pull: do nothing" >> $LOG
    exit
fi

setupLog "$@"
name="$1"
if [ "${name##*/}" != "testsuite" ] ; then
    echo unexpected testcase >> $LOG
    exit 1
fi
shift
if [ "$1" = "--gtest_list_tests" ] ; then
    echo devices: >> $LOG
    $ADB devices >> $LOG
    $ADB push testsuite $REMOTE_DIR/testsuite >> $LOG
    $ADB shell chmod +x $REMOTE_DIR/testsuite >> $LOG
fi

n="'"
args=$n$1$n
shift
for i in "$@" ; do
    args="$args $n$i$n"
done
echo $ADB shell "cd $REMOTE_DIR && ./testsuite $args" >> $LOG
$ADB shell "cd $REMOTE_DIR && ./testsuite $args"
