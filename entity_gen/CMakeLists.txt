set(SRC
	stdafx.h
	common.h
	entry.cpp
	output_stdio.cpp
	lexer.cpp
	parser.cpp
    emit_cpp.cpp

	mmap.h
	output.h
	lexer.h
	parser.h
    emit_cpp.h
)

if(WIN32)
set(SRC_PLAT mmap_win32.cpp)
else()
set(SRC_PLAT mmap_posix.cpp)
endif()

add_executable(entity_gen ${SRC} ${SRC_PLAT})
target_precompile_headers(entity_gen PRIVATE "stdafx.h")
ld_builddir(entity_gen)

add_executable(entity_gen_tests ${SRC} ${SRC_PLAT} tests.cpp)
ld_builddir(entity_gen_tests)

add_test(NAME entity_gen_tests COMMAND entity_gen_tests)
