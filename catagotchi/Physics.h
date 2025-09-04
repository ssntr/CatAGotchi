#pragma once
#include <box2d/box2d.h>
#include <mutex>
#include <atomic>

/// K‰ynnist‰‰ fysiikkas‰ikeen
void startPhysicsThread(b2WorldId worldId, std::atomic<bool>& running, std::mutex& worldMu);
