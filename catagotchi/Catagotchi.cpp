#include "Catagotchi.h"
#include <curses.h>

void Cat::draw(int xOffset, int termCols, int termRows) {
    b2Vec2 pos = b2Body_GetPosition(bodyId);
    int x = xOffset + (int)pos.x;
    int y = (int)pos.y;
    if (y >= 0 && y < termRows - 2 && x >= 0 && x + 4 < termCols) {
        mvprintw(y, x, "%s", sprite);
    }
}

void Food::draw(int xOffset, int termRows) {
    if (!active) return;
    b2Vec2 pos = b2Body_GetPosition(bodyId);
    mvaddch((int)pos.y, xOffset + (int)pos.x, *sprite);
}

void Poop::draw(int xOffset, int termRows) {
    if (!active) return;
    mvaddch(termRows - 3, xOffset + (int)x, *sprite);
}
