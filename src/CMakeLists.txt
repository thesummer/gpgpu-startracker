# Use all .cpp-files
FILE(GLOB gpulabeling_SRC *.cpp)

#set(gpulabeling_SRC "${gpulabeling_SRC} ${CMAKE_CURRENT_SOURCE_DIR}/getTime.c")

# Build ususat
if (BUILD_AS_LIBRARY STREQUAL ON)
    list(REMOVE_ITEM gpulabeling_SRC ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
    add_library(gpulabeling ${gpulabeling_SRC} ${CMAKE_CURRENT_SOURCE_DIR}/getTime.cpp ${gpulabeling_HEADER} ${RES_FILES})
else (BUILD_AS_LIBRARY STREQUAL ON)
    add_executable(gpulabeling ${gpulabeling_SRC} ${CMAKE_CURRENT_SOURCE_DIR}/getTime.cpp ${gpulabeling_HEADER} ${RES_FILES})
endif (BUILD_AS_LIBRARY STREQUAL ON)

if (TARGET_PI)
    target_link_libraries(gpulabeling png /opt/vc/lib/libGLESv2.so /opt/vc/lib/libEGL.so /opt/vc/lib/libbcm_host.so /usr/lib/arm-linux-gnueabihf/librt.so pthread)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Dcimg_use_png -Dcimg_display=0")
else (TARGET_PI)
    set(CMAKE_CXX_FLAGS "-Wall -std=gnu++11 -Dcimg_use_png -Dcimg_display=0")
    target_link_libraries(gpulabeling png GLESv2 EGL pthread)
endif (TARGET_PI)

set_target_properties(gpulabeling PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set_target_properties(gpulabeling PROPERTIES OUTPUT_NAME gpulabeling${BUILD_POSTFIX})

if (NOT BUILD_AS_LIBRARY STREQUAL ON)
    add_subdirectory(examples)
endif (NOT BUILD_AS_LIBRARY STREQUAL ON)

