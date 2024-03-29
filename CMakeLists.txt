cmake_minimum_required(VERSION 3.7.1)
set(CMAKE_SKIP_BUILD_RPATH true)
project(aitt VERSION 0.0.1 LANGUAGES CXX C)
set_property(GLOBAL PROPERTY GLOBAL_DEPENDS_DEBUG_MODE 0)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(VERSIONING "Specify Library Verion" ON)

include(GNUInstallDirs)
include(FindPkgConfig)
find_package(Threads REQUIRED)

add_definitions(-DLOG_TAG="AITT")
add_definitions(-DPLATFORM=${PLATFORM})

if(PLATFORM STREQUAL "android")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fdiagnostics-color")
	add_definitions(-DANDROID)
	include(${PROJECT_ROOT_DIR}/cmake/aitt_android_flatbuffers.cmake)
	include(${PROJECT_ROOT_DIR}/cmake/aitt_android_mosquitto.cmake)
	set(AITT_NEEDS_LIBRARIES ${MOSQUITTO_LIBRARY} ${FLATBUFFERS_LIBRARY} ${LOG_LIBRARIES})
else(PLATFORM STREQUAL "android")
	if(PLATFORM STREQUAL "tizen")
		if(WITH_WEBRTC)
			add_definitions(-DWITH_WEBRTC)
		endif()
		if(WITH_RTSP)
			add_definitions(-DWITH_RTSP)
		endif()
		add_definitions(-DTIZEN)
		set(ADDITIONAL_OPT "-DTIZEN")
		set(ADDITION_PKG dlog)
	endif(PLATFORM STREQUAL "tizen")
	if(PLATFORM STREQUAL "tizenRT")
		add_definitions(-DTIZEN_RT)
		set(USE_GLIB OFF)
		set(WITH_MBEDTLS ON)
		set(BUILD_TESTING OFF)
	endif(PLATFORM STREQUAL "tizenRT")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -Wno-psabi -fdiagnostics-color -fvisibility=hidden")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pie")
	if(USE_GLIB)
		set(ADDITION_PKG "${ADDITION_PKG} glib-2.0")
	endif(USE_GLIB)
	pkg_check_modules(AITT_NEEDS REQUIRED ${ADDITION_PKG} libmosquitto flatbuffers)
	include_directories(${AITT_NEEDS_INCLUDE_DIRS})
	link_directories(${AITT_NEEDS_LIBRARY_DIRS})
endif(PLATFORM STREQUAL "android")

if(LOG_STDOUT)
	add_definitions(-DLOG_STDOUT)
	#add_definitions(-DLOG_OFF)
endif(LOG_STDOUT)

if(COVERAGE_TEST)
	add_compile_options(--coverage)
	add_link_options(--coverage -Wl,--dynamic-list-data)
endif(COVERAGE_TEST)

set(AITT_COMMON ${PROJECT_NAME}-common)
set(AITT_TCP aitt-transport-tcp)

include_directories(include common)

aux_source_directory(src AITT_SRC)

set(AITT_INTERNAL_SRC src/ModuleManager.cc src/NullTransport.cc src/MosquittoMQ.cc)
add_library(INTERNAL_OBJ OBJECT ${AITT_INTERNAL_SRC})
list(REMOVE_ITEM AITT_SRC ${AITT_INTERNAL_SRC})

add_library(${PROJECT_NAME} SHARED ${AITT_SRC} $<TARGET_OBJECTS:INTERNAL_OBJ>)
target_link_libraries(${PROJECT_NAME} Threads::Threads ${CMAKE_DL_LIBS} ${AITT_COMMON})
target_link_libraries(${PROJECT_NAME} ${AITT_NEEDS_LIBRARIES})
if(PLATFORM STREQUAL "tizenRT")
	target_include_directories(${PROJECT_NAME} PRIVATE modules/custom_mqtt)
	add_library(CUSTOM_MQ STATIC modules/custom_mqtt/CustomMQ.cc)
	target_link_libraries(${PROJECT_NAME} ${AITT_TCP} CUSTOM_MQ)
endif(PLATFORM STREQUAL "tizenRT")

target_compile_options(${PROJECT_NAME} PUBLIC ${AITT_NEEDS_CFLAGS_OTHER})
if(VERSIONING)
	set_target_properties(${PROJECT_NAME} PROPERTIES
		VERSION ${PROJECT_VERSION}
		SOVERSION ${PROJECT_VERSION_MAJOR}
		)
endif(VERSIONING)
install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_LIBDIR})

file(GLOB HEADERS include/*.h)
install(FILES ${HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})

configure_file(${PROJECT_NAME}.pc.in ${PROJECT_NAME}.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
set_directory_properties(PROPERTIES ADDITIONAL_CMAKE_CLEAN_FILES ${PROJECT_NAME}.pc)

if(BUILD_TESTING)
	enable_testing() # NOTE: Must comes first than unittest subdirectories
	set(XML_OUTPUT "--gtest_output=xml:${CMAKE_BINARY_DIR}/test_results/")
	set(AITT_TEST_BINDIR ${CMAKE_INSTALL_BINDIR})
	add_subdirectory(tests)
endif(BUILD_TESTING)

add_subdirectory(tools)

option(WITH_TCP "Build TCP module?" ON)
option(WITH_MBEDTLS "Use Mbed TLS, not OpenSSL" OFF)
option(WITH_WEBRTC "Build WebRtc module?" OFF)
option(WITH_RTSP "Build RTSP module?" OFF)

if(WITH_TCP)
	add_subdirectory(modules/tcp)
endif()

if(PLATFORM STREQUAL "tizen")
	if(WITH_WEBRTC)
		add_subdirectory(modules/webrtc)
	endif()
	if(WITH_RTSP)
		add_subdirectory(modules/rtsp)
	endif()
endif(PLATFORM STREQUAL "tizen")

add_subdirectory(common)
