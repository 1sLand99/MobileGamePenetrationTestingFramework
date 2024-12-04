clear
echo "Building arm64 "
mkdir -p cmake-build64
cd cmake-build64
cmake \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=${NDK_PATH}/build/cmake/android.toolchain.cmake \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=android-19 \
    -DANDROID_NDK=${NDK_PATH} \
    -DANDROID_STL=c++_static \
    ..
ninja -j16
adb push libMyHook.so /data/local/tmp/arm64-v8a
cd ..

echo "Building Done"