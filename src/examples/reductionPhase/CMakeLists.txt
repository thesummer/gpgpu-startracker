set(reductionPhase_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
                              ${CMAKE_SOURCE_DIR}/src/getTime.cpp 
                              ${CMAKE_SOURCE_DIR}/src/phase.cpp
                              ${CMAKE_SOURCE_DIR}/src/reductionPhase.cpp)
# Build reductionPhase
add_executable(example_reductionPhase ${reductionPhase_SRCS} ${gpulabeling_HEADER} ${RES_FILES})

if (TARGET_PI)
    target_link_libraries(example_reductionPhase ${CMAKE_SOURCE_DIR}/lib-pi/libtga.so /opt/vc/lib/libGLESv2.so /opt/vc/lib/libEGL.so /opt/vc/lib/libbcm_host.so)
else (TARGET_PI)
    set(CMAKE_CXX_FLAGS "-Wall -std=gnu++11")    
    target_link_libraries(example_reductionPhase ${CMAKE_SOURCE_DIR}/lib/libtga.so GLESv2 EGL)
endif (TARGET_PI)

add_custom_command(TARGET example_reductionPhase POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/test1.tga .
                   COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/test2.tga .
                   COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/glsl/quad.vert .
                   COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/glsl/reductionPhase.frag .
                   COMMAND ${CMAKE_COMMAND} -E create_symlink test1.tga test.tga
                   WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/examples/reductionPhase
)

set_target_properties(example_reductionPhase PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/examples/reductionPhase)
set_target_properties(example_reductionPhase PROPERTIES OUTPUT_NAME example_reductionPhase${BUILD_POSTFIX})
