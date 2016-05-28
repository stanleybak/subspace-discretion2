#include "Ships.h"
#include "Graphics.h"

struct ShipsData
{
    Client& c;

    ShipsData(Client& c) : c(c){};

    shared_ptr<Image> shipsImage;
    shared_ptr<DrawnImage> selfShip;
    i32 totalMs = 0;
};

Ships::Ships(Client& c) : Module(c), data(make_shared<ShipsData>(c))
{
    data->shipsImage = c.graphics->LoadImage("ships", 10, 32);

    data->selfShip = c.graphics->MakeDrawnImage(Layer_Ships, data->shipsImage);
}

void Ships::AdvanceState(i32 difMs)
{
    data->totalMs += difMs;

    i32 ROTATE_TIME = 1200;
    i32 ROTATE_RADIUS = 40;
    i32 NUM_SHIPS = 8;
    i32 FRAMES_PER_SHIP = 40;
    float TWO_PI = 3.1415 * 2;
    i32 w, h;
    c.graphics->GetScreenSize(&w, &h);

    if (data->totalMs > NUM_SHIPS * ROTATE_TIME)
        data->totalMs -= NUM_SHIPS * ROTATE_TIME;

    i32 curFrame = (FRAMES_PER_SHIP * data->totalMs / ROTATE_TIME) % (FRAMES_PER_SHIP * NUM_SHIPS);

    float radians = TWO_PI * data->totalMs / ROTATE_TIME;

    i32 x = w / 2 + ROTATE_RADIUS * -cos(radians);
    i32 y = h / 2 + ROTATE_RADIUS * -sin(radians);

    data->selfShip->SetFrame(curFrame);
    data->selfShip->SetCenteredScreenPosition(x, y);
}
