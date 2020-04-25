set(SRC
	stdafx.h
	common.h
	render_queue.h
	draw_queue.h
	shaders.h
	mechaspirit.h
	collision.h
	animator.h
	projectiles.h

	entry.cpp
	game_core.cpp
	shaders.cpp
	collision.cpp
	animator.cpp
	projectiles.cpp
)

add_executable(game ${SRC})
target_include_directories(game PRIVATE ${SDL2_INCLUDE_DIRS} ${SDL2TTF_INCLUDE_DIR})
target_link_directories(game PRIVATE ${SDL2TTF_LIBRARY} )
target_link_libraries(game PRIVATE ${SDL2_LIBRARIES} SDL2_ttf glad imgui libgame)
target_compile_options(game PRIVATE "-mfma")
target_precompile_headers(game PRIVATE "stdafx.h")
ld_builddir(game)