/*
 * SDL Objects holder
 * Stanley Bak (May 2016)
 */
#pragma once

#include <SDL2/SDL.h>
#include "Module.h"

class SDLmanData;

class SDLman : public Module
{
   public:
    SDLman(Client& c);
    ~SDLman();

    void MainLoop();
    void Exit();

   private:
    shared_ptr<SDLmanData> data;
};
