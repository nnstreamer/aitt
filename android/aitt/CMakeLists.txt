cmake_minimum_required(VERSION 3.4.1)
project("aitt-android" CXX)

if(NOT DEFINED PROJECT_ROOT_DIR)
    set(PROJECT_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)
endif(NOT DEFINED PROJECT_ROOT_DIR)

find_library(ALOG NAMES log)
set(LOG_LIBRARIES ${ALOG})

add_subdirectory(${PROJECT_ROOT_DIR} ${CMAKE_BINARY_DIR}/aitt)

include_directories(${PROJECT_ROOT_DIR}/include)

set(ANDROID_SRC src/main/jni/aitt_jni.cc)

add_library(${PROJECT_NAME} SHARED ${ANDROID_SRC})

target_link_libraries(${PROJECT_NAME} aitt ${LOG_LIBRARIES})

add_dependencies(${PROJECT_NAME} aitt)
