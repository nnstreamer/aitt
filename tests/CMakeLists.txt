SET(AITT_UT ${PROJECT_NAME}_ut)

ADD_DEFINITIONS(-DLOG_STDOUT)

PKG_CHECK_MODULES(UT_NEEDS REQUIRED gmock_main)
INCLUDE_DIRECTORIES(${UT_NEEDS_INCLUDE_DIRS})
LINK_DIRECTORIES(${UT_NEEDS_LIBRARY_DIRS})

###########################################################################
SET(AITT_UT_SRC AITT_test.cc RequestResponse_test.cc MainLoopHandler_test.cc aitt_c_test.cc AITT_TCP_test.cc MQ_test.cc)
ADD_EXECUTABLE(${AITT_UT} ${AITT_UT_SRC})
TARGET_LINK_LIBRARIES(${AITT_UT} Threads::Threads ${UT_NEEDS_LIBRARIES} ${PROJECT_NAME})

INSTALL(TARGETS ${AITT_UT} DESTINATION ${AITT_TEST_BINDIR})

ADD_TEST(
    NAME
        ${AITT_UT}
    COMMAND
        ${CMAKE_COMMAND} -E env
        LD_LIBRARY_PATH=../modules/tcp/:../:../common/:$ENV{LD_LIBRARY_PATH}
        ${CMAKE_CURRENT_BINARY_DIR}/${AITT_UT} --gtest_filter=*_Anytime
)

###########################################################################
FILE(GLOB AITT_MANUAL_SRC *_manualtest.cc)
ADD_EXECUTABLE(${AITT_UT}_manual ${AITT_MANUAL_SRC})
TARGET_LINK_LIBRARIES(${AITT_UT}_manual Threads::Threads ${UT_NEEDS_LIBRARIES} ${PROJECT_NAME})

INSTALL(TARGETS ${AITT_UT}_manual DESTINATION ${AITT_TEST_BINDIR})

###########################################################################
AUX_SOURCE_DIRECTORY(../mock MOCK_SRC)
ADD_EXECUTABLE(${AITT_UT}_mq MQ_mocktest.cc ${MOCK_SRC})
TARGET_LINK_LIBRARIES(${AITT_UT}_mq ${UT_NEEDS_LIBRARIES} Threads::Threads ${AITT_NEEDS_LIBRARIES} ${AITT_COMMON})
TARGET_INCLUDE_DIRECTORIES(${AITT_UT}_mq PRIVATE ../mock)
INSTALL(TARGETS ${AITT_UT}_mq DESTINATION ${AITT_TEST_BINDIR})

ADD_TEST(
    NAME
        ${AITT_UT}_mq
    COMMAND
        ${CMAKE_COMMAND} -E env
        LD_LIBRARY_PATH=../common/:$ENV{LD_LIBRARY_PATH}
        ${CMAKE_CURRENT_BINARY_DIR}/${AITT_UT}_mq --gtest_filter=*_Anytime
)

###########################################################################
ADD_EXECUTABLE(${AITT_UT}_module ModuleLoader_test.cc)
TARGET_LINK_LIBRARIES(${AITT_UT}_module ${UT_NEEDS_LIBRARIES} ${AITT_NEEDS_LIBRARIES} ${CMAKE_DL_LIBS} ${AITT_COMMON})
TARGET_INCLUDE_DIRECTORIES(${AITT_UT}_module PRIVATE ../src)

INSTALL(TARGETS ${AITT_UT}_module DESTINATION ${AITT_TEST_BINDIR})

ADD_TEST(
    NAME
        ${AITT_UT}_module
    COMMAND
        ${CMAKE_COMMAND} -E env
        LD_LIBRARY_PATH=../modules/tcp/:../:../common/:$ENV{LD_LIBRARY_PATH}
        ${CMAKE_CURRENT_BINARY_DIR}/${AITT_UT}_module --gtest_filter=*_Anytime
)
