#pragma once
#include <box2d/box2d.h>
#include <memory>

/**
 * @file Catagotchi.h
 * @brief Catagotchi-pelin olioiden m‰‰rittely ja hallinta.
 * @note K‰ytet‰‰n smart pointereita (shared_ptr) ja Box2D:t‰.
 */

struct Cat {
    b2BodyId bodyId = { 0 }; ///< Box2D-body
    float halfWidth = 2.5f;  ///< Puolileveys ASCII-kissan piirt‰miseen
    int eaten = 0;            ///< Kuinka monta ruokaa kissa on syˆnyt
};

struct Food {
    b2BodyId bodyId = { 0 }; ///< Box2D-body
    bool active = false;      ///< Onko ruoka n‰kyviss‰
};

struct Poop {
    b2BodyId bodyId = { 0 }; ///< Box2D-body
    float x = 0.0f;           ///< X-koordinaatti
    bool active = false;       ///< Onko kakka n‰kyviss‰
};

using CatPtr = std::shared_ptr<Cat>;   ///< Smart pointer Cat-oliolle
using FoodPtr = std::shared_ptr<Food>; ///< Smart pointer Food-oliolle
using PoopPtr = std::shared_ptr<Poop>; ///< Smart pointer Poop-oliolle