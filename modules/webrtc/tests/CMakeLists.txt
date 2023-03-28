include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)
include_directories(${CMAKE_BINARY_DIR}/src)

pkg_check_modules(UT_NEEDS REQUIRED gmock_main)
include_directories(${UT_NEEDS_INCLUDE_DIRS})
link_directories(${UT_NEEDS_LIBRARY_DIRS})

set(AITT_WEBRTC_UT ${PROJECT_NAME}_webrtc_ut)
set(AITT_WEBRTC_UT_SRC WEBRTC_test.cc ../Module.cc ${CMAKE_BINARY_DIR}/src/MosquittoMQ.cc)

add_executable(${AITT_WEBRTC_UT} ${AITT_WEBRTC_UT_SRC} $<TARGET_OBJECTS:WEBRTC_OBJ>)
target_link_libraries(${AITT_WEBRTC_UT} ${UT_NEEDS_LIBRARIES} ${AITT_WEBRTC_NEEDS_LIBRARIES} ${AITT_COMMON})
install(TARGETS ${AITT_WEBRTC_UT} DESTINATION ${AITT_TEST_BINDIR})

add_test(
    NAME
        ${AITT_WEBRTC_UT}
    COMMAND
        ${CMAKE_COMMAND} -E env
        LD_LIBRARY_PATH=${CMAKE_BINARY_DIR}/common/:$ENV{LD_LIBRARY_PATH}
        ${CMAKE_CURRENT_BINARY_DIR}/${AITT_WEBRTC_UT} --gtest_filter=*_Anytime
)
