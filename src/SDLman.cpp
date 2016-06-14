
#include "SDLman.h"
#include "Graphics.h"
#include "Ships.h"
#include "Chat.h"
#include "Timers.h"
#include "Connection.h"
#include "Net.h"

struct SDLmanData
{
    SDLmanData(Client& client) : c(client) {}
    Client& c;
    i32 fpsFrameCount = 0;
    shared_ptr<Timer> fpsTimer;
    shared_ptr<DrawnText> fpsText;
    shared_ptr<DrawnText> shipInfoText;

    bool shouldExit = false;
    bool escPressedState = false;

    void EscapeToggled();
    void AdvanceState(i32 difMs);
    void ProcessEvent(SDL_Event* event);
    void DoIteration(i32 difMs);
    void PostInit();
    void PreDestroy();

    std::function<void()> updateFpsImageFunc = [this]()
    {
        string text = "FPS: " + to_string(fpsFrameCount);
        fpsText = c.graphics->MakeDrawnText(Layer_Chat, Color_Red, text.c_str());

        u32 w = 0, h = 0;
        c.graphics->GetScreenSize(&w, &h);
        fpsText->SetPosition(w - 100, 20);

        fpsFrameCount = 0;
    };

    std::function<void(const char*)> quitFunc = [this](const char* textUtf8)
    {
        if (string("?quit") == textUtf8)
            shouldExit = true;
        else
            c.chat->InternalMessage("Expected plain '?quit' command.");
    };
};

void SDLmanData::PreDestroy()
{
    // explicitly free resources here (since this module gets destroyed after graphics and others
    fpsTimer = nullptr;
    fpsText = nullptr;
    shipInfoText = nullptr;

    // in case we haven't disconnected yet
    c.connection->Disconnect();
}

void SDLmanData::PostInit()
{
    u32 w = 0, h = 0;
    c.graphics->GetScreenSize(&w, &h);

    SDL_Rect rect = {50, (i32)h - 150, 0, 0};
    SDL_SetTextInputRect(&rect);

    // load ship info text here
    shipInfoText = c.graphics->MakeDrawnText(Layer_Gauges, Color_Blue,
                                             "Q - Quit   1-8 - Change Ship   S - Spectator Mode");
    shipInfoText->SetPosition(w / 2 - shipInfoText->GetWidth() / 2, 10);
    shipInfoText->SetVisible(false);

    fpsTimer = c.timers->PeriodicTimer("fps_timer", 1000, updateFpsImageFunc);

    c.chat->AddInternalCommand("quit", quitFunc);
}

void SDLmanData::ProcessEvent(SDL_Event* event)
{
    switch (event->type)
    {
        case SDL_QUIT:
            shouldExit = true;

            // pop all other events (ignoring them)
            while (SDL_PollEvent(event))
                ;

            break;
        case SDL_TEXTINPUT:
        {
            bool escapeCommand = false;

            if (escPressedState)
            {
                string text = event->text.text;

                if (text.length() == 1)
                {
                    if (text[0] == 'q')
                    {
                        escapeCommand = true;
                        shouldExit = true;
                    }
                    else if (text[0] >= '1' && text[0] <= '8')
                    {
                        escapeCommand = true;
                        c.ships->RequestChangeShip((ShipType)(Ship_Warbird + text[0] - '1'));
                    }
                    else if (text[0] == 's')
                    {
                        escapeCommand = true;
                        c.ships->RequestChangeShip(Ship_Spec);
                    }
                }

                EscapeToggled();
            }

            if (!escapeCommand)
                c.chat->TextTyped(event->text.text);

            break;
        }
        case SDL_KEYUP:
        {
            if (event->key.repeat == 0)
            {
                switch (event->key.keysym.sym)
                {
                    case SDLK_RIGHT:
                        c.ships->RightPressed(false);
                        break;
                    case SDLK_LEFT:
                        c.ships->LeftPressed(false);
                        break;

                    case SDLK_DOWN:
                        c.ships->DownPressed(false);
                        break;
                    case SDLK_UP:
                        c.ships->UpPressed(false);
                        break;
                }
            }
            break;
        }
        case SDL_KEYDOWN:
        {
            if (event->key.keysym.sym == SDLK_BACKSPACE)
                c.chat->TextBackspace();

            if (event->key.repeat == 0)
            {
                switch (event->key.keysym.sym)
                {
                    case SDLK_RIGHT:
                        c.ships->RightPressed(true);
                        break;
                    case SDLK_LEFT:
                        c.ships->LeftPressed(true);
                        break;

                    case SDLK_DOWN:
                        c.ships->DownPressed(true);
                        break;
                    case SDLK_UP:
                        c.ships->UpPressed(true);
                        break;
                    case SDLK_ESCAPE:
                        EscapeToggled();
                        break;

                    case SDLK_RETURN:
                    case SDLK_RETURN2:
                    case SDLK_KP_ENTER:
                        c.chat->TextEnter();
                        break;
                    default:
                        break;
                }
            }

            break;
        }
    }
}

