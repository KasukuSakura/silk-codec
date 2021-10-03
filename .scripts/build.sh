cd native || exit 1

rm -rf cmake-build-release
mkdir cmake-build-release
cd cmake-build-release || exit 1

cmake -DCMAKE_BUILD_TYPE=Release "-DCMAKE_CACHEFILE_DIR=$PWD" "-Dcross_triple=$CROSS" ..

cmake --build . --target silk -- -j 3
cmake --build . --target silk_codec -- -j 3

ecode=$?

mkdir bin

ls -la

cp "silk.dll" bin
cp "silk.so" bin
cp "silk.dylib" bin

cp "libsilk.dll" -T bin/silk.dll
cp "libsilk.so" -T bin/silk.so
mv "libsilk.dylib" bin/silk.dylib

cp "silk_codec.exe" bin
cp "silk_codec" bin

ls -la

exit $ecode
