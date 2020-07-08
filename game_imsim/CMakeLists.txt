set(SRC
	stdafx.h
	common.h
	tools.h
	render_queue.h
	draw_queue.h
	serialization.cpp
	serialization.h
	convar.cpp
	convar.h

	entities.def
	an.h

	entry.cpp
	# game_core.cpp
	game.cpp
	game.h
	editor.cpp
	draw_queue.cpp
	collision_handling.cpp
	path_finding.cpp
	path_finding.h
	texture_picker.cpp
	texture_picker.h
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

add_executable(game_an ${SRC})
target_include_directories(game_an PRIVATE ${SDL2_INCLUDE_DIRS} ${SDL2TTF_INCLUDE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
target_link_directories(game_an PRIVATE ${SDL2TTF_LIBRARY} )
target_link_libraries(game_an PRIVATE ${SDL2_LIBRARIES} SDL2_ttf glad imgui libgame ${LINUXLIBS} box2d)
target_compile_options(game_an PRIVATE "-mfma")
target_precompile_headers(game_an PRIVATE "stdafx.h")
ld_builddir(game_an)
