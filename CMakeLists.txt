cmake_minimum_required(VERSION 3.16)
project(ld46)

# =======================================================
# Purpose: Set the target's build directory
# =======================================================
macro(ld_builddir target)
	# For executables
	set_target_properties(${target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/Debug")
	set_target_properties(${target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release")
	set_target_properties(${target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/RelWithDebInfo")
	# For libraries
	set_target_properties(${target} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/Debug/bin")
	set_target_properties(${target} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release/bin")
	set_target_properties(${target} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/RelWithDebInfo/bin")
endmacro()

# =======================================================
# Purpose: Enable precompiled headers for target
# TODO(danielm): cmake has builtin support for this now!
# =======================================================
macro(ld_enable_pch target pchbase)
	if(MSVC)
		set_target_properties(${target} PROPERTIES COMPILE_FLAGS "/Yu\"${pchbase}.h\"")
		set_source_files_properties(${pchbase}.cpp
			PROPERTIES COMPILE_FLAGS "/Yc\"${pchbase}.h\""
			OBJECT_CREATES "${pchbase}.pch")
	endif()
endmacro()

# =======================================================
# Purpose: Makes a file not use the precompiled header
# =======================================================
macro(ld_ignore_pch file)
	if(MSVC)
		set_source_files_properties(${file}
			PROPERTIES COMPILE_FLAGS "/Y-")
	endif()
endmacro()

# =======================================================
# Purpose: Don't link the CRT
# =======================================================
macro(ld_no_crt target)
	if(MSVC)
		set_property(TARGET ${target} APPEND_STRING PROPERTY LINK_FLAGS_DEBUG "/NODEFAULTLIB:msvcrt ")
		set_property(TARGET ${target} APPEND_STRING PROPERTY LINK_FLAGS_RELEASE "/NODEFAULTLIB:msvcrt ")
		set_property(TARGET ${target} APPEND_STRING PROPERTY LINK_FLAGS_RELWITHDEBINFO "/NODEFAULTLIB:msvcrt ")
	endif()
endmacro()

# =======================================================
# Purpose: Defines a macro
# =======================================================
macro(ld_is_library target)
	target_compile_definitions(${target} PRIVATE ZAIDAN_LIBRARY=1)
endmacro()

# =======================================================
# Purpose: Sets the subsystem of the target
# =======================================================
macro(ld_subsystem target subsys)
	if(MSVC)
		set_property(TARGET ${target} APPEND_STRING PROPERTY LINK_FLAGS_DEBUG "/SUBSYSTEM:${subsys} ")
		set_property(TARGET ${target} APPEND_STRING PROPERTY LINK_FLAGS_RELEASE "/SUBSYSTEM:${subsys} ")
		set_property(TARGET ${target} APPEND_STRING PROPERTY LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:${subsys} ")
	endif(MSVC)
endmacro()

# =======================================================
# Purpose: Disables Runtime Type Information, exceptions
# and anything else that we don't need
# =======================================================
macro(ld_disable_bloat target)
	if(MSVC)
		set_property(TARGET ${target} APPEND_STRING PROPERTY COMPILE_OPTIONS "-GS-;-Gs9999999;-GR-;-Gm-;/EHs-c-;-fp:fast;-fp:except-")
		set_property(TARGET ${target} APPEND_STRING PROPERTY LINK_FLAGS_DEBUG "-opt:ref -STACK:0x400000,0x400000 ")
		set_property(TARGET ${target} APPEND_STRING PROPERTY LINK_FLAGS_RELEASE "-opt:ref -STACK:0x400000,0x400000 ")
		set_property(TARGET ${target} APPEND_STRING PROPERTY LINK_FLAGS_RELWITHDEBINFO "-opt:ref -STACK:0x400000,0x400000 ")
	endif(MSVC)
endmacro()

# =======================================================
# Purpose: Use this on every source file that uses
# SSE4.1 intrinsics
# =======================================================
macro(ld_sse41 file)
	if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86")
		if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
			set_source_files_properties(${file} PROPERTIES COMPILE_FLAGS -msse4.1)
		endif()
	endif()
endmacro()

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
enable_testing()

# Include public headers for all targets
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/public")

if(WIN32)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
endif()

# Statically link the CRT
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(MSVC)
	# Disable permissive mode in VS 2017 or later
	if (NOT (MSVC_VERSION LESS 1910))
		add_compile_options (/permissive-)
	endif()
endif()

# find_package(SDL2 REQUIRED)
# find_package(SDL2_ttf REQUIRED)

set(BOX2D_BUILD_TESTBED false)
set(BOX2D_BUILD_UNIT_TESTS false)
set(BOX2D_BUILD_DOCS false)

add_subdirectory(steamworks)
add_subdirectory(sdl2)
add_subdirectory(glad)
add_subdirectory(imgui)
add_subdirectory(box2d)
add_subdirectory(libgame)
add_subdirectory(entity_gen)
# add_subdirectory(game_mechaspirit)
add_subdirectory(game_imsim)
