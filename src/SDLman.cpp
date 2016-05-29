
#include "SDLman.h"
#include "Graphics.h"
#include "Ships.h"
#include "Chat.h"

struct SDLmanData
{
    SDLmanData(Client& client) : c(client) {}
    Client& c;
    i32 fpsMsRemaining = 1000;
    i32 fpsFrameCount = 0;
    shared_ptr<DrawnText> fpsText;

    bool shouldExit = false;

    void AdvanceState(i32 difMs);
    void ProcessEvent(SDL_Event* event);
    void DoIteration(i32 difMs);
    void UpdateFps(i32 difMs);
};

void SDLmanData::UpdateFps(i32 difMs)
{
    ++fpsFrameCount;
    fpsMsRemaining -= difMs;

    if (fpsMsRemaining <= 0)
    {
        string text = "FPS: " + to_string(fpsFrameCount);
        fpsText = c.graphics->MakeDrawnText(Layer_Chat, Color_Red, text.c_str());
        fpsText->SetPosition(150, 20);

        fpsMsRemaining = 1000;
        fpsFrameCount = 0;
    }
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
    SDL_Event event;

    while (SDL_PollEvent(&event))
        ProcessEvent(&event);

    AdvanceState(difMs);

    UpdateFps(difMs);

    c.graphics->Render(difMs);
}

void SDLman::MainLoop()
{
    i32 targetFps = c.cfg->GetInt("video", "target_fps", 60, [](i32 val)
                                  {
                                      return val > 0;
                                  });

    u32 msPerIteration = round(1000.0 / targetFps);
    u32 lastIterationMs = SDL_GetTicks();
    u32 nowMs = 0, difMs = 0;

    SDL_StartTextInput();

    u32 w = 0, h = 0;
    c.graphics->GetScreenSize(&w, &h);

    SDL_Rect rect = {50, (i32)h - 150, 0, 0};
    SDL_SetTextInputRect(&rect);

    while (!data->shouldExit)
    {
        do
        {
            nowMs = SDL_GetTicks();
            difMs = nowMs - lastIterationMs;

            if (difMs < msPerIteration)
                SDL_Delay(1);
        } while (difMs < msPerIteration);

        difMs = nowMs - lastIterationMs;
        data->DoIteration(difMs);

        lastIterationMs = nowMs;
    }

    SDL_StopTextInput();

    // explicitly free it here
    if (data->fpsText)
        data->fpsText = nullptr;
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
            c.chat->TextTyped(event->text.text);

            break;
        case SDL_KEYDOWN:

            if (event->key.keysym.sym == SDLK_BACKSPACE)
                c.chat->TextBackspace();

            if (event->key.keysym.sym == SDLK_RETURN || event->key.keysym.sym == SDLK_KP_ENTER ||
                event->key.keysym.sym == SDLK_RETURN2)
                c.chat->TextEnter();

            if (event->key.repeat == 0)
            {
                switch (event->key.keysym.sym)
                {
                    /*case SDLK_RIGHT:
                                    playerPos.x += movementFactor;
                                    break;
                    case SDLK_LEFT:
                                    playerPos.x -= movementFactor;
                                    break;
                    // Remeber 0,0 in SDL is left-top. So when the user pressus down, the y need to
                    increase*/
                    case SDLK_DOWN:
                        printf("down arrow\n");
                        break;
                    case SDLK_UP:
                        printf("up arrow\n");
                        break;
                    default:
                        break;
                }
            }

            break;
    }
}

void SDLmanData::AdvanceState(i32 difMs)
{
    c.ships->AdvanceState(difMs);
}
