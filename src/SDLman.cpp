
#include "SDLman.h"

static SDL_Texture* texture;

void SDLman::Init()
{
    if (SDL_Init(SDL_INIT_EVERYTHING) == -1)
        c.log.FatalError("Failed to initialize SDL: %s", SDL_GetError());

    int w = c.cfg.GetInt("video", "width", 800);
    int h = c.cfg.GetInt("video", "height", 600);

    SDL_Rect windowRect = {0, 0, w, h};

    c.log.LogDrivel("Creating window: %i %i %i %i", windowRect.x, windowRect.y, windowRect.w, windowRect.h);
    printf("after created window...\n");

    window = SDL_CreateWindow("Server", windowRect.x, windowRect.y, windowRect.w, windowRect.h, 0);

    if (window == nullptr)
        c.log.FatalError("Failed to create window: %s", SDL_GetError());

    renderer = SDL_CreateRenderer(window, -1, 0);

    if (renderer == nullptr)
        c.log.FatalError("Failed to create renderer: %s", SDL_GetError());

    // Set size of renderer to the same as window
    SDL_RenderSetLogicalSize(renderer, windowRect.w, windowRect.h);

    // Set color of renderer to red
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);

    c.log.LogDrivel("SDL Initialized Correctly");

    texture = LoadTexture("graphics/NotFound.png");
}

SDL_Texture* SDLman::LoadTexture(const char* path)
{
    // Load image as SDL_Surface
    SDL_Surface* surface = IMG_Load(path);

    // SDL_Surface is just the raw pixels
    // Convert it to a hardware-optimzed texture so we can render it
    texture = SDL_CreateTextureFromSurface(renderer, surface);

    // Don't need the orignal texture, release the memory
    SDL_FreeSurface(surface);

    return texture;
}

void SDLman::Exit()
{
    shouldExit = true;
}

void SDLman::MainLoop()
{
    while (!shouldExit)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
                shouldExit = true;
            else if (event.type == SDL_KEYDOWN)
            {
                switch (event.key.keysym.sym)
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

        Render();

        // Add a 16msec delay to make our game run at ~60 fps
        SDL_Delay(16);
    }
}

void SDLman::Render()
{
    // Clear the window and make it all red
    SDL_RenderClear(renderer);

    SDL_Rect backgroundPos = {100, 100, 200, 200};

    SDL_RenderCopy(renderer, texture, NULL, &backgroundPos);

    // Render the changes above
    SDL_RenderPresent(renderer);
}

SDLman::~SDLman()
{
    /*
     *
     TODO:
        add proper de -
            initialization !!also add main loop(which loops while !shouldExit !)
        */
    SDL_Quit();
}