void SDLmanData::AdvanceState(i32 ms)
{
    c.net->ReceivePackets(ms);
    c.connection->UpdateConnectionStatus(ms);

    c.ships->AdvanceState(ms);
    c.timers->AdvanceTime(ms);

    c.net->SendPackets(ms);
}

void SDLmanData::EscapeToggled()
{
    escPressedState = !escPressedState;

    c.chat->SetEscPressedState(escPressedState);

    // temporary, until we get a real escapebox
    if (escPressedState)
        shipInfoText->SetVisible(true);
    else
        shipInfoText->SetVisible(false);
}

SDLman::SDLman(Client& c) : Module(c), data(make_shared<SDLmanData>(c))
{
    if (SDL_Init(SDL_INIT_EVERYTHING) == -1)
        c.log->FatalError("Failed to initialize SDL: %s", SDL_GetError());

    c.log->LogDrivel("SDL Initialized Correctly");
}

SDLman::~SDLman()
{
    SDL_Quit();
}

void SDLman::Exit()
{
    data->shouldExit = true;
}

void SDLmanData::DoIteration(i32 difMs)
{
    // events are processed 60 times a second
    // rendering might be slower

    SDL_Event event;

    while (SDL_PollEvent(&event))
        ProcessEvent(&event);

    AdvanceState(difMs);
}

void SDLman::MainLoop()
{
    data->PostInit();

    i32 targetFps = c.cfg->GetInt("game", "ticks_per_second", 60, [](i32 val)
                                  {
                                      return val > 0;
                                  });

    u32 msPerIteration = round(1000.0 / targetFps);

    u32 lastIterationMs = SDL_GetTicks();
    u32 difMs = 0;

    SDL_StartTextInput();

    while (!data->shouldExit)
    {
        while (difMs < msPerIteration)
        {
            u32 nowMs = SDL_GetTicks();
            difMs = nowMs - lastIterationMs;

            if (difMs < msPerIteration)
                SDL_Delay(1);
        }

        // at this point difMs >= msPerIteration

        if (difMs >= 2000)
            c.log->LogError("Slow Main Loop Iteration: difMs=%d\n", difMs);

        if (difMs >= 5000)
        {
            c.log->LogError("Main loop iteration could not keep up (difMs=%d). Exiting.", difMs);
            break;
        }

        // advance time in increments of msPerIteration
        i32 timeAdvanced = 0;
        while (difMs >= msPerIteration)
        {
            timeAdvanced += msPerIteration;
            difMs -= msPerIteration;
            data->DoIteration(msPerIteration);
            lastIterationMs += msPerIteration;
        }

        // render
        c.graphics->Render(timeAdvanced);
        ++data->fpsFrameCount;
    }

    SDL_StopTextInput();

    data->PreDestroy();
}
