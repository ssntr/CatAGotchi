#include "Physics.h"
#include <thread>
#include <chrono>

/**
 * @brief Fysiikkas‰ie Box2D:lle
 * @param worldId Box2D-maailman tunniste
 * @param running Atominen lippu, jolla voidaan pys‰ytt‰‰ s‰ie
 * @param worldMu Mutex Box2D-kutsujen sarjallistamiseen
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