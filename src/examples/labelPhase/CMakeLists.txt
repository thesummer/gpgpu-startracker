set(labelPhase_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
                              ${CMAKE_SOURCE_DIR}/src/getTime.cpp 
                              ${CMAKE_SOURCE_DIR}/src/phase.cpp
                              ${CMAKE_SOURCE_DIR}/src/labelPhase.cpp)
# Build labelPhase
add_executable(example_labelPhase ${labelPhase_SRCS} ${gpulabeling_HEADER} ${RES_FILES})

if (TARGET_PI)
    target_link_libraries(example_labelPhase ${CMAKE_SOURCE_DIR}/lib-pi/libtga.so /opt/vc/lib/libGLESv2.so /opt/vc/lib/libEGL.so /opt/vc/lib/libbcm_host.so)
else (TARGET_PI)
    set(CMAKE_CXX_FLAGS "-Wall -std=gnu++11")    
    target_link_libraries(example_labelPhase ${CMAKE_SOURCE_DIR}/lib/libtga.so GLESv2 EGL)
endif (TARGET_PI)

set_target_properties(example_labelPhase PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set_target_properties(example_labelPhase PROPERTIES OUTPUT_NAME gpulabeling${BUILD_POSTFIX})
