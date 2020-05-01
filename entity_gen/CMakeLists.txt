set(SRC
	stdafx.h
	common.h
	entry.cpp
	output_stdio.cpp

	mmap.h
	output.h
)

if(WIN32)
set(SRC_PLAT mmap_win32.cpp)
else()
set(SRC_PLAT mmap_posix.cpp)
endif()

add_executable(entity_gen ${SRC} ${SRC_PLAT})
target_precompile_headers(entity_gen PRIVATE "stdafx.h")
ld_builddir(entity_gen)
