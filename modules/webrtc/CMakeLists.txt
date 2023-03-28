set(AITT_WEBRTC aitt-stream-webrtc)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

pkg_check_modules(AITT_WEBRTC_NEEDS REQUIRED
    capi-media-camera
    capi-media-webrtc
    gstreamer-video-1.0
    json-glib-1.0
)
include_directories(${AITT_WEBRTC_NEEDS_INCLUDE_DIRS})
link_directories(${AITT_WEBRTC_NEEDS_LIBRARY_DIRS})

add_library(WEBRTC_OBJ OBJECT
    StreamManager.cc
    SrcStreamManager.cc
    SinkStreamManager.cc
    WebRtcMessage.cc
    WebRtcState.cc
    WebRtcStream.cc
    RequestServer.cc
)
add_library(${AITT_WEBRTC} SHARED $<TARGET_OBJECTS:WEBRTC_OBJ> Module.cc ../stream_entry.cc )
target_link_libraries(${AITT_WEBRTC} ${AITT_WEBRTC_NEEDS_LIBRARIES} ${AITT_COMMON})
target_compile_options(${AITT_WEBRTC} PUBLIC ${AITT_WEBRTC_NEEDS_CFLAGS_OTHER})

install(TARGETS ${AITT_WEBRTC} DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(BUILD_TESTING)
    add_subdirectory(tests)
endif(BUILD_TESTING)
