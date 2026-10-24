cmake_minimum_required(VERSION 3.19 FATAL_ERROR)

project(
	flare
	VERSION 0.1
	DESCRIPTION "A command-line utility to crop white gutters off images"
	HOMEPAGE_URL "https://github.com/AmanoTeam/flare"
	LANGUAGES C
)

option(FLARE_LTO "Turn on compiler link-time optimizations" OFF)

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
	--enable-decoder=aac,h264,mp3,png,webp
	--enable-encoder=png
	--enable-demuxer=aac,h264,hls,webvtt,mp3,image2,webp
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
	--extra-cflags=-I${ZLIB_SOURCE_DIRECTORY}
	--extra-ldflags=-fPIC
	--prefix="${FFMPEG_INSTALL_PREFIX}"
)

execute_process(
	COMMAND git submodule update --init --depth=1
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
	COMMAND_ERROR_IS_FATAL ANY
)

if (SERENITYOS)
	file(READ "${CMAKE_CURRENT_SOURCE_DIR}/submodules/ffmpeg/libavutil/thread.h" FILE_CONTENTS)
	string(REPLACE "#if HAVE_PRCTL" "#if !defined(__serenity__) && HAVE_PRCTL" FILE_CONTENTS "${FILE_CONTENTS}")
	file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/submodules/ffmpeg/libavutil/thread.h" "${FILE_CONTENTS}")
endif()

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/submodules/ffmpeg/configure" FILE_CONTENTS)
string(REPLACE "die \"Host compiler lacks C11 support\"" "true" FILE_CONTENTS "${FILE_CONTENTS}")
string(REPLACE "check_lib zlib   zlib.h      zlibVersion    -lz" "true" FILE_CONTENTS "${FILE_CONTENTS}")
file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/submodules/ffmpeg/configure" "${FILE_CONTENTS}")

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

	list(APPEND FFMPEG_CONFIGURE_FLAGS "--target-os=${system_name}")
	list(APPEND FFMPEG_CONFIGURE_FLAGS "--arch=${CMAKE_SYSTEM_PROCESSOR}")
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

if (NOT EXISTS "${FFMPEG_BUILD_DIRECTORY}/Makefile")
	message("-- Configuring FFmpeg")

	execute_process(
		COMMAND bash
		"${FFMPEG_SOURCE_DIRECTORY}/configure"
		${FFMPEG_CONFIGURE_FLAGS}
		WORKING_DIRECTORY "${FFMPEG_BUILD_DIRECTORY}"
		COMMAND_ERROR_IS_FATAL ANY
	)
endif()

if (NOT EXISTS "${FFMPEG_INSTALL_PREFIX}")
	add_custom_command(
		OUTPUT "${FFMPEG_INSTALL_PREFIX}"
		COMMAND make
		COMMAND make install
		WORKING_DIRECTORY "${FFMPEG_BUILD_DIRECTORY}"
		COMMENT "-- Building FFmpeg"
	)
endif()

add_custom_target(
	ffmpeg
	ALL
	DEPENDS "${FFMPEG_INSTALL_PREFIX}"
)

# zlib
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

set(
	FFMPEG_LIBRARIES
	"${FFMPEG_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}avformat${CMAKE_STATIC_LIBRARY_SUFFIX}"
	"${FFMPEG_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}avcodec${CMAKE_STATIC_LIBRARY_SUFFIX}"
	"${FFMPEG_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}swscale${CMAKE_STATIC_LIBRARY_SUFFIX}"
	"${FFMPEG_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}swresample${CMAKE_STATIC_LIBRARY_SUFFIX}"
	"${FFMPEG_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}avutil${CMAKE_STATIC_LIBRARY_SUFFIX}"
)

add_executable(
	flare
	"${CMAKE_CURRENT_SOURCE_DIR}/src/argparse.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/errors.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/gutter.c"
)

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
			zlibstatic
			PROPERTIES
			INTERPROCEDURAL_OPTIMIZATION TRUE
		)
	endif()
endif()

target_link_libraries(
	flare
	PRIVATE
	${FFMPEG_LIBRARIES}
	zlibstatic
)

if (UNIX)
	target_link_libraries(
		flare
		PRIVATE
		m
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
)
