cmake_minimum_required(VERSION 3.20)
project(game LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE Release)
option(PLATFORM "platform (Desktop/Web)" Desktop)

add_library(shared STATIC
    src/Player.cpp
    src/Game.cpp
    src/Car.cpp
    src/Stepper.cpp
    src/HUD.cpp
    src/HumanCharacter.cpp
    src/DialogSystem.cpp
    src/Interactable.cpp
    src/NPC.cpp
    src/AdvancedText.cpp
    src/DrivableCar.cpp
)
target_include_directories(shared PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(shared PUBLIC lil_engine::lil_engine)

add_executable(game src/game.cpp)
add_executable(editor src/editor.cpp)

include(lil_engine_sdk/lil_engine.cmake)
target_link_libraries(game PRIVATE shared)
target_link_libraries(editor PRIVATE shared lil_engine::lil_editor)

# add_executable(game
#     src/Player.cpp
#     src/Game.cpp
#     src/Car.cpp
#     src/Stepper.cpp
#     src/HUD.cpp
#     src/HumanCharacter.cpp
#     src/game.cpp
# )

# target_include_directories(game PUBLIC ${CMAKE_SOURCE_DIR}/include)
# target_link_libraries(game PUBLIC lil_engine::lil_engine)

# include(lil_engine_sdk/lil_engine.cmake)

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -g -O1")
# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")