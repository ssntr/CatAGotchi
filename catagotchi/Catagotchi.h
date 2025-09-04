#pragma once
#include <box2d/box2d.h>
#include <memory>

/// Kissa
struct Cat {
    b2BodyId bodyId{};
    float halfWidth = 2.0f;
    int eaten = 0;
    const char* sprite = "=^.^=";  ///< ASCII-ulkonäkö

    void draw(int xOffset, int termCols, int termRows);
};
using CatPtr = std::shared_ptr<Cat>;

/// Ruoka
struct Food {
    b2BodyId bodyId{};
    bool active = false;
    const char* sprite = "*"; ///< ASCII-ulkonäkö

    void draw(int xOffset, int termRows);
};
using FoodPtr = std::shared_ptr<Food>;

/// Kakka
struct Poop {
    b2BodyId bodyId{};
    bool active = false;
    float x = 0.0f;
    const char* sprite = "o"; ///< ASCII-ulkonäkö

    void draw(int xOffset, int termRows);
};
using PoopPtr = std::shared_ptr<Poop>;
