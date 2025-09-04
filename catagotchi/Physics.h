#pragma once
#include "Catagotchi.h"
#include <box2d/box2d.h>
#include <atomic>
#include <mutex>

/**
 * @file Physics.h
 * @brief Box2D fysiikkas�ie.
 * @note Kurssin s�ikeistys-tekniikka demonstroitu fysiikkap�ivityksiss�.
 */

void startPhysicsThread(b2WorldId worldId, std::atomic<bool>& running, std::mutex& worldMu);