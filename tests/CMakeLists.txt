ADD_DEFINITIONS(-DLOG_STDOUT)

PKG_CHECK_MODULES(UT_NEEDS REQUIRED gmock_main)
INCLUDE_DIRECTORIES(${UT_NEEDS_INCLUDE_DIRS})
LINK_DIRECTORIES(${UT_NEEDS_LIBRARY_DIRS})

###########################################################################
SET(AITT_UT ${PROJECT_NAME}_ut)

INCLUDE_DIRECTORIES(../mock)
AUX_SOURCE_DIRECTORY(../mock MOCK_SRC)
SET(AITT_SRC ../src/MQ.cc ../src/MSG.cc MQ_test.cc)
ADD_EXECUTABLE(${AITT_UT} ${AITT_SRC} ${MOCK_SRC})
TARGET_LINK_LIBRARIES(${AITT_UT} ${UT_NEEDS_LIBRARIES} Threads::Threads ${AITT_NEEDS_LIBRARIES})
INSTALL(TARGETS ${AITT_UT} DESTINATION ${AITT_TEST_BINDIR})

ADD_TEST(
    NAME
        ${AITT_UT}
    COMMAND
        ${CMAKE_COMMAND} -E env
        ${CMAKE_CURRENT_BINARY_DIR}/${AITT_UT} --gtest_filter=*_Anytime
)

###########################################################################
SET(AITT_UT_SRC AITT_test.cc RequestResponse_test.cc MainLoopHandler_test.cc aitt_c_test.cc)
ADD_EXECUTABLE(${AITT_UT}_aitt ${AITT_UT_SRC})
TARGET_LINK_LIBRARIES(${AITT_UT}_aitt Threads::Threads ${UT_NEEDS_LIBRARIES} ${PROJECT_NAME})

INSTALL(TARGETS ${AITT_UT}_aitt DESTINATION ${AITT_TEST_BINDIR})

ADD_TEST(
    NAME
        ${AITT_UT}_aitt
    COMMAND
        ${CMAKE_COMMAND} -E env
        LD_LIBRARY_PATH=../modules/tcp/:../:$ENV{LD_LIBRARY_PATH}
        ${CMAKE_CURRENT_BINARY_DIR}/${AITT_UT}_aitt --gtest_filter=*_Anytime
)

###########################################################################
SET(TransportModuleLoader_UT_SRC TransportModuleLoader_test.cc ../src/TransportModuleLoader.cc)
ADD_EXECUTABLE(${AITT_UT}_transportModule ${TransportModuleLoader_UT_SRC})
TARGET_LINK_LIBRARIES(${AITT_UT}_transportModule ${UT_NEEDS_LIBRARIES} ${CMAKE_DL_LIBS})
TARGET_INCLUDE_DIRECTORIES(${AITT_UT}_transportModule PRIVATE ../src)

INSTALL(TARGETS ${AITT_UT}_transportModule DESTINATION bin)

ADD_TEST(
    NAME
        ${AITT_UT}_transportModule
    COMMAND
        ${CMAKE_COMMAND} -E env
        LD_LIBRARY_PATH=../modules/tcp/:../:$ENV{LD_LIBRARY_PATH}
        ${CMAKE_CURRENT_BINARY_DIR}/${AITT_UT}_transportModule --gtest_filter=*_Anytime
)
