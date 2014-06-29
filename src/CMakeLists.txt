# Use all .cpp-files
FILE(GLOB gpulabeling_SRC *.cpp)

#set(gpulabeling_SRC "${gpulabeling_SRC} ${CMAKE_CURRENT_SOURCE_DIR}/getTime.c")

# Build ususat
add_executable(gpulabeling ${gpulabeling_SRC} ${CMAKE_CURRENT_SOURCE_DIR}/getTime.cpp ${gpulabeling_HEADER})

if (TARGET_PI)
    target_link_libraries(gpulabeling ${CMAKE_SOURCE_DIR}/lib-pi/libtga.so /opt/vc/lib/libGLESv2.so /opt/vc/lib/libEGL.so /opt/vc/lib/libbcm_host.so)
else (TARGET_PI)
    set(CMAKE_CXX_FLAGS "-Wall -std=gnu++11")    
    target_link_libraries(gpulabeling ${CMAKE_SOURCE_DIR}/lib/libtga.so GLESv2 EGL)
endif (TARGET_PI)

set_target_properties(gpulabeling PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set_target_properties(gpulabeling PROPERTIES OUTPUT_NAME gpulabeling${BUILD_POSTFIX})
