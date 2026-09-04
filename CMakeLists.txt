cmake_minimum_required(VERSION 3.20)
project(game LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE Release)
option(PLATFORM "platform (Desktop/Web)" Desktop)

add_library(shared STATIC src/Player.cpp src/Game.cpp src/Car.cpp src/Stepper.cpp)
target_include_directories(shared PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(shared PUBLIC lil_engine::lil_engine)

add_executable(game src/game.cpp)
add_executable(editor src/editor.cpp)

include(lil_engine_sdk/lil_engine.cmake)
target_link_libraries(game PRIVATE shared)
target_link_libraries(editor PRIVATE shared lil_engine::lil_editor)