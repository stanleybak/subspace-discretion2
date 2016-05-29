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

enum TextColor
{
    Color_Grey,
    Color_Green,
    Color_Red,
    Color_Yellow,
    Color_Purple,
    Color_Orange,
    Color_Pink,
};

// an sdl_texture with automatic memory management
class ManagedTexture;

// an automatically-drawn object
class DrawnObject;

// automatically-drawn text
class DrawnText
{
   public:
    DrawnText(shared_ptr<DrawnObject> d) : d(d) {}

    void SetPosition(i32 x, i32 y);
    u32 GetHeight();
    u32 GetWidth();

   private:
    shared_ptr<DrawnObject> d;
};

struct Image
{
    Image(int numXFrames, int numYFrames, shared_ptr<ManagedTexture> tex, const char* filename);

    i32 numXFrames = 1;  // x frames in texture
    i32 numYFrames = 1;  // y frames in texture

    i32 frameWidth = -1;
    i32 frameHeight = -1;
    i32 halfFrameWidth = -1;
    i32 halfFrameHeight = -1;

    i32 textureWidth = -1;
    i32 textureHeight = -1;
    shared_ptr<ManagedTexture> texture;
    string filename;
};

struct Animation
{
    Animation(shared_ptr<Image> i, u32 animMs, u32 animFrameOffset, u32 animNumFrames);
    Animation(shared_ptr<Image> i, u32 animMs);

    shared_ptr<Image> image;
    u32 animMs;
    u32 animFrameOffset;
    u32 animNumFrames;
};

// automatically-drawn image
class DrawnImage
{
   public:
    DrawnImage(shared_ptr<GraphicsData> gd, Layer layer, u32 animMs, u32 animFrameOffset,
               u32 animNumFrames, shared_ptr<Image> i);
    ~DrawnImage();

    const char* GetName();
    u32 GetFrame();
    void SetFrame(u32 frameNum);
    void SetCenteredScreenPosition(i32 x, i32 y);
    void AdvanceAnimation(u32 mills);

   private:
    u32 lastFrameNum = 0;
    u32 animMsElapsed = 0;
    u32 animMs = 0;  // 0 = no animation
    u32 animFrameOffset = 0;
    u32 animNumFrames = 0;

    shared_ptr<GraphicsData> gd;
    shared_ptr<Image> image;
    shared_ptr<DrawnObject> drawnObj;
};

class Graphics : public Module
{
   public:
    Graphics(Client& c);
    ~Graphics();

    void Render(i32 difMs);
    void GetScreenSize(u32* w, u32* h);
    i32 GetFontHeight();

    // text will keep getting drawn until object is disposed (wrapPixels = 0 means no wrap)
    shared_ptr<DrawnText> MakeDrawnText(Layer layer, TextColor color, u32 wrapPixels,
                                        const char* utf8);

    // image will keep getting drawn until object is disposed
    shared_ptr<DrawnImage> MakeDrawnImage(Layer layer, shared_ptr<Image> image);
    shared_ptr<DrawnImage> MakeDrawnAnimation(Layer layer, shared_ptr<Animation> anim);

    // will loop once
    void MakeSingleDrawnAnimation(Layer layer, i32 x, i32 y, shared_ptr<Animation> anim);

    shared_ptr<Image> LoadImage(const char* filename);
    shared_ptr<Image> LoadImage(const char* filename, u32 framesW, u32 framesH);

   private:
    shared_ptr<GraphicsData> data;
};
