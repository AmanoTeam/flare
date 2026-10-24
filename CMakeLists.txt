cmake_minimum_required(VERSION 3.19 FATAL_ERROR)

project(
	flare
	VERSION 0.1
	DESCRIPTION "A command-line utility to crop white and black gutters off images"
	HOMEPAGE_URL "https://github.com/AmanoTeam/flare"
	LANGUAGES C
)

option(FLARE_LTO "Turn on compiler link-time optimizations" OFF)
option(FLARE_LIBRARY "Build the project as a shared library" OFF)

set(CMAKE_POLICY_DEFAULT_CMP0048 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

set(
	WARNING_OPTIONS
	"-Wall"
	"-Wextra"
)

set(FFMPEG_SOURCE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/submodules/ffmpeg")
set(ZLIB_SOURCE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/submodules/zlib")

set(FFMPEG_BUILD_DIRECTORY "${CMAKE_BINARY_DIR}/ffmpeg-build")
set(FFMPEG_INSTALL_PREFIX "${FFMPEG_BUILD_DIRECTORY}/binaries")

set(
	FFMPEG_CONFIGURE_FLAGS
	--disable-all
	--disable-alsa
	--disable-appkit
	--disable-asm
	--disable-audiotoolbox
	--disable-avfoundation
	--disable-bzlib
	--disable-coreimage
	--disable-debug
	--disable-doc
	--disable-iconv
	--disable-libxcb
	--disable-lzma
	--disable-neon
	--disable-network
	--disable-pthreads
	--disable-schannel
	--disable-sdl2
	--disable-securetransport
	--disable-shared
	--disable-videotoolbox
	--disable-vulkan
	--disable-w32threads
	--disable-xlib
	--enable-avcodec
	--enable-avformat
	--enable-avutil
	--enable-swscale
	--enable-swresample
	--enable-decoder=aac,h264,mp3,png,webp,mjpeg
	--enable-encoder=png
	--enable-demuxer=aac,h264,hls,webvtt,mp3,image2,webp,image2pipe,image_png_pipe,image_jpeg_pipe,image_webp_pipe
	--enable-muxer=h264,mpegts,mp4,matroska,mp3,image2
	--enable-parser=h264,aac
	--enable-bsf=extract_extradata
	--enable-pic
	--enable-protocol=file,crypto
	--enable-small
	--enable-static
	--enable-version3
	--extra-cflags=-Dstatic_assert=_Static_assert
	--extra-cflags=-w
	--extra-cflags=${CMAKE_C_FLAGS}
	--extra-ldflags=-fPIC
	--prefix="${FFMPEG_INSTALL_PREFIX}"
)

if (NOT ANDROID)
	list(APPEND FFMPEG_CONFIGURE_FLAGS "--extra-cflags=-I${ZLIB_SOURCE_DIRECTORY}")
endif()

file(LOCK "${CMAKE_CURRENT_SOURCE_DIR}/lock")

execute_process(
	COMMAND git submodule update --init --depth=1
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
	COMMAND_ERROR_IS_FATAL ANY
)

if (SERENITYOS AND NOT EXISTS "${FFMPEG_SOURCE_DIRECTORY}/.thread-patched")
	file(READ "${FFMPEG_SOURCE_DIRECTORY}/libavutil/thread.h" FILE_CONTENTS)
	string(REPLACE "#if HAVE_PRCTL" "#if !defined(__serenity__) && HAVE_PRCTL" FILE_CONTENTS "${FILE_CONTENTS}")
	file(WRITE "${FFMPEG_SOURCE_DIRECTORY}/libavutil/thread.h" "${FILE_CONTENTS}")

	file(WRITE "${FFMPEG_SOURCE_DIRECTORY}/.thread-patched" "")
endif()

set(FFMPEG_CONFIGURE_ORIGINAL "")

file(READ "${FFMPEG_SOURCE_DIRECTORY}/configure" FFMPEG_CONFIGURE_ORIGINAL)

set(FFMPEG_CONFIGURE_PATCHED "${FFMPEG_CONFIGURE_ORIGINAL}")

string(REPLACE "die \"Host compiler lacks C11 support\"" "true" FFMPEG_CONFIGURE_PATCHED "${FFMPEG_CONFIGURE_PATCHED}")

string(REPLACE "check_pkg_config zlib zlib \"zlib.h\" zlibVersion ||" "check_lib zlib zlib.h zlibVersion -lz ||" FFMPEG_CONFIGURE_PATCHED "${FFMPEG_CONFIGURE_PATCHED}")

string(REPLACE "check_lib zlib   zlib.h      zlibVersion    -lz; }" "true; }" FFMPEG_CONFIGURE_PATCHED "${FFMPEG_CONFIGURE_PATCHED}")

string(REPLACE "enabled getenv || echo \"#define getenv(x) NULL\" >> $TMPH" "true" FFMPEG_CONFIGURE_PATCHED "${FFMPEG_CONFIGURE_PATCHED}")

if (NOT "${FFMPEG_CONFIGURE_PATCHED}" STREQUAL "${FFMPEG_CONFIGURE_ORIGINAL}")
	file(WRITE "${FFMPEG_SOURCE_DIRECTORY}/configure" "${FFMPEG_CONFIGURE_PATCHED}")
endif()

file(LOCK "${CMAKE_CURRENT_SOURCE_DIR}/lock" RELEASE)

if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
	add_compile_options($<$<CONFIG:Release>:-fipa-pta>)
endif()

if (FLARE_LTO)
	list(APPEND FFMPEG_CONFIGURE_FLAGS "--enable-lto")
endif()

if (CMAKE_CROSSCOMPILING)
	string(TOLOWER "${CMAKE_SYSTEM_NAME}" system_name)

	if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
		set(system_name "mingw32")
	elseif ("${CMAKE_SYSTEM_NAME}" STREQUAL "SerenityOS")
		set(system_name "none")
	elseif ("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS")
		set(system_name "darwin")
	endif()

	if (NOT "${TOOLCHAIN_PREFIX}" STREQUAL "")
		list(APPEND FFMPEG_CONFIGURE_FLAGS "--cross-prefix=${TOOLCHAIN_PREFIX}-")
	elseif (DEFINED ENV{CROSS_COMPILE_TRIPLET})
		list(APPEND FFMPEG_CONFIGURE_FLAGS "--cross-prefix=$ENV{CROSS_COMPILE_TRIPLET}-")
	endif()

	if (ANDROID)
		set(FLARE_API_LEVEL "${ANDROID_PLATFORM_LEVEL}")

		if (FLARE_API_LEVEL STREQUAL "")
			string(REGEX MATCH "[0-9]+$" FLARE_API_LEVEL "${ANDROID_PLATFORM}")
		endif()

		if ("${ANDROID_ABI}" STREQUAL "arm64-v8a")
			set(FLARE_ARCHITECTURE "aarch64")
			set(FLARE_TRIPLE "aarch64-unknown-linux-android")
		elseif ("${ANDROID_ABI}" STREQUAL "armeabi-v7a")
			set(FLARE_ARCHITECTURE "arm")
			set(FLARE_TRIPLE "armv7a-unknown-linux-androideabi")
		elseif ("${ANDROID_ABI}" STREQUAL "x86_64")
			set(FLARE_ARCHITECTURE "x86_64")
			set(FLARE_TRIPLE "x86_64-unknown-linux-android")
		else()
			set(FLARE_ARCHITECTURE "i686")
			set(FLARE_TRIPLE "i686-unknown-linux-android")
		endif()

		list(APPEND FFMPEG_CONFIGURE_FLAGS "--enable-cross-compile")

		if (CMAKE_C_COMPILER_ID MATCHES "Clang")
			list(APPEND FFMPEG_CONFIGURE_FLAGS "--extra-cflags=--target=${FLARE_TRIPLE}${FLARE_API_LEVEL}")
			list(APPEND FFMPEG_CONFIGURE_FLAGS "--extra-ldflags=--target=${FLARE_TRIPLE}${FLARE_API_LEVEL}")
		endif()
	else()
		set(FLARE_ARCHITECTURE "${CMAKE_SYSTEM_PROCESSOR}")
	endif()

	list(APPEND FFMPEG_CONFIGURE_FLAGS "--target-os=${system_name}")
	list(APPEND FFMPEG_CONFIGURE_FLAGS "--arch=${FLARE_ARCHITECTURE}")
	list(APPEND FFMPEG_CONFIGURE_FLAGS "--as=${CMAKE_C_COMPILER}")
	list(APPEND FFMPEG_CONFIGURE_FLAGS "--ld=${CMAKE_C_COMPILER}")
	list(APPEND FFMPEG_CONFIGURE_FLAGS "--cc=${CMAKE_C_COMPILER}")

	if (CMAKE_AR)
		list(APPEND FFMPEG_CONFIGURE_FLAGS "--ar=${CMAKE_AR}")
	endif()

	if (CMAKE_RANLIB)
		list(APPEND FFMPEG_CONFIGURE_FLAGS "--ranlib=${CMAKE_RANLIB}")
	endif()

	if (CMAKE_NM)
		list(APPEND FFMPEG_CONFIGURE_FLAGS "--nm=${CMAKE_NM}")
	endif()

	if (CMAKE_STRIP)
		list(APPEND FFMPEG_CONFIGURE_FLAGS "--strip=${CMAKE_STRIP}")
	endif()
else()
	list(APPEND FFMPEG_CONFIGURE_FLAGS "--cc=${CMAKE_C_COMPILER}")
	list(APPEND FFMPEG_CONFIGURE_FLAGS "--ld=${CMAKE_C_COMPILER}")
endif()

set(FFMPEG_INCLUDE_DIRS "${FFMPEG_INSTALL_PREFIX}/include")

if (NOT EXISTS "${FFMPEG_BUILD_DIRECTORY}")
	file(MAKE_DIRECTORY "${FFMPEG_BUILD_DIRECTORY}")
endif()

set(FFMPEG_PREVIOUS_FLAGS "")

if (EXISTS "${FFMPEG_BUILD_DIRECTORY}/.configure-flags")
	file(READ "${FFMPEG_BUILD_DIRECTORY}/.configure-flags" FFMPEG_PREVIOUS_FLAGS)
endif()

string(JOIN " " FFMPEG_CONFIGURE_FLAGS_TEXT ${FFMPEG_CONFIGURE_FLAGS})

if (NOT "${FFMPEG_PREVIOUS_FLAGS}" STREQUAL "${FFMPEG_CONFIGURE_FLAGS_TEXT}")
	message("-- Configuring FFmpeg")

	file(WRITE "${FFMPEG_BUILD_DIRECTORY}/.configure-flags" "${FFMPEG_CONFIGURE_FLAGS_TEXT}")

	execute_process(
		COMMAND bash
		"${FFMPEG_SOURCE_DIRECTORY}/configure"
		${FFMPEG_CONFIGURE_FLAGS}
		WORKING_DIRECTORY "${FFMPEG_BUILD_DIRECTORY}"
		COMMAND_ERROR_IS_FATAL ANY
	)
endif()

set(
	FFMPEG_LIBRARIES
	"${FFMPEG_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}avformat${CMAKE_STATIC_LIBRARY_SUFFIX}"
	"${FFMPEG_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}avcodec${CMAKE_STATIC_LIBRARY_SUFFIX}"
	"${FFMPEG_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}swscale${CMAKE_STATIC_LIBRARY_SUFFIX}"
	"${FFMPEG_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}swresample${CMAKE_STATIC_LIBRARY_SUFFIX}"
	"${FFMPEG_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}avutil${CMAKE_STATIC_LIBRARY_SUFFIX}"
)

add_custom_command(
	OUTPUT ${FFMPEG_LIBRARIES}
	COMMAND make
	COMMAND make install
	DEPENDS "${FFMPEG_BUILD_DIRECTORY}/.configure-flags"
	WORKING_DIRECTORY "${FFMPEG_BUILD_DIRECTORY}"
	COMMENT "-- Building FFmpeg"
)

add_custom_target(
	ffmpeg
	ALL
	DEPENDS ${FFMPEG_LIBRARIES}
)

# zlib
if (ANDROID)
	set(FLARE_ZLIB_LIBRARY "z")
else()
	set(FLARE_ZLIB_LIBRARY zlibstatic)

	set(ZLIB_BUILD_SHARED OFF)
	set(ZLIB_BUILD_STATIC ON)
	set(ZLIB_BUILD_TESTING OFF)
	set(BUILD_TESTING OFF)

	add_subdirectory(
		submodules/zlib
		EXCLUDE_FROM_ALL
	)

	set_target_properties(
		zlibstatic
		PROPERTIES
		POSITION_INDEPENDENT_CODE ON
	)
endif()

set(
	FLARE_SOURCE_FILES
	"${CMAKE_CURRENT_SOURCE_DIR}/src/argparse.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/errors.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/gutter.c"
)

set(
	FLARE_JNI_SOURCE_FILES
	"${CMAKE_CURRENT_SOURCE_DIR}/src/cropborders.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/flare-jni.c"
)

if (FLARE_LIBRARY)
	add_library(
		flare
		SHARED
		${FLARE_SOURCE_FILES}
		${FLARE_JNI_SOURCE_FILES}
	)

	target_compile_definitions(
		flare
		PRIVATE
		FLARE_LIBRARY
	)

	if (NOT WIN32 AND NOT APPLE)
		target_link_options(
			flare
			PRIVATE
			"-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/src/version_script.map"
		)
	endif()

	if (NOT ANDROID AND DEFINED ENV{JAVA_HOME})
		set(FLARE_JNI_PLATFORM "linux")

		if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
			set(FLARE_JNI_PLATFORM "darwin")
		elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
			set(FLARE_JNI_PLATFORM "win32")
		endif()

		target_include_directories(
			flare
			PRIVATE
			"$ENV{JAVA_HOME}/include"
			"$ENV{JAVA_HOME}/include/${FLARE_JNI_PLATFORM}"
		)
	endif()
else()
	add_executable(
		flare
		${FLARE_SOURCE_FILES}
	)
endif()

target_compile_options(
	flare
	PRIVATE
	${WARNING_OPTIONS}
)

add_dependencies(
	flare
	ffmpeg
)

target_include_directories(
	flare
	PRIVATE
	"${FFMPEG_INCLUDE_DIRS}"
)

if (FLARE_LTO)
	set(FLARE_HAS_LTO OFF)

	include(CheckIPOSupported)
	check_ipo_supported(RESULT FLARE_HAS_LTO LANGUAGES C)

	if (FLARE_HAS_LTO)
		set_target_properties(
			flare
			PROPERTIES
			INTERPROCEDURAL_OPTIMIZATION TRUE
		)

		if (NOT ANDROID)
			set_target_properties(
				zlibstatic
				PROPERTIES
				INTERPROCEDURAL_OPTIMIZATION TRUE
			)
		endif()
	endif()
endif()

target_link_libraries(
	flare
	PRIVATE
	${FFMPEG_LIBRARIES}
	${FLARE_ZLIB_LIBRARY}
)

if (UNIX)
	target_link_libraries(
		flare
		PRIVATE
		m
	)
endif()

if (ANDROID)
	target_link_libraries(
		flare
		PRIVATE
		log
	)
endif()

if (WIN32)
	target_link_libraries(
		flare
		PRIVATE
		bcrypt
	)
endif()

include(CheckCSourceCompiles)

check_c_source_compiles(
	"#include <features.h>
	#if !defined(__linux__) || defined(__GLIBC__) || defined(__UCLIBC__) || defined(__BIONIC__)
		#error \"not musl\"
	#endif
	int main(void) {
		return 0;
	}"
	FLARE_MUSL
)

if (WIN32 OR FLARE_MUSL)
	target_link_options(
		flare
		PRIVATE
		-static
	)
endif()

install(
	TARGETS flare
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
)
