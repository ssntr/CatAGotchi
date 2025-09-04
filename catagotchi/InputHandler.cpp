#include "InputHandler.h"
#include <curses.h>
#include <cstdlib>
#include <memory>
#include <box2d/box2d.h>

InputHandler::InputHandler(CatPtr cat, FoodPtr& food, PoopPtr& poop,
    b2WorldId worldId, std::mutex& worldMu,
    int virtualCols, int asciiHeight)
    : cat(cat), food(food), poop(poop),
    worldId(worldId), worldMu(worldMu),
    virtualCols(virtualCols), asciiHeight(asciiHeight) {
}

void InputHandler::handleInput(std::atomic<bool>& running) {
    int ch = getch();
    if (ch == 'q' || ch == 'Q') {
        running = false;
    }
    else if ((ch == 'f' || ch == 'F') && (!food || !food->active)) {
        spawnFood();
    }
    else if ((ch == 'c' || ch == 'C') && poop && poop->active) {
        cleanPoop();
    }
}

void InputHandler::spawnFood() {
    std::lock_guard<std::mutex> lock(worldMu);
    float fx = static_cast<float>(4 + rand() % (virtualCols - 8));

    // Älä spawnata kakkan päälle
    if (poop && poop->active && fx >= poop->x - 1 && fx <= poop->x + 1)
        fx = poop->x + 2.0f;

    food = Food::spawn(worldId, fx, static_cast<float>(asciiHeight + 1));
}

void InputHandler::cleanPoop() {
    std::lock_guard<std::mutex> lock(worldMu);
    b2DestroyBody(poop->bodyId);
    poop->active = false;
}
