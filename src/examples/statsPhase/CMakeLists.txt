set(statsPhase_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
                              ${CMAKE_SOURCE_DIR}/src/getTime.cpp 
                              ${CMAKE_SOURCE_DIR}/src/phase.cpp
                              ${CMAKE_SOURCE_DIR}/src/statsPhase.cpp)
# Build statsPhase
add_executable(example_statsPhase ${statsPhase_SRCS} ${gpulabeling_HEADER} ${RES_FILES})

if (TARGET_PI)
    target_link_libraries(example_statsPhase png /opt/vc/lib/libGLESv2.so /opt/vc/lib/libEGL.so /opt/vc/lib/libbcm_host.so)
else (TARGET_PI)
    set(CMAKE_CXX_FLAGS "-Wall -std=gnu++11 -Dcimg_use_png -Dcimg_display=0")
    target_link_libraries(example_statsPhase png GLESv2 EGL)
endif (TARGET_PI)

add_custom_command(TARGET example_statsPhase POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/testLabel1.png .
                   COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/testLabel2.png .
                   COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/testReduced1.png .
                   COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/testReduced2.png .
                   COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/testOrig1.png .
                   COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/testOrig2.png .
                   COMMAND ${CMAKE_COMMAND} -E remove ./common.glsl quad.vert statsPhase.frag fillStage.frag countStage.frag centroidStage.frag
                   COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_SOURCE_DIR}/glsl/common.glsl ./common.glsl
                   COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_SOURCE_DIR}/glsl/quad.vert ./quad.vert
                   COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_SOURCE_DIR}/glsl/statsPhase.frag ./statsPhase.frag
                   COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_SOURCE_DIR}/glsl/fillStage.frag ./fillStage.frag
                   COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_SOURCE_DIR}/glsl/countStage.frag ./countStage.frag
                   COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_SOURCE_DIR}/glsl/centroidStage.frag ./centroidStage.frag
                   COMMAND ${CMAKE_COMMAND} -E create_symlink testLabel1.png testLabel.png
                   COMMAND ${CMAKE_COMMAND} -E create_symlink testReduced1.png testReduced.png
                   COMMAND ${CMAKE_COMMAND} -E create_symlink testOrig1.png testOrig.png
                   WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/examples/statsPhase
)

set_target_properties(example_statsPhase PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/examples/statsPhase)
set_target_properties(example_statsPhase PROPERTIES OUTPUT_NAME example_statsPhase${BUILD_POSTFIX})
