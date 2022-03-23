if(CMAKE_VERSION VERSION_LESS "3.10.0")
    if(DEFINED AITT_ANDROID_FLATBUFFERS)
        return()
    endif()
    set(AITT_ANDROID_FLATBUFFERS TRUE)
else(CMAKE_VERSION VERSION_LESS "3.10.0")
    include_guard(GLOBAL)
endif(CMAKE_VERSION VERSION_LESS "3.10.0")

include_directories(${PROJECT_ROOT_DIR}/third_party/flatbuffers-2.0.0/include)

link_directories(${PROJECT_ROOT_DIR}/android/flatbuffers/.cxx/cmake/debug/${ANDROID_ABI})

set(FLATBUFFERS_LIBRARY ${PROJECT_ROOT_DIR}/android/flatbuffers/.cxx/cmake/debug/${ANDROID_ABI}/libflatbuffers.a)
