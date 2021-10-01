cmake_minimum_required(VERSION 3.12)
project(native)
set(CMAKE_CXX_STANDARD 11)


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

AUX_SOURCE_DIRECTORY(src/silk SRC_DIR)
AUX_SOURCE_DIRECTORY(src/bind SRC_DIR)

AUX_SOURCE_DIRECTORY(src/pcm_to_silk PCM_TO_SILK)

set(CMAKE_ANDROID_ARM_MODE ON)
set(CMAKE_ANDROID_ARM_NEON ON)
set(CMAKE_ANDROID_STL_TYPE system)

add_library(silk SHARED ${SRC_DIR})
add_executable(pcm_to_silk ${SRC_DIR} ${PCM_TO_SILK})

