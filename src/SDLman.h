/*
 * SDL Objects holder
 * Stanley Bak (May 2016)
 */
#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "Module.h"

class SDLman : public Module
{
   public:
    SDLman(Client& c);
    ~SDLman();

    void MainLoop();
    void Exit();
    SDL_Texture* LoadTexture(const char* path);

   private:
    void Render(i32 difMs);
    void AdvanceState(i32 difMs);
    void DoEvents(SDL_Event* event);
    void DoIteration(i32 difMs);

    bool shouldExit = false;
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
};
