# Build native

## Requirements

- Source code of this repository
- CMake
- C & CPP Compiler
- Little knowledge of C & CPP
- A search engine if compilation failed

## Build

```shell
cd native
mkdir cmake-build
cd cmake-build

cmake -DCMAKE_BUILD_TYPE=Release "-DCMAKE_CACHEFILE_DIR=$PWD" ..
cmake --build . --target silk_codec -- -j 3
```

## Use custom build

1. Create a folder (Here named it `/root/silk-native`)
2. Move native lib to `/root/silk-native/silk.so`
3. Edit java bootstrap command. Add `-Dsilk-codec.data-path=/root/silk-native`
4. Type `vim /root/silk-native/settings.properties`
5. Type following content into it

```properties
native.path=/root/silk-native/silk.so
```

> Remember rebuild native when updating silk-codec
>
> Version mismatch may crash java runtime
