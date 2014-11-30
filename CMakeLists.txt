cmake_minimum_required(VERSION 2.8)
project(gpulabeling)

option (TARGET_PI 
        "Use build options for the raspberry" OFF) 

if (TARGET_PI)
    set(CMAKE_CXX_FLAGS "-Wall -std=gnu++0x -D_RPI")    
    include_directories(/opt/vc/include /opt/vc/include/interface/vcos/pthreads /opt/vc/include/interface/vmcs_host/linux    )
    set(BUILD_POSTFIX "_pi")
else (TARGET_PI)
    set(CMAKE_CXX_FLAGS "-Wall -std=gnu++11")  
    set(BUILD_POSTFIX "")
endif (TARGET_PI)


set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lrt")

message("CMAKE_CXX_FLAGS_DEBUG is ${CMAKE_CXX_FLAGS_DEBUG}")
message("CMAKE_CXX_FLAGS_RELEASE is ${CMAKE_CXX_FLAGS_RELEASE}")
message("CMAKE_BUILD_TYPE is ${CMAKE_BUILD_TYPE}")


if (CMAKE_BUILD_TYPE STREQUAL Debug)
    set(BUILD_POSTFIX "${BUILD_POSTFIX}_d")
endif (CMAKE_BUILD_TYPE STREQUAL Debug)

message("BUILD_POSTFIX is ${BUILD_POSTFIX}")

include_directories(include)
file(GLOB RES_FILES glsl/*.frag glsl/*.vert glsl/*.glsl)

# collect header files
FILE(GLOB gpulabeling_HEADER include/*.h)

add_subdirectory(src)


