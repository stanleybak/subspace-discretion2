#include "Ships.h"
#include "Graphics.h"
#include "Players.h"
#include "Net.h"
using namespace std;

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

    shared_ptr<Player> self;

    bool moveKeys[4] = {false, false, false, false};

    i32 totalMs = 0;

    bool inGame = false;

    void NowInGame() { inGame = true; }

    void InitDemo()
    {
        drawnShip = c.graphics->MakeDrawnImage(Layer_Ships, shipsImage);
        drawnPowerball = c.graphics->MakeDrawnAnimation(Layer_Ships, powerballAnim);
    }

    void DeinitDemo()
    {
        drawnShip = nullptr;
        drawnPowerball = nullptr;
    }

    void DrawDemo()
    {
        i32 ROTATE_TIME = 1500;
        i32 ROTATE_RADIUS = 40;
        i32 NUM_SHIPS = 8;
        i32 FRAMES_PER_SHIP = 40;
        float TWO_PI = 3.1415 * 2;
        u32 w, h;
        c.graphics->GetScreenSize(&w, &h);

        if (totalMs > NUM_SHIPS * ROTATE_TIME)
            totalMs -= NUM_SHIPS * ROTATE_TIME;

        u32 lastFrame = drawnShip->GetFrame();
        u32 curFrame = (FRAMES_PER_SHIP * totalMs / ROTATE_TIME) % (FRAMES_PER_SHIP * NUM_SHIPS);

        float radians = TWO_PI * totalMs / ROTATE_TIME;

        i32 centerX = w / 2;
        i32 centerY = h / 2;
        i32 x = centerX + ROTATE_RADIUS * -cos(radians);
        i32 y = centerY + ROTATE_RADIUS * -sin(radians);

        drawnShip->SetFrame(curFrame);
        drawnShip->SetCenteredScreenPosition(x, y);

        drawnPowerball->SetCenteredScreenPosition(centerX, centerY);

        if (lastFrame % FRAMES_PER_SHIP > curFrame % FRAMES_PER_SHIP)
        {
            c.graphics->MakeSingleDrawnAnimation((Layer)(Layer_Ships + 5), x, y,
                                                 explodeBombAnimation);
        }
    }

    void DrawInGame(i32 difMs)
    {
        int forwardMult = 0;
        int leftMult = 0;

        if (moveKeys[0] && !moveKeys[1])
            forwardMult = 1;
        else if (!moveKeys[0] && moveKeys[1])
            forwardMult = -1;

        if (moveKeys[2] && !moveKeys[3])
            leftMult = 1;
        else if (!moveKeys[2] && moveKeys[3])
            leftMult = -1;

        if (self->ship == Ship_Spec)
        {
            const int MOVE_SPEED = 5000;

            self->physics.y -= difMs * forwardMult * MOVE_SPEED;
            self->physics.x -= difMs * leftMult * MOVE_SPEED;
        }

        // draw the player frame
    }
};

Ships::Ships(Client& c) : Module(c), data(make_shared<ShipsData>(c))
{
    data->shipsImage = c.graphics->LoadImage("ships", 10, 32);
    data->powerballImage = c.graphics->LoadImage("powerb", 10, 3);

    data->powerballAnim = c.graphics->InitAnimation(data->powerballImage, 500, 0, 10);
    data->explodeBombImage = c.graphics->LoadImage("explode1", 6, 6);
    data->explodeBombAnimation = c.graphics->InitAnimation(data->explodeBombImage, 1000);

    data->InitDemo();
}

void Ships::AdvanceState(i32 difMs)
{
    data->totalMs += difMs;

    if (!data->inGame)
        data->DrawDemo();
    else
        data->DrawInGame(difMs);
}

void Ships::NowInGame()
{
    data->DeinitDemo();
    data->self = c.players->GetSelfPlayer();
    data->NowInGame();
}

void Ships::UpPressed(bool isPressed)
{
    data->moveKeys[0] = isPressed;
}

void Ships::DownPressed(bool isPressed)
{
    data->moveKeys[1] = isPressed;
}

void Ships::LeftPressed(bool isPressed)
{
    data->moveKeys[2] = isPressed;
}

void Ships::RightPressed(bool isPressed)
{
    data->moveKeys[3] = isPressed;
}

void Ships::RequestChangeShip(ShipType s)
{
    // send the ship request packet to the server
    PacketInstance pi("change ship request");
    pi.SetValue("ship", (i32)s);

    c.net->SendReliablePacket(&pi);
}
