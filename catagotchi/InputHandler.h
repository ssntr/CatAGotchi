#pragma once
#include <box2d/box2d.h>
#include <memory>
#include <mutex>
#include "Catagotchi.h"

class InputHandler {
public:
    InputHandler(CatPtr cat, FoodPtr& food, PoopPtr& poop,
        b2WorldId worldId, std::mutex& worldMu, int virtualCols, int asciiHeight);

    // Tarkistaa ja käsittelee käyttäjän napit
    void handleInput(std::atomic<bool>& running);

private:
    CatPtr cat;
    FoodPtr& food;
    PoopPtr& poop;
    b2WorldId worldId;
    std::mutex& worldMu;
    int virtualCols;
    int asciiHeight;

    void spawnFood();
    void cleanPoop();
};
