#
cmake_minimum_required (VERSION 3.16)


#
add_executable (${PROJECT_NAME} 
	"src/astun.cpp" 
)


#
##
set(LIBS
	PRIVATE astun-core
)


#
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)


#
target_link_libraries(${PROJECT_NAME} 
	${LIBS} 
	PRIVATE Boost::asio
	PRIVATE Boost::json
)


#
install(TARGETS ${PROJECT_NAME} DESTINATION ./bin)
