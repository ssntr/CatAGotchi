#include <curses.h>
#include <box2d/box2d.h>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <mutex>
#include <atomic>
#include <chrono>
#include <thread>
#include "Catagotchi.h"
#include "Physics.h"
#include "Assets.h"

// Yhteinen mutex Box2D-kutsuille
std::mutex worldMu;

int main() {
    // --- ASCII-otsikko
    int ascii_height = CATAGOTCHI_ASCII_HEIGHT;
    int ascii_width = 0;
    for (int i = 0; i < ascii_height; ++i) {
        int len = strlen(CATAGOTCHI_ASCII[i]);
        if (len > ascii_width) ascii_width = len;
    }

    // --- PDCurses
    initscr();
    noecho();
    curs_set(FALSE);
    nodelay(stdscr, TRUE);

    int termRows = 0, termCols = 0;
    getmaxyx(stdscr, termRows, termCols);
    int virtualCols = ascii_width;
    int xOffset = (termCols - virtualCols) / 2;
    float worldWidth = (float)virtualCols;
    float worldHeight = (float)termRows;

    // --- Box2D world
    b2WorldDef wdef = b2DefaultWorldDef();
    wdef.gravity = { 0.0f, 10.0f };
    b2WorldId worldId = b2CreateWorld(&wdef);

    // --- Ground
    {
        std::lock_guard<std::mutex> lock(worldMu);
        b2BodyDef gbd = b2DefaultBodyDef();
        gbd.type = b2_staticBody;
        gbd.position = { worldWidth * 0.5f, worldHeight - 1.0f };
        b2BodyId groundId = b2CreateBody(worldId, &gbd);
        b2Polygon groundBox = b2MakeBox(worldWidth * 0.5f, 1.0f);
        b2ShapeDef gsd = b2DefaultShapeDef();
        b2ShapeId groundShape = b2CreatePolygonShape(groundId, &gsd, &groundBox);
        b2Shape_SetFriction(groundShape, 0.8f);
    }

    // --- Cat
    CatPtr cat = std::make_shared<Cat>();
    {
        std::lock_guard<std::mutex> lock(worldMu);
        b2BodyDef bd = b2DefaultBodyDef();
        bd.type = b2_dynamicBody;
        bd.position = { worldWidth * 0.5f, worldHeight - 2.0f };
        cat->bodyId = b2CreateBody(worldId, &bd);

        b2Polygon catBox = b2MakeBox(cat->halfWidth, 0.5f);
        b2ShapeDef sd = b2DefaultShapeDef();
        sd.density = 3.0f;
        b2ShapeId s = b2CreatePolygonShape(cat->bodyId, &sd, &catBox);
        b2Shape_SetFriction(s, 0.3f);
    }

    FoodPtr food = nullptr;
    PoopPtr poop = nullptr;
    std::atomic<bool> running{ true };
    startPhysicsThread(worldId, running, worldMu);

    while (running.load()) {
        clear();
        getmaxyx(stdscr, termRows, termCols);

        // --- ASCII title
        for (int i = 0; i < ascii_height; ++i)
            mvprintw(i, xOffset, "%s", CATAGOTCHI_ASCII[i]);
        for (int c = 0; c < virtualCols; ++c)
            mvaddch(termRows - 2, xOffset + c, '-');

        // --- Draw poop
        if (poop) poop->draw(xOffset, termRows);

        // --- Draw food
        if (food) food->draw(xOffset, termRows);

        // --- Cat logic
        b2Vec2 cpos = b2Body_GetPosition(cat->bodyId);
        float dx = 0.0f, speed = 1.5f;

        if (food && food->active) {
            float foodX = b2Body_GetPosition(food->bodyId).x;

            if (poop && poop->active) {
                // Jos kakka on välissä
                if ((cpos.x < poop->x && foodX > poop->x) ||
                    (cpos.x > poop->x && foodX < poop->x)) {
                    // Väistä kakkaa: käänny poispäin siitä
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

        float leftBound = cat->halfWidth, rightBound = worldWidth - cat->halfWidth;
        float targetX = cpos.x + dx * 0.05f;
        if (targetX < leftBound || targetX > rightBound) dx = 0.0f;
        b2Body_SetLinearVelocity(cat->bodyId, { dx * speed, 0.0f });

        // --- Eating logic
        if (food && food->active) {
            b2Vec2 fpos = b2Body_GetPosition(food->bodyId);
            if (abs((int)cpos.x - (int)fpos.x) <= 4 && abs((int)cpos.y - (int)fpos.y) <= 2) {
                std::lock_guard<std::mutex> lock(worldMu);
                b2DestroyBody(food->bodyId);
                food->active = false;

                cat->eaten++;
                if (cat->eaten >= 5 && (!poop || !poop->active)) {
                    poop = std::make_shared<Poop>();
                    float dxrel = 0.0f;
                    float impulse = 0.0f;

                    
                    // Luo kakka
                    b2BodyDef pd = b2DefaultBodyDef();
                    pd.type = b2_staticBody;
                    pd.position = cpos;
                    poop->bodyId = b2CreateBody(worldId, &pd);
                    poop->active = true;
                    poop->x = pd.position.x;

                    // Laske impulssi
                    dxrel = cpos.x - poop->x;
                    impulse = (dxrel >= 0.0f) ? 450.0f : -450.0f;

                    // Sovella impulssi
                    b2Body_ApplyLinearImpulseToCenter(cat->bodyId, { impulse, 0.0f }, true);

                    // Pehmennä reunojen läheisyydessä
                    if (cpos.x < 3.0f) {
                        b2Body_SetLinearVelocity(cat->bodyId, { fabsf(impulse) * 0.01f, 0.0f });
                    }
                    else if (cpos.x > worldWidth - 3.0f) {
                        b2Body_SetLinearVelocity(cat->bodyId, { -fabsf(impulse) * 0.01f, 0.0f });
                    }
                    

                    cat->eaten = 0;
                }
            }
        }

        // --- Draw cat
        cat->draw(xOffset, termCols, termRows);

        // --- Commands
        if (poop && poop->active)
            mvprintw(termRows - 1, xOffset, "Commands: q=quit  f=feed  c=clean poop");
        else
            mvprintw(termRows - 1, xOffset, "Commands: q=quit  f=feed");

        refresh();

        // --- Input
        int ch = getch();
        if (ch == 'q' || ch == 'Q') running = false;
        else if ((ch == 'f' || ch == 'F') && (!food || !food->active)) {
            std::lock_guard<std::mutex> lock(worldMu);
            food = std::make_shared<Food>();
            b2BodyDef fd = b2DefaultBodyDef();
            fd.type = b2_dynamicBody;
            fd.position = { (float)(4 + rand() % (virtualCols - 8)), (float)(ascii_height + 1) };
            food->bodyId = b2CreateBody(worldId, &fd);
            b2Polygon fBox = b2MakeBox(0.5f, 0.5f);
            b2ShapeDef sd = b2DefaultShapeDef();
            sd.density = 0.8f;
            b2ShapeId s = b2CreatePolygonShape(food->bodyId, &sd, &fBox);
            b2Shape_SetFriction(s, 0.3f);
            b2Body_SetLinearVelocity(food->bodyId, { 0.0f, 10.0f });
            food->active = true;
        }
        else if ((ch == 'c' || ch == 'C') && poop && poop->active) {
            std::lock_guard<std::mutex> lock(worldMu);
            b2DestroyBody(poop->bodyId);
            poop->active = false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    endwin();
    b2DestroyWorld(worldId);
    return 0;
}
