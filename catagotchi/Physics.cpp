#include "Physics.h"
#include <thread>
#include <chrono>

/**
 * @brief Starts a separate thread to step the Box2D world
 * @param worldId Box2D world
 * @param running Atomic flag to control the loop
 * @param worldMu Mutex to serialize Box2D calls
 *
 * Demonstrates multi-threading with a lambda function.
 */
void startPhysicsThread(b2WorldId worldId, std::atomic<bool>& running, std::mutex& worldMu) {
    const float timeStep = 1.0f / 60.0f;
    const int subSteps = 4;

    std::thread([=, &running, &worldMu]() {
        while (running.load()) {
            {
                std::lock_guard<std::mutex> lock(worldMu);
                b2World_Step(worldId, timeStep, subSteps);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
        }).detach();
}