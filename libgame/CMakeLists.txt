set(SRC_LIBGAME
	stdafx.h
	stb_image.h
	animator.cpp
	collision.cpp
	projectiles.cpp
	shaders.cpp
	textures.cpp

	../public/animator.h
	../public/collision.h
	../public/projectiles.h
	../public/shaders.h
	../public/textures.h
)

add_library(libgame STATIC ${SRC_LIBGAME})
target_precompile_headers(libgame PRIVATE "stdafx.h")