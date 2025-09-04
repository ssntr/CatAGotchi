#pragma once
#include "Catagotchi.h"
#include <box2d/box2d.h>
#include <atomic>
#include <mutex>

/**
 * @file Physics.h
 * @brief Box2D fysiikkasäie.
 * @note Kurssin säikeistys-tekniikka demonstroitu fysiikkapäivityksissä.
 */

void startPhysicsThread(b2WorldId worldId, std::atomic<bool>& running, std::mutex& worldMu);