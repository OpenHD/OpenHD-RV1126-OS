cmake_minimum_required(VERSION 3.8)

project(cvr_app)

include(FindPkgConfig)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(RkAiq REQUIRED)
include_directories(${RKAIQ_INCLUDE_DIRS})
add_definitions(-DRKAIQ)

include_directories(
	${PROJECT_SOURCE_DIR}/./
	${PROJECT_SOURCE_DIR}/src/
	${CMAKE_SYSROOT}/usr/include/lvgl/
	${CMAKE_SYSROOT}/usr/include/lvgl/src/
	${CMAKE_SYSROOT}/usr/include/libdrm/
	${CMAKE_SYSROOT}/usr/include/easymedia/
	${CMAKE_SYSROOT}/usr/include/rkadk/
	${CMAKE_SYSROOT}/usr/include/freetype2/
	)

link_directories(
	${PROJECT_SOURCE_DIR}/./lib)

aux_source_directory(./ SRCS)
aux_source_directory(./src/hal/ SRCS)
aux_source_directory(./src/lvgl SRCS)
aux_source_directory(./src/media SRCS)
aux_source_directory(./src/osd SRCS)
aux_source_directory(./src/ui SRCS)
aux_source_directory(./src/utils/isp SRCS)

add_executable(${PROJECT_NAME} ${SRCS})

target_link_libraries(${PROJECT_NAME}
  drm lvgl rga pthread m rkadk rkaiq png freetype rkfsmk easymedia)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
