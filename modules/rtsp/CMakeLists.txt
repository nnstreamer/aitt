if(PLATFORM STREQUAL "tizen")
    set(AITT_RTSP aitt-stream-rtsp)

    include_directories(${CMAKE_CURRENT_SOURCE_DIR})
    pkg_check_modules(AITT_RTSP_NEEDS REQUIRED
        capi-media-player
        capi-media-image-util
    )
    include_directories(${AITT_RTSP_NEEDS_INCLUDE_DIRS})
    link_directories(${AITT_RTSP_NEEDS_LIBRARY_DIRS})

    file(GLOB AITT_RTSP_SRC *.cc)
    list(REMOVE_ITEM AITT_RTSP_SRC ${CMAKE_CURRENT_SOURCE_DIR}/Module.cc)
    add_library(RTSP_OBJ OBJECT ${AITT_RTSP_SRC})
    add_library(${AITT_RTSP} SHARED $<TARGET_OBJECTS:RTSP_OBJ> Module.cc ../stream_entry.cc)
    target_link_libraries(${AITT_RTSP} ${AITT_RTSP_NEEDS_LIBRARIES} ${AITT_COMMON})
    target_compile_options(${AITT_RTSP} PUBLIC ${AITT_RTSP_NEEDS_CFLAGS_OTHER})
    install(TARGETS ${AITT_RTSP} DESTINATION ${CMAKE_INSTALL_LIBDIR})
else(PLATFORM STREQUAL "tizen")
    message ( WARNING "Required to write CMakefile for each Platform" )
endif(PLATFORM STREQUAL "tizen")
