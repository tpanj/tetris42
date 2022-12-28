cmake_minimum_required(VERSION 3.22)
project(tetris42 VERSION 1.0.0)

LIST(APPEND SRC tetris42.c)
IF(WIN32)
  LIST(APPEND SRC tetris42.rc)
ENDIF()

# Player count names are define via agruments
add_executable(tetris42  ${SRC})
# Players use generic names PLAYERn and below are seperate
# clickable executables for different number of players
add_executable(tetris4-1 ${SRC})
add_executable(tetris4-2 ${SRC})
add_executable(tetris4-3 ${SRC})
add_executable(tetris4-4 ${SRC})

target_compile_definitions(tetris4-1 PUBLIC PLAYERS=1)
target_compile_definitions(tetris4-2 PUBLIC PLAYERS=2)
target_compile_definitions(tetris4-3 PUBLIC PLAYERS=3)
target_compile_definitions(tetris4-4 PUBLIC PLAYERS=4)

set (CMAKE_BUILD_TYPE "Release")
add_subdirectory(raylib)

find_path(RAYLIB_DIR "raylib.h" HINTS raylib/src)
include_directories(${RAYLIB_DIR})
LIST(APPEND LIBS raylib)
target_link_libraries(tetris42  ${LIBS})
target_link_libraries(tetris4-1 ${LIBS})
target_link_libraries(tetris4-2 ${LIBS})
target_link_libraries(tetris4-3 ${LIBS})
target_link_libraries(tetris4-4 ${LIBS})

INSTALL(TARGETS tetris42 tetris4-1 tetris4-2 tetris4-3 tetris4-4
DESTINATION bin)

# CPack support -
set(CPACK_GENERATOR "ZIP;TGZ")
include (CPack)
