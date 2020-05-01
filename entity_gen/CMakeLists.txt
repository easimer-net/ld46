set(SRC_CORE
	stdafx.h
	common.h
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

set(SRC_TESTS
    ${SRC_CORE}
    tests_lexer.cpp
    tests_parser.cpp
)

set(SRC
	entry.cpp
    ${SRC_CORE}
    ${SRC_PLAT}
)


add_executable(entity_gen ${SRC})
target_precompile_headers(entity_gen PRIVATE "stdafx.h")
ld_builddir(entity_gen)

add_executable(entity_gen_tests ${SRC_TESTS})
target_precompile_headers(entity_gen_tests PRIVATE "stdafx.h")
ld_builddir(entity_gen_tests)

add_test(NAME entity_gen_tests COMMAND entity_gen_tests)
