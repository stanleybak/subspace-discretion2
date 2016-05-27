
#include "SDLman.h"
#include "Graphics.h"
#include "Text.h"

struct SDLmanData
{
    SDLmanData(Client& client) : c(client) {}
    Client& c;
    i32 fpsMsRemaining = 1000;
    i32 fpsFrameCount = 0;
    i32 currentFps = 0;

    bool shouldExit = false;

    void AdvanceState(i32 difMs);
    void DoEvents(SDL_Event* event);
    void DoIteration(i32 difMs);
    void UpdateFps(i32 difMs);
};

static void DrawFps(Client* c, void* param)
{
    int* fpsPtr = (int*)param;

    c->text->DrawTextScreen(Text_Red, 100, 50, "FPS: %d", *fpsPtr);
}

void SDLmanData::UpdateFps(i32 difMs)
{
    ++fpsFrameCount;
    fpsMsRemaining -= difMs;

    if (fpsMsRemaining <= 0)
    {
        currentFps = fpsFrameCount;

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
        DoEvents(&event);

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

    c.graphics->AddDrawFunction(Layer_Chat, DrawFps, &data->currentFps);

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
}

void SDLmanData::DoEvents(SDL_Event* event)
{
    if (event->type == SDL_QUIT)
    {
        shouldExit = true;

        // pop all other events (ignoring them)
        while (SDL_PollEvent(event))
            ;
    }
    else if (event->type == SDL_KEYDOWN)
    {
        switch (event->key.keysym.sym)
        {
            /*case SDLK_RIGHT:
                    playerPos.x += movementFactor;
                    break;
            case SDLK_LEFT:
                    playerPos.x -= movementFactor;
                    break;
            // Remeber 0,0 in SDL is left-top. So when the user pressus down, the y need to increase
            case SDLK_DOWN:
                    playerPos.y += movementFactor;
                    break;
            case SDLK_UP:
                    playerPos.y -= movementFactor;
                    break;*/
            default:
                break;
        }
    }
}

void SDLmanData::AdvanceState(i32 difMs)
{
}
