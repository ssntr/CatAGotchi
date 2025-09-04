#include "Catagotchi.h"
#include <curses.h>
#include <cmath>

/**
 * @file Catagotchi.cpp
 * @brief Implementation of Cat, Food, and Poop classes.
 */

 /**
  * @brief Spawn a new Cat in the Box2D world.
  * @param worldId Box2D world identifier
  * @param x Initial x-position
  * @param y Initial y-position
  * @return Shared pointer to the new Cat
  */
CatPtr Cat::spawn(b2WorldId worldId, float x, float y) {
    CatPtr cat = std::make_shared<Cat>();
    b2BodyDef bd = b2DefaultBodyDef();
    bd.type = b2_dynamicBody;
    bd.position = { x, y };
    cat->bodyId = b2CreateBody(worldId, &bd);

    b2Polygon catBox = b2MakeBox(cat->halfWidth, 0.5f);
    b2ShapeDef sd = b2DefaultShapeDef();
    sd.density = 3.0f;
    b2ShapeId s = b2CreatePolygonShape(cat->bodyId, &sd, &catBox);
    b2Shape_SetFriction(s, 0.3f);

    return cat;
}

/**
 * @brief Move the Cat toward the Food while avoiding Poop.
 * @param food Shared pointer to Food object
 * @param poop Shared pointer to Poop object
 * @param worldWidth Width of the Box2D world
 */
void Cat::moveToward(FoodPtr food, PoopPtr poop, float worldWidth) {
    b2Vec2 cpos = b2Body_GetPosition(bodyId);
    float dx = 0.0f, speed = 1.5f;

    if (food && food->active) {
        float foodX = b2Body_GetPosition(food->bodyId).x;

        if (poop && poop->active) {
            // Avoid poop if it is between Cat and Food
            if ((cpos.x < poop->x && foodX > poop->x) ||
                (cpos.x > poop->x && foodX < poop->x)) {
                dx = (cpos.x < poop->x) ? -0.5f : 0.5f;
            }
            else {
                dx = foodX - cpos.x;
            }
        }
        else {
            dx = foodX - cpos.x;
        }
    }

    float leftBound = halfWidth;
    float rightBound = worldWidth - halfWidth;
    float targetX = cpos.x + dx * 0.05f;
    if (targetX < leftBound || targetX > rightBound) dx = 0.0f;
    b2Body_SetLinearVelocity(bodyId, { dx * speed, 0.0f });
}

/**
 * @brief Attempt to eat the food and possibly spawn poop.
 * @param food Reference to shared pointer to Food
 * @param poop Reference to shared pointer to Poop
 * @param worldId Box2D world identifier
 * @param worldWidth Width of the Box2D world
 */
void Cat::tryEat(FoodPtr& food, PoopPtr& poop, b2WorldId worldId, float worldWidth) {
    if (!food || !food->active) return;

    b2Vec2 cpos = b2Body_GetPosition(bodyId);
    b2Vec2 fpos = b2Body_GetPosition(food->bodyId);

    if (abs((int)cpos.x - (int)fpos.x) <= 4 && abs((int)cpos.y - (int)fpos.y) <= 2) {
        b2DestroyBody(food->bodyId);
        food->active = false;

        eaten++;
        if (eaten >= 5 && (!poop || !poop->active)) {
            //Here, the poop is zzpawned and the cat is moved away so it doesn't stand on it :3
            poop = Poop::spawn(worldId, cpos);
            eaten = 0;

            float impulse = (cpos.x < worldWidth / 2) ? -450.0f : 450.0f;
            b2Body_ApplyLinearImpulseToCenter(bodyId, { impulse, 0.0f }, true);

            if (cpos.x < 3.0f)
                b2Body_SetLinearVelocity(bodyId, { fabsf(impulse) * 0.01f, 0.0f });
            else if (cpos.x > worldWidth - 3.0f)
                b2Body_SetLinearVelocity(bodyId, { -fabsf(impulse) * 0.01f, 0.0f });
        }
    }
}

/**
 * @brief Draw the Cat using ASCII in terminal.
 * @param xOffset Horizontal offset in terminal
 * @param termCols Terminal width
 * @param termRows Terminal height
 */
void Cat::draw(int xOffset, int termCols, int termRows) {
    b2Vec2 pos = b2Body_GetPosition(bodyId);
    int x = xOffset + (int)pos.x;
    int y = (int)pos.y;
    if (y >= 0 && y < termRows - 2 && x >= 0 && x + 4 < termCols)
        mvprintw(y, x, "%s", sprite);
}


/**
 * @brief Spawn a new Food in the Box2D world.
 * @param worldId Box2D world identifier
 * @param x Initial x-position
 * @param y Initial y-position
 * @return Shared pointer to the new Food
 */
FoodPtr Food::spawn(b2WorldId worldId, float x, float y) {
    FoodPtr food = std::make_shared<Food>();
    b2BodyDef fd = b2DefaultBodyDef();
    fd.type = b2_dynamicBody;
    fd.position = { x, y };
    food->bodyId = b2CreateBody(worldId, &fd);

    b2Polygon fBox = b2MakeBox(0.5f, 0.5f);
    b2ShapeDef sd = b2DefaultShapeDef();
    sd.density = 0.8f;
    b2ShapeId s = b2CreatePolygonShape(food->bodyId, &sd, &fBox);
    b2Shape_SetFriction(s, 0.3f);
    b2Body_SetLinearVelocity(food->bodyId, { 0.0f, 10.0f });
    food->active = true;

    return food;
}

/**
 * @brief Draw the Food using ASCII in terminal.
 * @param xOffset Horizontal offset in terminal
 * @param termRows Terminal height
 */
void Food::draw(int xOffset, int termRows) {
    if (!active) return;
    b2Vec2 pos = b2Body_GetPosition(bodyId);
    mvaddch((int)pos.y, xOffset + (int)pos.x, *sprite);
}

/**
 * @brief Spawn a new Poop in the Box2D world.
 * @param worldId Box2D world identifier
 * @param pos Position to spawn the Poop
 * @return Shared pointer to the new Poop
 */
PoopPtr Poop::spawn(b2WorldId worldId, const b2Vec2& pos) {
    PoopPtr poop = std::make_shared<Poop>();
    b2BodyDef pd = b2DefaultBodyDef();
    pd.type = b2_staticBody;
    pd.position = pos;
    poop->bodyId = b2CreateBody(worldId, &pd);
    poop->active = true;
    poop->x = pos.x;
    return poop;
}

/**
 * @brief Draw the Poop using ASCII in terminal.
 * @param xOffset Horizontal offset in terminal
 * @param termRows Terminal height
 */
void Poop::draw(int xOffset, int termRows) {
    if (!active) return;
    mvaddch(termRows - 3, xOffset + (int)x, *sprite);
}
