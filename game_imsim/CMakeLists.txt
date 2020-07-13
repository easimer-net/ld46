set(SRC
	stdafx.h
	common.h
	tools.h
	draw_queue.h
	serialization.cpp
	serialization.h
	convar.cpp
	convar.h

	entities.def
	an.h

	entry.cpp
	game.cpp
	game.h
	editor.cpp
	main_menu.cpp
	collision_handling.cpp
	path_finding.cpp
	path_finding.h
	texture_picker.cpp
	texture_picker.h
	input_manager.cpp
	input_manager.h

	irenderer.h
	r_gl.cpp
)

if(MSVC)
    set(LINUXLIBS)
else()
    set(LINUXLIBS dl)
endif()

add_custom_command(
	OUTPUT an.h serialization.cpp
	COMMAND entity_gen ARGS ${CMAKE_CURRENT_BINARY_DIR} "${CMAKE_CURRENT_SOURCE_DIR}/entities.def" an
	DEPENDS entity_gen entities.def
)

macro(build_variant TARGET_SUFFIX HAS_STEAM)
	set(TARGET_NAME game_an-${TARGET_SUFFIX})
	add_executable(${TARGET_NAME} ${SRC})
	target_include_directories(${TARGET_NAME} PRIVATE ${SDL2_INCLUDE_DIRS} ${SDL2TTF_INCLUDE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
	target_link_directories(${TARGET_NAME} PRIVATE ${SDL2TTF_LIBRARY} )

	if(${HAS_STEAM})
		set(STEAMWORKS_LIB_PRIV ${STEAMWORKS_LIB})
	else()
		target_compile_definitions(${TARGET_NAME} PRIVATE "BUILD_NO_STEAM")
	endif()

	target_link_libraries(${TARGET_NAME} PRIVATE SDL2-static SDL2main SDL2_ttf glad imgui libgame ${LINUXLIBS} box2d ${STEAMWORKS_LIB_PRIV})
	target_compile_options(${TARGET_NAME} PRIVATE "-mfma")
	target_precompile_headers(${TARGET_NAME} PRIVATE "stdafx.h")
	ld_builddir(${TARGET_NAME})
endmacro()

build_variant(steam TRUE)
build_variant(nosteam FALSE)
