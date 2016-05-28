#include "Ships.h"
#include "Graphics.h"

struct ShipsData
{
    Client& c;

    ShipsData(Client& c) : c(c){};

    shared_ptr<Image> shipsImage;
    shared_ptr<DrawnImage> drawnShip;

    shared_ptr<Image> powerballImage;
    shared_ptr<Animation> powerballAnim;
    shared_ptr<DrawnImage> drawnPowerball;

    shared_ptr<Image> explodeBombImage;
    shared_ptr<Animation> explodeBombAnimation;

    i32 totalMs = 0;
};

Ships::Ships(Client& c) : Module(c), data(make_shared<ShipsData>(c))
{
    data->shipsImage = c.graphics->LoadImage("ships", 10, 32);
    data->powerballImage = c.graphics->LoadImage("powerb", 10, 3);
    data->powerballAnim = make_shared<Animation>(data->powerballImage, 500, 0, 10);
    data->explodeBombImage = c.graphics->LoadImage("explode1", 6, 6);
    data->explodeBombAnimation = make_shared<Animation>(data->explodeBombImage, 1000);

    data->drawnShip = c.graphics->MakeDrawnImage(Layer_Ships, data->shipsImage);

    // u32 animMs, u32 animFrameOffset, u32 animNumFrames
    data->drawnPowerball = c.graphics->MakeDrawnAnimation(Layer_Ships, data->powerballAnim);
}

void Ships::AdvanceState(i32 difMs)
{
    data->totalMs += difMs;

    i32 ROTATE_TIME = 1500;
    i32 ROTATE_RADIUS = 40;
    i32 NUM_SHIPS = 8;
    i32 FRAMES_PER_SHIP = 40;
    float TWO_PI = 3.1415 * 2;
    u32 w, h;
    c.graphics->GetScreenSize(&w, &h);

    if (data->totalMs > NUM_SHIPS * ROTATE_TIME)
        data->totalMs -= NUM_SHIPS * ROTATE_TIME;

    u32 lastFrame = data->drawnShip->GetFrame();
    u32 curFrame = (FRAMES_PER_SHIP * data->totalMs / ROTATE_TIME) % (FRAMES_PER_SHIP * NUM_SHIPS);

    float radians = TWO_PI * data->totalMs / ROTATE_TIME;

    i32 centerX = w / 2;
    i32 centerY = h / 2;
    i32 x = centerX + ROTATE_RADIUS * -cos(radians);
    i32 y = centerY + ROTATE_RADIUS * -sin(radians);

    data->drawnShip->SetFrame(curFrame);
    data->drawnShip->SetCenteredScreenPosition(x, y);

    data->drawnPowerball->SetCenteredScreenPosition(centerX, centerY);

    if (lastFrame % FRAMES_PER_SHIP > curFrame % FRAMES_PER_SHIP)
    {
        c.graphics->MakeSingleDrawnAnimation((Layer)(Layer_Ships + 5), x, y,
                                             data->explodeBombAnimation);
    }
}
