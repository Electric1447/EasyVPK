cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
	if(DEFINED ENV{VITASDK})
		set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
	else()
		message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
	endif()
endif()

set(SHORT_NAME easyvpk)
project(${SHORT_NAME})
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "EasyVPK")
set(VITA_TITLEID  "ESVPK0009")
set(UPDATER_TITLEID "ESUPD0009")
set(VITA_VERSION  "02.00")


set(PACKAGE_TEMP_FOLDER "ux0:/temp/pkg/")

add_subdirectory(updater)
create_updater("${SHORT_NAME}" "${VITA_TITLEID}" "${UPDATER_TITLEID}" "${PACKAGE_TEMP_FOLDER}")


set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -Wall -O2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-sign-compare -Wno-unused-parameter -Wno-psabi -Wunused-variable -Wwrite-strings -fpermissive -std=c++17 -Wimplicit-fallthrough")

set(VITA_MKSFOEX_FLAGS "${VITA_MKSFOEX_FLAGS} -d PARENTAL_LEVEL=7")


find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)


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


file(GLOB_RECURSE res_files RELATIVE ${CMAKE_SOURCE_DIR}
	assets/head.bin
)

add_resources(PROJECT_RESOURCES ${res_files})


file(GLOB_RECURSE PROJECT_SOURCE_FILES "src/*.h" "src/*.hpp" "src/*.cpp" "src/*.c")

add_executable(${SHORT_NAME}
	${PROJECT_SOURCE_FILES}
	${PROJECT_RESOURCES}
)

target_compile_definitions(${SHORT_NAME}
	PRIVATE
	VITA_VERSION="${VITA_VERSION}"
	VITA_TITLEID="${VITA_TITLEID}"
	PACKAGE_TEMP_FOLDER="${PACKAGE_TEMP_FOLDER}"
	UPDATER_TITLEID="${UPDATER_TITLEID}"
)

target_link_libraries(${SHORT_NAME}
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

vita_create_self(${SHORT_NAME}.self ${SHORT_NAME} UNSAFE)
vita_create_vpk(${SHORT_NAME}.vpk ${VITA_TITLEID} ${SHORT_NAME}.self
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
	
	FILE build/easyvpk_updater.self resources/updater/eboot.bin
	FILE build/easyvpk_updater_param.sfo resources/updater/param.sfo
)

add_dependencies(${SHORT_NAME}.vpk
	easyvpk_updater.self
	easyvpk_updater_param.sfo
)
