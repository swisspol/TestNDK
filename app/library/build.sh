#!/bin/sh -ex

export ANDROID_NDK="$HOME/Library/Android/sdk/ndk-bundle"

export NDK_PLATFORM="$ANDROID_NDK/platforms/android-21/arch-arm"
export NDK_TOOLCHAIN="$ANDROID_NDK/toolchains/arm-linux-androideabi-4.9/prebuilt/darwin-x86_64"

export CC="$NDK_TOOLCHAIN/bin/arm-linux-androideabi-gcc"
export LD="$NDK_TOOLCHAIN/bin/arm-linux-androideabi-ld"
export AR="$NDK_TOOLCHAIN/bin/arm-linux-androideabi-ar"

rm -f "test.o" "libtest.a"

$CC --sysroot="$NDK_PLATFORM" -c "test.c" -o "test.o"
$AR rcs "libtest.a" "test.o"

echo "Done!"
