set(SRC_LIBGAME
	stdafx.h
	stb_image.h
	textures.cpp

	../public/textures.h
)

add_library(libgame STATIC ${SRC_LIBGAME})