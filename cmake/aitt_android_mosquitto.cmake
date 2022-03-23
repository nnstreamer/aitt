if(CMAKE_VERSION VERSION_LESS "3.10.0")
    if(DEFINED AITT_ANDROID_MOSQUITTO)
        return()
    endif()
    set(AITT_ANDROID_MOSQUITTO TRUE)
else(CMAKE_VERSION VERSION_LESS "3.10.0")
    include_guard(GLOBAL)
endif(CMAKE_VERSION VERSION_LESS "3.10.0")

include_directories(${PROJECT_ROOT_DIR}/third_party/mosquitto-2.0.14/include)

link_directories(${PROJECT_ROOT_DIR}/android/mosquitto/.cxx/cmake/debug/${ANDROID_ABI}/lib)

set(MOSQUITTO_LIBRARY ${PROJECT_ROOT_DIR}/android/mosquitto/.cxx/cmake/debug/${ANDROID_ABI}/lib/libmosquitto_static.a)
