pkg_check_modules(SAMPLE_NEEDS REQUIRED ${ADDITION_PKG})
include_directories(${SAMPLE_NEEDS_INCLUDE_DIRS})
link_directories(${SAMPLE_NEEDS_LIBRARY_DIRS})

add_executable("aitt_tcp_test" tcp_test.cc)
target_link_libraries("aitt_tcp_test" TCP_OBJ ${SAMPLE_NEEDS_LIBRARIES} ${AITT_TCP_NEEDS_LIBRARIES})
install(TARGETS "aitt_tcp_test" DESTINATION ${AITT_TEST_BINDIR})
