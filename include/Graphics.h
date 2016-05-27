/*
 * Graphics.h
 *
 *  Created on: May 22, 2016
 *      Author: stan
 */
#pragma once

#include "Module.h"
#include <SDL2/SDL_image.h>

struct GraphicsData;

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

// The layer enum, note you can define your own layers between these ones if you want
enum Layer
{
    Layer_BelowAll = 50,
    Layer_Background = 100,
    Layer_AfterBackground = 150,
    Layer_Tiles = 200,
    Layer_AfterTiles = 250,
    Layer_Weapons = 300,
    Layer_AfterWeapons = 350,
    Layer_Ships = 400,
    Layer_AfterShips = 450,
    Layer_Gauges = 500,
    Layer_AfterGauges = 550,
    Layer_Chat = 600,
    Layer_AfterChat = 650,
    Layer_TopMost = 700
};

class Graphics : public Module
{
   public:
    Graphics(Client& c);
    ~Graphics();

    void Render(i32 difMs);
    shared_ptr<Image> LoadAnimation(const char* name, i32 w, i32 h, i32 frameOffset, i32 numFrames,
                                    i32 animMs);
    shared_ptr<Image> LoadImage(const char* name);

    SDL_Texture* SurfaceToTexture(SDL_Surface* surf);
    SDL_Renderer* getRenderer();

    // draw a function at a particular layer
    void AddDrawFunction(Layer l, void (*drawFunc)(Client* c, void* param), void* param);
    void RemoveDrawFunction(Layer l, void (*drawFunc)(Client* c, void* param), void* param);

   private:
    shared_ptr<GraphicsData> data;
};
