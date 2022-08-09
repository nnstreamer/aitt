if(CMAKE_VERSION VERSION_LESS "3.10.0")
    if(DEFINED AITT_ANDROID_GLIB)
        return()
    endif()
    set(AITT_ANDROID_GLIB TRUE)
else(CMAKE_VERSION VERSION_LESS "3.10.0")
    include_guard(GLOBAL)
endif(CMAKE_VERSION VERSION_LESS "3.10.0")

if(ANDROID_ABI STREQUAL "arm64-v8a")
    set(GSTREAMER_ABI arm64)
elseif(ANDROID_ABI STREQUAL "armeabi-v7a")
    set(GSTREAMER_ABI armv7)
else(ANDROID_ABI STREQUAL "armeabi-v7a")
    set(GSTREAMER_ABI ${ANDROID_ABI})
endif(ANDROID_ABI STREQUAL "arm64-v8a")

include_directories(
        ${GSTREAMER_ROOT_ANDROID}/${GSTREAMER_ABI}/include/glib-2.0
        ${GSTREAMER_ROOT_ANDROID}/${GSTREAMER_ABI}/lib/glib-2.0/include
)

link_directories(${GSTREAMER_ROOT_ANDROID}/${GSTREAMER_ABI}/lib)

set(GLIB_LIBRARIES ${GSTREAMER_ROOT_ANDROID}/${GSTREAMER_ABI}/lib/libglib-2.0.a
        ${GSTREAMER_ROOT_ANDROID}/${GSTREAMER_ABI}/lib/libiconv.a
        ${GSTREAMER_ROOT_ANDROID}/${GSTREAMER_ABI}/lib/libintl.a)
