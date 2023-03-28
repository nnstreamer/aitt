set(AITT_UT ${PROJECT_NAME}_ut)

add_definitions(-DLOG_STDOUT)

pkg_check_modules(UT_NEEDS REQUIRED gmock_main)
include_directories(${UT_NEEDS_INCLUDE_DIRS} ../src)
link_directories(${UT_NEEDS_LIBRARY_DIRS})

###########################################################################
set(AITT_UT_SRC AITT_test.cc RequestResponse_test.cc MainLoopHandler_test.cc aitt_c_test.cc
    AITT_TCP_test.cc)
add_executable(${AITT_UT} ${AITT_UT_SRC})
target_link_libraries(${AITT_UT} Threads::Threads ${UT_NEEDS_LIBRARIES} ${PROJECT_NAME})

install(TARGETS ${AITT_UT} DESTINATION ${AITT_TEST_BINDIR})

add_test(
    NAME
        ${AITT_UT}
    COMMAND
        ${CMAKE_COMMAND} -E env
        LD_LIBRARY_PATH=../modules/tcp/:../modules/webrtc/:../:../common/:$ENV{LD_LIBRARY_PATH}
        ${CMAKE_CURRENT_BINARY_DIR}/${AITT_UT} --gtest_filter=*_Anytime
)

###########################################################################

if(WITH_WEBRTC)
add_definitions(-DWITH_WEBRTC)
endif()
set(AITT_STREAM_UT ${PROJECT_NAME}_stream_ut)
set(AITT_STREAM_UT_SRC AittStream_test.cc)
add_executable(${AITT_STREAM_UT} ${AITT_STREAM_UT_SRC})
target_link_libraries(${AITT_STREAM_UT} Threads::Threads ${UT_NEEDS_LIBRARIES} ${PROJECT_NAME})

install(TARGETS ${AITT_STREAM_UT} DESTINATION ${AITT_TEST_BINDIR})

add_test(
    NAME
        ${AITT_STREAM_UT}
    COMMAND
        ${CMAKE_COMMAND} -E env
        LD_LIBRARY_PATH=../modules/tcp/:../modules/webrtc/:../modules/rtsp/:../:../common/:$ENV{LD_LIBRARY_PATH}
        ${CMAKE_CURRENT_BINARY_DIR}/${AITT_STREAM_UT} --gtest_filter=*_Anytime
)
###########################################################################
file(GLOB AITT_MANUAL_SRC *_manualtest.cc)
add_executable(${AITT_UT}_manual ${AITT_MANUAL_SRC})
target_link_libraries(${AITT_UT}_manual Threads::Threads ${UT_NEEDS_LIBRARIES} ${PROJECT_NAME})

install(TARGETS ${AITT_UT}_manual DESTINATION ${AITT_TEST_BINDIR})

###########################################################################
if(${UT_NEEDS_VERSION} GREATER 1.7)
    set(AITT_UT_MQ ${PROJECT_NAME}_ut_mq)
    add_executable(${AITT_UT_MQ} MosquittoMQ_mocktest.cc MosquittoMock.cc $<TARGET_OBJECTS:INTERNAL_OBJ>)
    target_link_libraries(${AITT_UT_MQ} ${UT_NEEDS_LIBRARIES} Threads::Threads ${CMAKE_DL_LIBS} ${AITT_COMMON})
    if(${UT_NEEDS_VERSION} GREATER 1.10)
        target_include_directories(${AITT_UT_MQ} PRIVATE ../external/C-Mock/v0.4.0)
    else()
        target_include_directories(${AITT_UT_MQ} PRIVATE ../external/C-Mock/v0.3.1)
    endif()
    target_link_libraries(${AITT_UT_MQ} ${UT_NEEDS_LIBRARIES})
    install(TARGETS ${AITT_UT_MQ} DESTINATION ${AITT_TEST_BINDIR})

    add_test(
        NAME
            ${AITT_UT_MQ}
        COMMAND
            ${CMAKE_COMMAND} -E env
            LD_LIBRARY_PATH=../common/:$ENV{LD_LIBRARY_PATH}
            ${CMAKE_CURRENT_BINARY_DIR}/${AITT_UT_MQ} --gtest_filter=*_Anytime
    )
endif(${UT_NEEDS_VERSION} GREATER 1.7)
###########################################################################
add_executable(${AITT_UT}_local ModuleLoader_test.cc MosquittoMQ_test.cc $<TARGET_OBJECTS:INTERNAL_OBJ>)
target_link_libraries(${AITT_UT}_local ${UT_NEEDS_LIBRARIES} ${AITT_NEEDS_LIBRARIES} ${CMAKE_DL_LIBS} ${AITT_COMMON})

install(TARGETS ${AITT_UT}_local DESTINATION ${AITT_TEST_BINDIR})

add_test(
    NAME
        ${AITT_UT}_local
    COMMAND
        ${CMAKE_COMMAND} -E env
        LD_LIBRARY_PATH=../modules/tcp/:../modules/webrtc/:../modules/rtsp/:../:../common/:$ENV{LD_LIBRARY_PATH}
        ${CMAKE_CURRENT_BINARY_DIR}/${AITT_UT}_local --gtest_filter=*_Anytime
)
