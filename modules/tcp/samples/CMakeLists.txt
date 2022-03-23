ADD_EXECUTABLE("aitt_tcp_test" tcp_test.cc)
TARGET_LINK_LIBRARIES("aitt_tcp_test" ${AITT_TCP} Threads::Threads ${APP_LIBRARIES})
INSTALL(TARGETS "aitt_tcp_test" DESTINATION ${AITT_TEST_BINDIR})
