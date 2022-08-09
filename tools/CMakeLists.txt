SET(AITT_DISCOVERY_TOOL ${PROJECT_NAME}_discovery_viewer)

###########################################################################
ADD_EXECUTABLE(${AITT_DISCOVERY_TOOL} discovery_viewer.cc FlexbufPrinter.cc)
TARGET_LINK_LIBRARIES(${AITT_DISCOVERY_TOOL} ${AITT_NEEDS_LIBRARIES})

INSTALL(TARGETS ${AITT_DISCOVERY_TOOL} DESTINATION ${AITT_TEST_BINDIR})
