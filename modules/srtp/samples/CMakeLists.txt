ADD_EXECUTABLE("aitt_udp_test" udp_test.cc)
TARGET_LINK_LIBRARIES("aitt_udp_test" ${PROJECT_NAME} Threads::Threads ${APP_LIBRARIES})
INSTALL(TARGETS "aitt_udp_test" DESTINATION ${AITT_TEST_BINDIR})

ADD_EXECUTABLE("aitt_udp_simple_test" udp_simple_test.cc)
TARGET_LINK_LIBRARIES("aitt_udp_simple_test" ${PROJECT_NAME} Threads::Threads ${APP_LIBRARIES})
INSTALL(TARGETS "aitt_udp_simple_test" DESTINATION ${AITT_TEST_BINDIR})

ADD_EXECUTABLE("aitt_srtp_test" srtp_test.cc)
TARGET_LINK_LIBRARIES("aitt_srtp_test" ${PROJECT_NAME} Threads::Threads ${APP_LIBRARIES})
INSTALL(TARGETS "aitt_srtp_test" DESTINATION ${AITT_TEST_BINDIR})
