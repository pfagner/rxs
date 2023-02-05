cmake_minimum_required(VERSION 2.8)

set(platform "x86-64" CACHE STRING "name of platform for config[x86 | x86-64 | arm | qnx32]")

#required for arm
set(arm_compiler_path "usr/bin" CACHE PATH "path to arm compiler")
set(arm_gcc_name "arm-linux-gnueabi-gcc")
set(arm_g++_name "arm-linux-gnueabi-g++")

project(rxs)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wunused-function -std=gnu99 -D_XOPEN_SOURCE=700 -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64")
#-std=c11
###################################################################
#  X/Open and POSIX standards
###################################################################
#-D_POSIX_C_SOURCE=200809L
#-D_XOPEN_SOURCE=700 (700 - X/Open 7, incorporating POSIX 2008)
#-D_GNU_SOURCE

#make env var from arm_compiler_path string
set(ENV{CARMPATH} ${arm_compiler_path})

if(platform MATCHES "x86")
	message( STATUS "configure for x86")
	set(CMAKE_C_COMPILER "gcc")
	set(CMAKE_CXX_COMPILER "g++")
	#set(CMAKE_C_COMPILER "clang")
	#set(CMAKE_CXX_COMPILER "clang++")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
	link_libraries("-L/usr/lib32")
    # Address Sanitizer. Compile options
    #set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -fsanitize=address -fsanitize=leak")
    #set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -g -fsanitize=address -fsanitize=leak")
elseif(platform MATCHES "x86-64")
	message( STATUS "configure for x86-64")
	set(CMAKE_C_COMPILER "gcc")
	set(CMAKE_CXX_COMPILER "g++")
	#set(CMAKE_C_COMPILER "clang")
	#set(CMAKE_CXX_COMPILER "clang++")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
	link_libraries("-L/usr/lib/x86_64-linux-gnu")
    # Address Sanitizer. Compile options
    #set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -fsanitize=address -fsanitize=leak")
    #set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -g -fsanitize=address -fsanitize=leak")
        
elseif(platform MATCHES "arm")
	message( STATUS "configure for arm")
	
	#check compiler existance
	if(NOT EXISTS $ENV{CARMPATH}/${arm_gcc_name})
		message( FATAL_ERROR $ENV{CARMPATH}/${arm_gcc_name} " not found" )
	endif()
	if(NOT EXISTS $ENV{CARMPATH}/${arm_g++_name})
		message( FATAL_ERROR $ENV{CARMPATH}/${arm_g++_name} " not found" )
	endif()
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
	#set
	set(CMAKE_C_COMPILER $ENV{CARMPATH}/${arm_gcc_name})
	set(CMAKE_CXX_COMPILER $ENV{CARMPATH}/${arm_g++_name})
	link_libraries("-L$ENV{CARMPATH}/../sysroot/usr/lib")
    # Address Sanitizer. Compile options
    #set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -lgcc_eh -ldl -lm -g -fsanitize=address -fsanitize=leak")
    #set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -lgcc_eh -ldl -lm -g -fsanitize=address -fsanitize=leak")

elseif(platform MATCHES "qnx32")
	message( STATUS "configure for qnx32")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -l socket")
	link_libraries("-L/usr/lib32")

else()
#default - x86
	message( STATUS "platform not recognized. Configure for x86")
	set(CMAKE_C_COMPILER "/usr/bin/gcc")
	set(CMAKE_CXX_COMPILER "/usr/bin/g++")
	link_libraries("-L/usr/lib32")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
endif()

set(RXS_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

include_directories(
  ${RXS_INCLUDE_DIR}
  )

add_subdirectory(lib)
add_subdirectory(tools)
