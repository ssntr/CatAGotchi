#pragma once
#include <box2d/box2d.h>
#include <memory>
#include <mutex>
#include "Catagotchi.h"

/**
 * @brief Handles user input for the CatAGotchi demo
 *
 * This class checks keypresses and performs actions such as spawning food
 * or cleaning poop. It interacts with the Box2D world safely using a mutex.
 */
class InputHandler {
public:
    InputHandler(CatPtr cat, FoodPtr& food, PoopPtr& poop,
        b2WorldId worldId, std::mutex& worldMu, int virtualCols, int asciiHeight);


    /**
     * @brief Checks keypresses and handles commands
     * @param running Atomic boolean to control main loop
     */
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
