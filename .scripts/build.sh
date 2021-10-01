cd native || exit 1

rm -rf cmake-build-release
mkdir cmake-build-release
cd cmake-build-release || exit 1

cmake -DCMAKE_BUILD_TYPE=Release "-DCMAKE_CACHEFILE_DIR=$PWD" "-Dcross_triple=$CROSS" ..

cmake --build . --target silk -- -j 3
cmake --build . --target pcm_to_silk -- -j 3

ecode=$?

mkdir bin

ls -la

cp "silk.dll" bin
cp "silk.so" bin
cp "silk.dylib" bin

cp "pcm_to_silk.exe" bin
cp "pcm_to_silk" bin

exit $ecode
