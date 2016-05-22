/*
 * Graphics.h
 *
 *  Created on: May 22, 2016
 *      Author: stan
 */
#pragma once

#include "Module.h"
#include <SDL2/SDL_image.h>

// obtain these through Graphics->LoadImage()
struct Image
{
    ~Image();

    SDL_Texture* texture = nullptr;
    i32 framesW = 1;
    i32 framesH = 1;
    i32 numFrames = 1;
    i32 frameOffset = 0;
    i32 animMs = 1000;  // animation time, in milliseconds
    i32 curAnimTime = 0;
};

class Graphics : public Module
{
   public:
    Graphics(Client& c);
    ~Graphics();

    void Render(i32 difMs);
    shared_ptr<Image> LoadAnimation(const char* name, i32 w, i32 h, i32 frameOffset, i32 numFrames, i32 animMs);
    shared_ptr<Image> LoadImage(const char* name);

   private:
    SDL_Texture* notFoundTexture = nullptr;
};
