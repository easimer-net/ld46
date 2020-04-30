set(SRC
	stdafx.h
	common.h
	tools.h
	render_queue.h
	draw_queue.h
	imsim.h
	serialization.cpp
	serialization.h
	convar.cpp
	convar.h

	entities.def
	imsim_temp.h

	entry.cpp
	# game_core.cpp
	game.cpp
	editor.cpp
	draw_queue.cpp
)

if(MSVC)
    set(LINUXLIBS)
else()
    set(LINUXLIBS dl)
endif()

add_executable(game_imsim ${SRC})
target_include_directories(game_imsim PRIVATE ${SDL2_INCLUDE_DIRS} ${SDL2TTF_INCLUDE_DIR})
target_link_directories(game_imsim PRIVATE ${SDL2TTF_LIBRARY} )
target_link_libraries(game_imsim PRIVATE ${SDL2_LIBRARIES} SDL2_ttf glad imgui libgame ${LINUXLIBS})
target_compile_options(game_imsim PRIVATE "-mfma")
target_precompile_headers(game_imsim PRIVATE "stdafx.h")
ld_builddir(game_imsim)
