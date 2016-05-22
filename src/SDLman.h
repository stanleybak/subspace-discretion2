/*
 * SDL Objects holder
 * Stanley Bak (May 2016)
 */

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "Client.h"

#ifndef SRC_SDLMAN_H_
#define SRC_SDLMAN_H_

class SDLman : public Module
{
    using Module::Module;  // use Module's constructor

   public:
    ~SDLman();

    void Init();
    void MainLoop();
    void Exit();

   private:
    void Render();
    SDL_Texture* LoadTexture(const char* path);

    bool shouldExit = false;
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
};

#endif  // SRC_SDLMAN_H_
