file(GLOB COMMON_SOURCES "common/*.cpp")
file(GLOB COMMON_HEADERS "common/*.hpp")

function(add_test TEST_DIR TEST_NAME)
	message(STATUS "Configuring example ${TEST_NAME} in ${TEST_DIR}")
 
	# source
	file(GLOB SOURCES "${TEST_DIR}/*.cpp")
	file(GLOB HEADERS "${TEST_DIR}/*.hpp")
 
	add_executable(${TEST_NAME} ${HEADERS} ${SOURCES} ${COMMON_HEADERS} ${COMMON_SOURCES})
	target_include_directories(${TEST_NAME} PUBLIC ../src/)
	target_link_libraries(${TEST_NAME} FengRenderer)
	set_target_properties(${TEST_NAME} PROPERTIES CXX_STANDARD 20)
	set_target_properties(${TEST_NAME} PROPERTIES CXX_EXTENSIONS OFF)
	set_target_properties(${TEST_NAME} PROPERTIES CMAKE_CXX_STANDARD_REQUIRED ON)
	set_target_properties(${TEST_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/../")
endfunction(add_test)

add_test(demo Demo)
add_test(demo2 Demo2)