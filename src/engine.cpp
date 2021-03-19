// 13519214
#include "header/render.hpp"
#include "header/engine.hpp"
#include "header/entities/map.hpp"
#include <iostream>
#include <stdio.h>

using namespace std;

enum Direction {
    Up,
    Down,
    Left,
    Right
};

Engine::Engine() : map(), renderer(MAP_OFFSET_X, MAP_OFFSET_Y, MESSAGE_OFFSET_X, MESSAGE_OFFSET_Y, map) {
    // TODO : Put private variable preparation here
}

void Engine::startGame() {
    // TODO : Put game here
    system(CLEAR_SCREEN_CMD);
    renderer.drawMap(map);
}

Engine::~Engine() {
    // TODO : Cleanup variable
}
