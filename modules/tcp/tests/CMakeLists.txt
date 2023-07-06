set(AITT_TCP_UT ${PROJECT_NAME}_tcp_ut)

set(AITT_TCP_UT_SRC TCP_test.cc TCPServer_test.cc AESEncryptor_test.cc)
if(WITH_MBEDTLS)
    set(AITT_TCP_UT_SRC ${AITT_TCP_UT_SRC} ../AESEncryptorOpenSSL.cc AES_Compatibility_test.cc)
    set(ADDITION_PKG ${ADDITION_PKG} openssl)
endif(WITH_MBEDTLS)

pkg_check_modules(UT_NEEDS REQUIRED gmock_main ${ADDITION_PKG})
include_directories(${UT_NEEDS_INCLUDE_DIRS})
link_directories(${UT_NEEDS_LIBRARY_DIRS})

add_executable(${AITT_TCP_UT} ${AITT_TCP_UT_SRC})
target_link_libraries(${AITT_TCP_UT} TCP_OBJ Threads::Threads ${UT_NEEDS_LIBRARIES} ${AITT_TCP_NEEDS_LIBRARIES})
install(TARGETS ${AITT_TCP_UT} DESTINATION ${AITT_TEST_BINDIR})

add_test(
    NAME
        ${AITT_TCP_UT}
    COMMAND
        ${CMAKE_COMMAND} -E env
        ${CMAKE_CURRENT_BINARY_DIR}/${AITT_TCP_UT} --gtest_filter=*_Anytime ${XML_OUTPUT}
)
