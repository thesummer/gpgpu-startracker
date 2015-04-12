set(labelPhase_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
                              ${CMAKE_SOURCE_DIR}/src/getTime.cpp 
                              ${CMAKE_SOURCE_DIR}/src/phase.cpp
                              ${CMAKE_SOURCE_DIR}/src/labelPhase.cpp)
# Build labelPhase
add_executable(example_labelPhase ${labelPhase_SRCS} ${gpulabeling_HEADER} ${RES_FILES})

if (TARGET_PI)
    target_link_libraries(example_labelPhase /opt/vc/lib/libGLESv2.so /opt/vc/lib/libEGL.so /opt/vc/lib/libbcm_host.so)
else (TARGET_PI)
    set(CMAKE_CXX_FLAGS "-Wall -std=gnu++11 -Dcimg_use_png -Dcimg_display=0")
    target_link_libraries(example_labelPhase GLESv2 EGL png)
endif (TARGET_PI)

add_custom_command(TARGET example_labelPhase POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/test1.png .
                   COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/test2.png .
                   COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_SOURCE_DIR}/glsl/quad.vert ./quad.vert
                   COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_SOURCE_DIR}/glsl/labelPhase.frag ./labelPhase.frag
                   COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_SOURCE_DIR}/glsl/common.glsl ./common.glsl
                   COMMAND ${CMAKE_COMMAND} -E create_symlink test1.png test.png
                   WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/examples/labelPhase
)

set_target_properties(example_labelPhase PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/examples/labelPhase)
set_target_properties(example_labelPhase PROPERTIES OUTPUT_NAME example_labelPhase${BUILD_POSTFIX})
