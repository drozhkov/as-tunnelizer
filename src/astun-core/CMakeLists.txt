#
cmake_minimum_required (VERSION 3.16)


#
project ("astun-core")


#
##
#include_directories("src")

##
set(LIBS
)


#
add_library(${PROJECT_NAME} 
	src/tunnelizer-server.cpp
	src/tunnelizer-session.cpp
	src/tunnelizer-client.cpp
	src/tunnelizer-service.cpp
	src/tunnelizer-config.cpp
	src/tunnelizer-core.cpp
	src/logger.cpp
)


#
set_target_properties(${PROJECT_NAME} PROPERTIES 
	CXX_STANDARD 20
)


#
target_link_libraries(${PROJECT_NAME} 
	PRIVATE Boost::asio
	PRIVATE Boost::json
)
