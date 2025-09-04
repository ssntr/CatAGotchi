#include <curses.h>
#include <cstdlib>
#include <cstring>
#include <memory>
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
    CatPtr cat = Cat::spawn(worldId, worldWidth * 0.5f, worldHeight - 2.0f);
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

        // --- Draw objects
        if (poop) poop->draw(xOffset, termRows);
        if (food) food->draw(xOffset, termRows);

        // --- Cat AI
        cat->moveToward(food, poop, worldWidth);
        cat->tryEat(food, poop, worldId, worldWidth);
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
            float fx = (float)(4 + rand() % (virtualCols - 8));
            // Vältä spawnata kakkan päälle
            if (poop && poop->active) {
                if (fx >= poop->x - 1 && fx <= poop->x + 1)
                    fx = poop->x + 2.0f;
            }
            food = Food::spawn(worldId, fx, (float)(ascii_height + 1));
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
