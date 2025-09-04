#pragma once
#include <box2d/box2d.h>
#include <mutex>
#include <atomic>

/**
 * @file Physics.h
 * @brief Functions for physics thread management
 *
 * Contains a function to start a Box2D physics thread
 * demonstrating multi-threading
 */
void startPhysicsThread(b2WorldId worldId, std::atomic<bool>& running, std::mutex& worldMu);
