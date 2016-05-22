
#include "SDLman.h"
#include "Graphics.h"

SDLman::SDLman(Client& c) : Module(c)
{
    if (SDL_Init(SDL_INIT_EVERYTHING) == -1)
        c.log->FatalError("Failed to initialize SDL: %s", SDL_GetError());

    int w = c.cfg->GetInt("video", "width", 800, [](i32 val)
                          {
                              return val >= 100 && val <= 10000;
                          });
    int h = c.cfg->GetInt("video", "height", 600, [](i32 val)
                          {
                              return val >= 100 && val <= 10000;
                          });

    c.log->LogDrivel("Creating window: %ix%i", w, h);
    const char* title = c.cfg->GetString("video", "title", "Discretion2");

    window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, 0);

    if (window == nullptr)
        c.log->FatalError("Failed to create window: %s", SDL_GetError());

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (renderer == nullptr)
        c.log->FatalError("Failed to create renderer: %s", SDL_GetError());

    // Set size of renderer to the same as window
    SDL_RenderSetLogicalSize(renderer, w, h);

    // Set color of renderer to black
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

    c.log->LogDrivel("SDL Initialized Correctly");
}

SDLman::~SDLman()
{
    SDL_DestroyRenderer(renderer);
    renderer = nullptr;

    SDL_DestroyWindow(window);
    window = nullptr;

    SDL_Quit();
}

SDL_Texture* SDLman::LoadTexture(const char* path)
{
    SDL_Texture* rv = nullptr;
    SDL_Surface* surface = IMG_Load(path);

    if (surface)
    {
        // SDL_Surface is just the raw pixels
        // Convert it to a hardware-optimized texture so we can render it
        rv = SDL_CreateTextureFromSurface(renderer, surface);

        // Don't need the original texture, release the memory
        SDL_FreeSurface(surface);
    }

    return rv;
}

void SDLman::Exit()
{
    shouldExit = true;
}

void SDLman::DoIteration(i32 difMs)
{
    SDL_Event event;

    while (SDL_PollEvent(&event))
        DoEvents(&event);

    AdvanceState(difMs);
    Render(difMs);
}

void SDLman::MainLoop()
{
    i32 targetFps = c.cfg->GetInt("video", "targetfps", 60, [](i32 val)
                                  {
                                      return val > 0;
                                  });

    u32 msPerIteration = round(1000.0 / targetFps);
    u32 lastIterationMs = SDL_GetTicks();
    u32 nowMs = 0, difMs = 0;

    while (!shouldExit)
    {
        do
        {
            nowMs = SDL_GetTicks();
            difMs = nowMs - lastIterationMs;

            if (difMs < msPerIteration)
                SDL_Delay(1);
        } while (difMs < msPerIteration);

        difMs = nowMs - lastIterationMs;
        DoIteration(difMs);

        lastIterationMs = nowMs;
    }
}

void SDLman::DoEvents(SDL_Event* event)
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

void SDLman::AdvanceState(i32 difMs)
{
}

void SDLman::Render(i32 difMs)
{
    // Clear the window
    SDL_RenderClear(renderer);

    c.graphics->Render(difMs);

    // Render the changes above
    SDL_RenderPresent(renderer);
}
