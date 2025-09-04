#pragma once
#include <box2d/box2d.h>
#include <memory>

struct Food;
struct Poop;

struct Cat {
    b2BodyId bodyId{};
    float halfWidth = 2.0f;
    int eaten = 0;
    const char* sprite = "=^.^=";

    static std::shared_ptr<Cat> spawn(b2WorldId worldId, float x, float y);

    void moveToward(std::shared_ptr<Food> food, std::shared_ptr<Poop> poop, float worldWidth);
    void tryEat(std::shared_ptr<Food>& food, std::shared_ptr<Poop>& poop, b2WorldId worldId, float worldWidth);

    void draw(int xOffset, int termCols, int termRows);
};
using CatPtr = std::shared_ptr<Cat>;

struct Food {
    b2BodyId bodyId{};
    bool active = false;
    const char* sprite = "*";

    static std::shared_ptr<Food> spawn(b2WorldId worldId, float x, float y);
    void draw(int xOffset, int termRows);
};
using FoodPtr = std::shared_ptr<Food>;

struct Poop {
    b2BodyId bodyId{};
    bool active = false;
    float x = 0.0f;
    const char* sprite = "o";

    static std::shared_ptr<Poop> spawn(b2WorldId worldId, const b2Vec2& pos);
    void draw(int xOffset, int termRows);
};
using PoopPtr = std::shared_ptr<Poop>;
