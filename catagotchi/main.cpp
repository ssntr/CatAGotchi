/*!
\mainpage Catagotchi Demo

\section intro_sec Projektin esittely
Tämä on pieni Catagotchi-demo, jossa kissa liikkuu, syö ruokaa ja tekee kakkaa.
Demo hyödyntää C++:aa, PDCurses-kirjastoa tekstipohjaiseen grafiikkaan ja Box2D:ta fysiikan simulointiin.
Otsikon ASCII-grafiikka generoitu osoitteessa https://patorjk.com/software/taag/

\section tech_sec Käytetyt tekniikat
- Säikeistys (physics-silmukka erillisessä threadissä)
- Smart pointerit (std::shared_ptr)
- Box2D-fysiikkamoottorin hyödyntäminen
- ASCII-grafiikka ja PDCurses

\section usage Käyttöohjeet
- q = lopeta demo
- f = lisää ruoka
- c = siivoa kakka (jos esiintyy)

*/

/**
 * @file main.cpp
 * @brief Pieni CatAGotchi-demo Box2D:llä ja PDCursesilla.
 *
 * Demon tarkoituksena on havainnollistaa C++-ohjelmointia, säikeistystä
 * ja Box2D-rajapinnan käyttöä.
 */

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

 /** ASCII-grafiikka kissalle */
static const char* CAT_SPRITE = "=^.^=";
const char* CATAGOTCHI_ASCII[] = {
    " ______         __   _______ _______         __         __     __ ",
    "|      |.---.-.|  |_|   _   |     __|.-----.|  |_.----.|  |--.|__|",
    "|   ---||  _  ||   _|       |    |  ||  _  ||   _|  __||     ||  |",
    "|______||___._||____|___|___|_______||_____||____|____||__|__||__|"
};

