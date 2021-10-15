cmake_minimum_required(VERSION 3.12)
project(native)
set(CMAKE_CXX_STANDARD 14)

IF (WIN32)
    MESSAGE("Windows with ${CMAKE_CXX_COMPILER_ID}")

    SET(NO_MINGW_LIB_FLAGS "-static-libgcc -static-libstdc++ -static")

    IF (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${NO_MINGW_LIB_FLAGS}")
        SET(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} ${NO_MINGW_LIB_FLAGS}")
    ENDIF()
    MESSAGE("CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
    MESSAGE("CMAKE_C  _FLAGS: ${CMAKE_C_FLAGS}")
ENDIF()

include_directories(jni_include/common)
IF (WIN32)
   include_directories(jni_include/inc_win)
ELSEIF (APPLE)
    include_directories(jni_include/inc_mac)
ELSEIF (UNIX)
    include_directories(jni_include/inc_linux)
ELSE()
    set(JAVA_AWT_LIBRARY NotNeeded)
    set(JAVA_JVM_LIBRARY NotNeeded)
    find_package(JNI REQUIRED)

    include_directories(${JAVA_INCLUDE_PATH})
    include_directories(${JAVA_INCLUDE_PATH2})
ENDIF ()


include_directories(../build/generated/sources/headers/java/main/)

#add_subdirectory(lame)

#SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/../build/classes/java/main/io/github/mzdluo123/silk4j)

include_directories(src/interface)
include_directories(src/silk)
include_directories(src/cxxopts/include)

AUX_SOURCE_DIRECTORY(src/silk SRC_DIR)
AUX_SOURCE_DIRECTORY(src/bind SRC_DIR)

AUX_SOURCE_DIRECTORY(src/silk_codec SILK_CODEC_EXEC)

set(CMAKE_ANDROID_ARM_MODE ON)
set(CMAKE_ANDROID_ARM_NEON ON)
set(CMAKE_ANDROID_STL_TYPE system)

add_library(silk SHARED ${SRC_DIR})
add_executable(silk_codec ${SRC_DIR} ${SILK_CODEC_EXEC})

