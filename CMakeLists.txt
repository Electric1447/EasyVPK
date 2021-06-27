cmake_minimum_required(VERSION 2.8)

## This includes the Vita toolchain, must go before project definition
# It is a convenience so you do not have to type
# -DCMAKE_TOOLCHAIN_FILE=$VITASDK/share/vita.toolchain.cmake for cmake. It is
# highly recommended that you include this block for all projects.
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
	if(DEFINED ENV{VITASDK})
		set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
	else()
		message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
	endif()
endif()

project(easyvpk)

include("${VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "EasyVPK")
set(VITA_TITLEID  "ESVPK0009")
set(VITA_VERSION  "01.31")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -Wall -O2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-sign-compare -Wno-unused-parameter -Wno-psabi -Wunused-variable -Wwrite-strings -fpermissive -std=c++17 -Wimplicit-fallthrough")

set(VITA_MKSFOEX_FLAGS "${VITA_MKSFOEX_FLAGS} -d PARENTAL_LEVEL=7")


find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)

link_directories(
	${CMAKE_CURRENT_BINARY_DIR}
)

FUNCTION(ADD_RESOURCES out_var)
	SET(result)
	FOREACH (in_f ${ARGN})
		SET(out_f "${CMAKE_CURRENT_BINARY_DIR}/${in_f}.o")
		GET_FILENAME_COMPONENT(out_dir ${out_f} DIRECTORY)
		ADD_CUSTOM_COMMAND(OUTPUT ${out_f}
				COMMAND ${CMAKE_COMMAND} -E make_directory ${out_dir}
				COMMAND ${CMAKE_LINKER} -r -b binary -o ${out_f} ${in_f}
				DEPENDS ${in_f}
				WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
				COMMENT "Building resource ${out_f}"
				VERBATIM
				)
		LIST(APPEND result ${out_f})
	ENDFOREACH ()
	SET(${out_var} "${result}" PARENT_SCOPE)
ENDFUNCTION()

set(RELEASE_DIR ${CMAKE_SOURCE_DIR}/release)
file(GLOB_RECURSE res_files RELATIVE ${CMAKE_SOURCE_DIR}
	assets/head.bin
)

add_resources(PROJECT_RESOURCES ${res_files})

file(GLOB minizip
	"src/utils/minizip/*.c"
	"src/utils/minizip/*.h"
)

file(GLOB screens
	"src/screens/*.cpp"
	"src/screens/*.hpp"
)

add_executable(easyvpk
	${minizip}
	${screens}
	${PROJECT_RESOURCES}
	src/main.cpp
	src/net/download.cpp
	src/utils/filesystem.cpp
	src/utils/search.cpp
	src/utils/vitaPackage.cpp
	src/utils/zip.cpp
	src/utils/sha1.c
	src/graphics/2dfont.cpp
)

target_link_libraries(easyvpk
	vita2d
	${CURL_LIBRARIES}
	${OPENSSL_LIBRARIES}
	freetype
	png
	jpeg
	z
	m
	c
	ScePgf_stub
	SceGxm_stub
	SceSqlite_stub
	SceCtrl_stub
	SceNet_stub
	SceNetCtl_stub
	SceAppUtil_stub
	SceAppMgr_stub
	ScePromoterUtil_stub
	SceSsl_stub
	SceIofilemgr_stub
	SceHttp_stub
	SceIme_stub
	ScePower_stub
	SceSysmodule_stub
	SceShellSvc_stub
	SceDisplay_stub
	SceCommonDialog_stub
	SceLibKernel_stub
)

vita_create_self(easyvpk.self easyvpk UNSAFE)

vita_create_vpk(easyvpk.vpk ${VITA_TITLEID} easyvpk.self
	VERSION ${VITA_VERSION}
	NAME ${VITA_APP_NAME}
	FILE sce_sys/icon0.png sce_sys/icon0.png
	FILE sce_sys/pic0.png sce_sys/pic0.png
	FILE sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
	FILE sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
	FILE sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
	FILE src/graphics/bg.png resources/bg.png
	FILE src/graphics/navbar0.png resources/navbar0.png
	FILE src/graphics/navbar1.png resources/navbar1.png
	FILE src/graphics/navbar2.png resources/navbar2.png
	FILE src/graphics/navbar3.png resources/navbar3.png
	FILE src/graphics/navbar4.png resources/navbar4.png
)