int main() {
    // --- ASCII-ruudukon koko ---
    const int ascii_height = 4;
    int ascii_width = 0;
    for (int i = 0; i < ascii_height; ++i) {
        int len = strlen(CATAGOTCHI_ASCII[i]);
        if (len > ascii_width) ascii_width = len;
    }

    // --- PDCurses-asetukset ---
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

    // --- Box2D-maailma ---
    b2WorldDef wdef = b2DefaultWorldDef();
    wdef.gravity = { 0.0f, 10.0f };
    b2WorldId worldId = b2CreateWorld(&wdef);

    // --- Lattia (static body) ---
    b2BodyDef gbd = b2DefaultBodyDef();
    gbd.type = b2_staticBody;
    gbd.position = { worldWidth * 0.5f, worldHeight - 1.0f };
    b2BodyId groundId = b2CreateBody(worldId, &gbd);
    b2Polygon groundBox = b2MakeBox(worldWidth * 0.5f, 1.0f);
    b2ShapeDef gsd = b2DefaultShapeDef();
    b2ShapeId groundShape = b2CreatePolygonShape(groundId, &gsd, &groundBox);
    b2Shape_SetFriction(groundShape, 0.8f);

    // --- Cat (dynamic body) ---
    CatPtr cat = std::make_shared<Cat>(); /**< Smart pointer - käytetty kurssilla */
    std::mutex worldMu;                    /**< Yksi mutex kaikkialle (ei paikallisia kaksoiskopioita) */

    {
        b2BodyDef bd = b2DefaultBodyDef();
        bd.type = b2_dynamicBody;
        bd.position = { worldWidth * 0.5f, worldHeight - 2.0f };
        std::lock_guard<std::mutex> lock(worldMu);
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
    // Käynnistä fysiikkasäie (oletetaan että startPhysicsThread käyttää samaa worldMu)
    startPhysicsThread(worldId, running, worldMu);

    // Pääkierros
    while (running.load()) {
        clear();
        getmaxyx(stdscr, termRows, termCols);

        // Päivitä keskeytyksettä muuttuvat arvot (xOffset voi muuttua jos ikkuna muuttuu)
        xOffset = (termCols - virtualCols) / 2;

        // --- Piirrä ASCII-tausta ---
        for (int i = 0; i < ascii_height; ++i)
            mvprintw(i, xOffset, "%s", CATAGOTCHI_ASCII[i]);

        for (int c = 0; c < virtualCols; ++c)
            mvaddch(termRows - 2, xOffset + c, '-');

        // --- Piirrä kakka (jos on) ---
        if (poop && poop->active) {
            // Poop position stored in poop->x (world coords). piirtoraja tarkistetaan.
            int px = xOffset + (int)poop->x;
            if (px >= 0 && px < termCols) mvaddch(termRows - 3, px, 'o');
        }

        // --- Piirrä ruoka (jos aktiivinen) ---
        if (food && food->active) {
            b2Vec2 fpos;
            {
                std::lock_guard<std::mutex> lock(worldMu);
                fpos = b2Body_GetPosition(food->bodyId);
            }
            int fx = xOffset + (int)fpos.x;
            int fy = (int)fpos.y;
            if (fy >= ascii_height && fy < termRows - 2 && fx >= 0 && fx < termCols)
                mvaddch(fy, fx, '*');
        }

        // --- Kissan logiikka (liike ja syönti) ---
        b2Vec2 cpos;
        {
            std::lock_guard<std::mutex> lock(worldMu);
            cpos = b2Body_GetPosition(cat->bodyId);
        }

        float dx = 0.0f;
        float speed = 1.5f;

        // Jos ruokaa on, laske ruoan x
        bool foodActive = (food && food->active);
        float foodX = 0.0f;
        if (foodActive) {
            std::lock_guard<std::mutex> lock(worldMu);
            foodX = b2Body_GetPosition(food->bodyId).x;
        }

        // Jos kakka on olemassa ja se on "välissä" kissan ja ruoan välillä, käsittele erikseen
        bool poopActive = (poop && poop->active);
        bool poopBetween = false;
        if (poopActive && foodActive) {
            // onko poop x kissan ja ruoan välillä?
            float px = poop->x;
            if ((px > cpos.x && px < foodX) || (px < cpos.x && px > foodX)) poopBetween = true;
        }

        if (foodActive) {
            if (poopActive && poopBetween) {
                // Jos kakka on välissä kissan ja ruoan välillä, lähde ensin kohti kakan reunaa (ei päälle)
                float px = poop->x;
                // määritellään tavoite: hieman kakan sivuun (cat.halfWidth + 1)
                float safeGap = cat->halfWidth + 1.0f;
                float targetX;
                // Jos ruoka on poopin oikealla puolella, mene poopin vasemmalle laidalle (poop.x - safeGap)
                if (foodX > px) targetX = px - safeGap;
                else targetX = px + safeGap;

                dx = targetX - cpos.x;
            }
            else {
                // Normaali: liiku kohti ruokaa
                dx = foodX - cpos.x;
            }
        }
        else {
            // Ei ruokaa -> jos kakkaa lähellä, väistä hieman
            if (poopActive) {
                float px = poop->x;
                if (fabsf(cpos.x - px) < 4.0f) {
                    dx = (cpos.x < px) ? -0.5f : 0.5f; // väistä
                }
                else {
                    dx = 0.0f;
                }
            }
            else {
                dx = 0.0f;
            }
        }

        // Rajoitukset: ettei kissa mene ruudun ulkopuolelle
        float leftBound = cat->halfWidth;
        float rightBound = worldWidth - cat->halfWidth;
        float targetXtest = cpos.x + dx * 0.05f;
        if (targetXtest < leftBound || targetXtest > rightBound) dx = 0.0f;

        {
            std::lock_guard<std::mutex> lock(worldMu);
            b2Body_SetLinearVelocity(cat->bodyId, { dx * speed, 0.0f });
        }

        // --- Syönti & kakka (kun kissa lähellä ruokaa) ---
        if (foodActive) {
            b2Vec2 fpos;
            {
                std::lock_guard<std::mutex> lock(worldMu);
                fpos = b2Body_GetPosition(food->bodyId);
            }
            // kun kissa on 'lähellä' ruokaa -> syö
            if (fabs((int)cpos.x - (int)fpos.x) <= 4 && fabs((int)cpos.y - (int)fpos.y) <= 2) {
                {
                    std::lock_guard<std::mutex> lock(worldMu);
                    b2DestroyBody(food->bodyId);
                    food->active = false;
                }
                // käytä cat->eaten-kenttää
                cat->eaten++;
                if (cat->eaten >= 5 && (!poop || !poop->active)) {
                    // luo kakka kissan paikalle (maassa)
                    poop = std::make_shared<Poop>();
                    {
                        std::lock_guard<std::mutex> lock(worldMu);
                        b2BodyDef pd = b2DefaultBodyDef();
                        pd.type = b2_staticBody;
                        pd.position = cpos;
                        poop->bodyId = b2CreateBody(worldId, &pd);
                        poop->active = true;
                        poop->x = pd.position.x;
                    }

                    // Impulssi: suunta poispäin kakasta (lasketaan suhteessa kakan sijaintiin)
                    float dxrel = cpos.x - poop->x;
                    float impulse = (dxrel >= 0.0f) ? 450.0f : -450.0f;
                    {
                        std::lock_guard<std::mutex> lock(worldMu);
                        b2Body_ApplyLinearImpulseToCenter(cat->bodyId, { impulse, 0.0f }, true);
                        // Jos kissan x on liian lähellä reunaa, "pehmennä" nopeutta
                        if (cpos.x < 3.0f) {
                            b2Body_SetLinearVelocity(cat->bodyId, { fabsf(impulse) * 0.01f, 0.0f });
                        }
                        else if (cpos.x > worldWidth - 3.0f) {
                            b2Body_SetLinearVelocity(cat->bodyId, { -fabsf(impulse) * 0.01f, 0.0f });
                        }
                    }

                    cat->eaten = 0;
                }
            }
        }

        // --- Piirrä kissa (varmista piirtoalueet oikein) ---
        {
            std::lock_guard<std::mutex> lock(worldMu);
            cpos = b2Body_GetPosition(cat->bodyId);
        }
        int drawX = xOffset + (int)cpos.x;
        int drawY = (int)cpos.y;
        if (drawY < ascii_height) drawY = ascii_height;
        if (drawY > termRows - 3) drawY = termRows - 3;
        if (drawX < 0) drawX = 0;
        if (drawX + (int)strlen(CAT_SPRITE) >= termCols) drawX = termCols - 1 - (int)strlen(CAT_SPRITE);

        if (drawY >= ascii_height && drawY < termRows - 2)
            mvprintw(drawY, drawX, "%s", CAT_SPRITE);

        // --- Komennot ---
        if (poop && poop->active)
            mvprintw(termRows - 1, xOffset, "Commands: q=quit  f=feed  c=clean poop");
        else
            mvprintw(termRows - 1, xOffset, "Commands: q=quit  f=feed");

        refresh();

        // --- Käyttäjän syöte ---
        int ch = getch();
        if (ch == 'q' || ch == 'Q') {
            running = false;
        }
        else if ((ch == 'f' || ch == 'F') && (!food || !food->active)) {
            // spawn food (älä spawnaa kakan päälle)
            std::lock_guard<std::mutex> lock(worldMu);
            food = std::make_shared<Food>();
            b2BodyDef fd = b2DefaultBodyDef();
            fd.type = b2_dynamicBody;
            float fx;
            do {
                fx = (float)(4 + rand() % (virtualCols - 8));
            } while (poop && poop->active && fx >= poop->x - 1.0f && fx <= poop->x + 1.0f);
            fd.position = { fx, (float)(ascii_height + 1) };
            food->bodyId = b2CreateBody(worldId, &fd);

            b2Polygon fBox = b2MakeBox(0.5f, 0.5f);
            b2ShapeDef sd = b2DefaultShapeDef();
            sd.density = 0.8f;
            b2ShapeId s = b2CreatePolygonShape(food->bodyId, &sd, &fBox);
            b2Shape_SetFriction(s, 0.3f);
            b2Body_SetLinearVelocity(food->bodyId, { 0.0f, 10.0f }); // nopea tippuminen
            food->active = true;
        }
        else if ((ch == 'c' || ch == 'C') && poop && poop->active) {
            std::lock_guard<std::mutex> lock(worldMu);
            b2DestroyBody(poop->bodyId);
            poop->active = false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // Lopeta
    endwin();
    b2DestroyWorld(worldId);
    return 0;
}
