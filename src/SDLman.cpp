
#include "SDLman.h"

SDLman::SDLman(Client& client) : Module(client)
{
    if (SDL_Init(SDL_INIT_EVERYTHING) == -1)
        c.log.FatalError("Failed to initialize SDL: %s", SDL_GetError());

    SDL_Rect windowRect = {900, 300, 300, 400};
    window = SDL_CreateWindow("Server", windowRect.x, windowRect.y,
                              windowRect.w, windowRect.h, 0);

    if (window == nullptr)
        c.log.FatalError("Failed to create window: %s", SDL_GetError());

    renderer = SDL_CreateRenderer(window, -1, 0);

    if (renderer == nullptr)
        c.log.FatalError("Failed to create renderer: %s", SDL_GetError());

    // Set size of renderer to the same as window
    SDL_RenderSetLogicalSize(renderer, windowRect.w, windowRect.h);

    // Set color of renderer to red
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
}

void SDLman::Exit()
{
    shouldExit = true;
}

SDLman::~SDLman()
{
    /*
     *
     TODO:
        add proper de -
            initialization !!also add main loop(which loops while !shouldExit !)
        */
}
