SET(AITT_UT ${PROJECT_NAME}_ut)

ADD_DEFINITIONS(-DLOG_STDOUT)

PKG_CHECK_MODULES(UT_NEEDS REQUIRED gmock_main)
INCLUDE_DIRECTORIES(${UT_NEEDS_INCLUDE_DIRS} ../src)
LINK_DIRECTORIES(${UT_NEEDS_LIBRARY_DIRS})

###########################################################################
SET(AITT_UT_SRC AITT_test.cc RequestResponse_test.cc MainLoopHandler_test.cc aitt_c_test.cc
    AITT_TCP_test.cc)
ADD_EXECUTABLE(${AITT_UT} ${AITT_UT_SRC})
TARGET_LINK_LIBRARIES(${AITT_UT} Threads::Threads ${UT_NEEDS_LIBRARIES} ${PROJECT_NAME})

INSTALL(TARGETS ${AITT_UT} DESTINATION ${AITT_TEST_BINDIR})

ADD_TEST(
    NAME
        ${AITT_UT}
    COMMAND
        ${CMAKE_COMMAND} -E env
        LD_LIBRARY_PATH=../modules/tcp/:../modules/webrtc/:../:../common/:$ENV{LD_LIBRARY_PATH}
        ${CMAKE_CURRENT_BINARY_DIR}/${AITT_UT} --gtest_filter=*_Anytime
)

###########################################################################

IF(WITH_WEBRTC)
ADD_DEFINITIONS(-DWITH_WEBRTC)
ENDIF()
SET(AITT_STREAM_UT ${PROJECT_NAME}_stream_ut)
SET(AITT_STREAM_UT_SRC AittStream_test.cc)
ADD_EXECUTABLE(${AITT_STREAM_UT} ${AITT_STREAM_UT_SRC})
TARGET_LINK_LIBRARIES(${AITT_STREAM_UT} Threads::Threads ${UT_NEEDS_LIBRARIES} ${PROJECT_NAME})

INSTALL(TARGETS ${AITT_STREAM_UT} DESTINATION ${AITT_TEST_BINDIR})

ADD_TEST(
    NAME
        ${AITT_STREAM_UT}
    COMMAND
        ${CMAKE_COMMAND} -E env
        LD_LIBRARY_PATH=../modules/tcp/:../modules/webrtc/:../modules/rtsp/:../:../common/:$ENV{LD_LIBRARY_PATH}
        ${CMAKE_CURRENT_BINARY_DIR}/${AITT_STREAM_UT} --gtest_filter=*_Anytime
)
###########################################################################
FILE(GLOB AITT_MANUAL_SRC *_manualtest.cc)
ADD_EXECUTABLE(${AITT_UT}_manual ${AITT_MANUAL_SRC})
TARGET_LINK_LIBRARIES(${AITT_UT}_manual Threads::Threads ${UT_NEEDS_LIBRARIES} ${PROJECT_NAME})

INSTALL(TARGETS ${AITT_UT}_manual DESTINATION ${AITT_TEST_BINDIR})

###########################################################################
IF(${UT_NEEDS_VERSION} GREATER 1.7)
    SET(AITT_UT_MQ ${PROJECT_NAME}_ut_mq)
    ADD_EXECUTABLE(${AITT_UT_MQ} MosquittoMQ_mocktest.cc MosquittoMock.cc $<TARGET_OBJECTS:INTERNAL_OBJ>)
    TARGET_LINK_LIBRARIES(${AITT_UT_MQ} ${UT_NEEDS_LIBRARIES} Threads::Threads ${CMAKE_DL_LIBS} ${AITT_COMMON})
    IF(${UT_NEEDS_VERSION} GREATER 1.10)
        TARGET_INCLUDE_DIRECTORIES(${AITT_UT_MQ} PRIVATE ../external/C-Mock/v0.4.0)
    ELSE()
        TARGET_INCLUDE_DIRECTORIES(${AITT_UT_MQ} PRIVATE ../external/C-Mock/v0.3.1)
    ENDIF()
    TARGET_LINK_LIBRARIES(${AITT_UT_MQ} ${UT_NEEDS_LIBRARIES})
    INSTALL(TARGETS ${AITT_UT_MQ} DESTINATION ${AITT_TEST_BINDIR})

    ADD_TEST(
        NAME
            ${AITT_UT_MQ}
        COMMAND
            ${CMAKE_COMMAND} -E env
            LD_LIBRARY_PATH=../common/:$ENV{LD_LIBRARY_PATH}
            ${CMAKE_CURRENT_BINARY_DIR}/${AITT_UT_MQ} --gtest_filter=*_Anytime
    )
ENDIF(${UT_NEEDS_VERSION} GREATER 1.7)
###########################################################################
ADD_EXECUTABLE(${AITT_UT}_local ModuleLoader_test.cc MosquittoMQ_test.cc $<TARGET_OBJECTS:INTERNAL_OBJ>)
TARGET_LINK_LIBRARIES(${AITT_UT}_local ${UT_NEEDS_LIBRARIES} ${AITT_NEEDS_LIBRARIES} ${CMAKE_DL_LIBS} ${AITT_COMMON})

INSTALL(TARGETS ${AITT_UT}_local DESTINATION ${AITT_TEST_BINDIR})

ADD_TEST(
    NAME
        ${AITT_UT}_local
    COMMAND
        ${CMAKE_COMMAND} -E env
        LD_LIBRARY_PATH=../modules/tcp/:../modules/webrtc/:../modules/rtsp/:../:../common/:$ENV{LD_LIBRARY_PATH}
        ${CMAKE_CURRENT_BINARY_DIR}/${AITT_UT}_local --gtest_filter=*_Anytime
)
