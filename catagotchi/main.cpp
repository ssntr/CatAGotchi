#include <curses.h>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <atomic>
#include <chrono>
#include <thread>
#include "Catagotchi.h"
#include "Physics.h"
#include "InputHandler.h"
#include "Assets.h"

/*!
 \@mainpage CatAGotchi Demo
 
 \section intro_sec Introduction
 This is a small CatAGotchi demo using C++ with Box2D and PDCurses.
 
 \section tech_sec Techniques from course:
 - Threads (physics loop in separate thread) (also mutex usage, ofc)
 - Smart pointers (shared_ptr)
 - Box2D physics engine (library)
 - ASCII graphics with PDCurses (another library)
 - A lamda function in physics loop
 
 \section usage Usage
 - q = quit
 - f = add food
 - c = clean poop (if exists)
 */

std::mutex worldMu;

int main() {
    // Calculate title width to match screen width wid it
    int ascii_height = CATAGOTCHI_ASCII_HEIGHT;
    int ascii_width = 0;
    for (int i = 0; i < ascii_height; ++i) {
        int len = strlen(CATAGOTCHI_ASCII[i]);
        if (len > ascii_width) ascii_width = len;
    }

    // PDCurses basic callz
    initscr();
    noecho();
    curs_set(FALSE);
    nodelay(stdscr, TRUE);

    // calculate correct offsets etc for PDCurses printing, and also world dimensions fo Box2D
    int termRows = 0, termCols = 0;
    getmaxyx(stdscr, termRows, termCols);
    int virtualCols = ascii_width;
    int xOffset = (termCols - virtualCols) / 2;
    float worldWidth = (float)virtualCols;
    float worldHeight = (float)termRows;

    // Box2D World initialization
    b2WorldDef wdef = b2DefaultWorldDef();
    wdef.gravity = { 0.0f, 10.0f };
    b2WorldId worldId = b2CreateWorld(&wdef);

    // Ground initialization
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

    // Cat spawnage
    CatPtr cat = Cat::spawn(worldId, worldWidth * 0.5f, worldHeight - 2.0f);
    FoodPtr food = nullptr;
    PoopPtr poop = nullptr;

    // start the main loop and physics thread
    std::atomic<bool> running{ true };
    startPhysicsThread(worldId, running, worldMu);

    // Lastly, create the input handler class (moved these to a seperate script to shorten main function)
    InputHandler inputHandler(cat, food, poop, worldId, worldMu, virtualCols, ascii_height);

    while (running.load()) {
        clear();
        getmaxyx(stdscr, termRows, termCols);

        // ASCII title
        for (int i = 0; i < ascii_height; ++i)
            mvprintw(i, xOffset, "%s", CATAGOTCHI_ASCII[i]);
        for (int c = 0; c < virtualCols; ++c)
            mvaddch(termRows - 2, xOffset + c, '-');

        // Draw foodz and poopz
        if (poop) poop->draw(xOffset, termRows);
        if (food) food->draw(xOffset, termRows);

        // Cat AI
        cat->moveToward(food, poop, worldWidth);
        cat->tryEat(food, poop, worldId, worldWidth);
        cat->draw(xOffset, termCols, termRows);

        // Draw commands in the bottom of the screen
        if (poop && poop->active)
            mvprintw(termRows - 1, xOffset, "Commands: q=quit  f=feed  c=clean poop");
        else
            mvprintw(termRows - 1, xOffset, "Commands: q=quit  f=feed");

        refresh();

        // Call for input handling
        inputHandler.handleInput(running);

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    endwin();
    b2DestroyWorld(worldId);
    return 0;
}
