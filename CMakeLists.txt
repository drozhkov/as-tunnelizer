#
cmake_minimum_required (VERSION 3.16)


#
project ("astun")


# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()


#
if (MSVC)
	add_compile_options(-D_WIN32_WINNT=0x0601)
endif()


#
##
##
include_directories("src/astun-core/src")

##
find_package(Boost REQUIRED COMPONENTS asio json)


# 
##
add_subdirectory ("src/astun-core")

##
add_subdirectory ("src/astun")


#
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
