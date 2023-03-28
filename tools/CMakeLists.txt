set(AITT_DISCOVERY_TOOL ${PROJECT_NAME}_discovery_viewer)

###########################################################################
add_executable(${AITT_DISCOVERY_TOOL} discovery_viewer.cc FlexbufPrinter.cc)
target_link_libraries(${AITT_DISCOVERY_TOOL} ${AITT_NEEDS_LIBRARIES})

install(TARGETS ${AITT_DISCOVERY_TOOL} DESTINATION ${AITT_TEST_BINDIR})
