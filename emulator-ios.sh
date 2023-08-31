#!/bin/sh

# Usage:
#   emulator-ios.sh TESTCASE [ARGS...]

LOG="log.txt"

setupLog() {
    echo "---" >> $LOG
    echo args: "$@" >> $LOG
    echo pwd: $PWD >> $LOG
    echo log: >> $LOG
}

setupLog "$@"
name="$1"
if [ "${name##*/}" != "testsuite" ] ; then
    echo unexpected testcase >> $LOG
    exit 1
fi
shift
id="mimicssl-aes128-cbc-decrypt-testsuite"
app="${name%/*}"

echo xcrun simctl install booted $app >> $LOG
xcrun simctl install booted $app || exit 1

echo xcrun simctl launch --console booted $id "$@" >> $LOG
xcrun simctl launch --console booted $id "$@" || exit 1

echo xcrun simctl uninstall booted $id >> $LOG
xcrun simctl uninstall booted $id || exit 1
