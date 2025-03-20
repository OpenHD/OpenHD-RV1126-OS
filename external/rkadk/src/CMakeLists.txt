file(GLOB_RECURSE COMMON_SRC "${CMAKE_SOURCE_DIR}/src/common/*.cpp")
file(GLOB_RECURSE RECORD_SRC "${CMAKE_SOURCE_DIR}/src/record/*.cpp")
file(GLOB_RECURSE PHOTO_SRC "${CMAKE_SOURCE_DIR}/src/photo/*.cpp")
file(GLOB_RECURSE STREAM_SRC "${CMAKE_SOURCE_DIR}/src/stream/*.cpp")
file(GLOB_RECURSE PLAYER_SRC "${CMAKE_SOURCE_DIR}/src/player/*.cpp")
file(GLOB_RECURSE PARAM_SRC "${CMAKE_SOURCE_DIR}/src/param/*.cpp")
file(GLOB_RECURSE RTSP_SRC "${CMAKE_SOURCE_DIR}/src/live/rtsp/*.cpp")
file(GLOB_RECURSE RTMP_SRC "${CMAKE_SOURCE_DIR}/src/live/rtmp/*.cpp")
file(GLOB_RECURSE DISP_SRC "${CMAKE_SOURCE_DIR}/src/display/*.cpp")
file(GLOB_RECURSE VI_SRC "${CMAKE_SOURCE_DIR}/src/vi/*.cpp")
file(GLOB_RECURSE STORAGE_SRC "${CMAKE_SOURCE_DIR}/src/storage/*.cpp")
add_definitions(-g -O0 -ggdb -gdwarf -funwind-tables -rdynamic -D_GNU_SOURCE)

add_library(rkadk SHARED
		${COMMON_SRC}
		${RECORD_SRC}
		${PHOTO_SRC}
		${STREAM_SRC}
		${PLAYER_SRC}
		${PARAM_SRC}
                ${RTMP_SRC}
		${RTSP_SRC}
                ${DISP_SRC}
                ${VI_SRC}
		${STORAGE_SRC})

set(LIB_RTSP ${CMAKE_SOURCE_DIR}/src/live/rtsp/librtsp.a)
message("LIB_RTSP = ${LIB_RTSP}")
target_include_directories(rkadk PUBLIC "${CMAKE_SOURCE_DIR}/src/record")
target_link_libraries(rkadk easymedia third_media rga iniparser cjson ${LIB_RTSP} rkfsmk)

if(USE_ROCKIT)
    target_link_libraries(rkadk rockit)
endif()

if(USE_RKAIQ)
    target_link_libraries(rkadk rkaiq)
endif()

# install target
install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
