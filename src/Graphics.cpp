#include "Graphics.h"
#include "SDLman.h"

static i32 fpsMsRemaining = 1000;
static i32 fpsFrameCount = 0;

Image::~Image()
{
    if (texture)
    {
        SDL_DestroyTexture(texture);
        texture = nullptr;
    }
}

Graphics::Graphics(Client& c) : Module(c)
{
    notFoundTexture = c.sdl->LoadTexture("graphics/NotFound.png");
}

Graphics::~Graphics()
{
    SDL_DestroyTexture(notFoundTexture);
    notFoundTexture = nullptr;
}

shared_ptr<Image> Graphics::LoadAnimation(const char* name, i32 w, i32 h, i32 frameOffset, i32 numFrames, i32 animMs)
{
    shared_ptr<Image> rv = make_shared<Image>();
    string path = "graphics/" + string(name);

    rv->texture = c.sdl->LoadTexture(path.c_str());

    if (rv->texture)
    {
        rv->framesW = w;
        rv->framesH = h;
        rv->numFrames = numFrames;
        rv->frameOffset = frameOffset;
        rv->animMs = animMs;
    }
    else
        c.log->LogError("Image not found '%s'", path.c_str());

    return rv;
}

shared_ptr<Image> Graphics::LoadImage(const char* name)
{
    return LoadAnimation(name, 1, 1, 0, 1, 1000);
}

void Graphics::Render(i32 difMs)
{
    /*
    SDL_Rect backgroundPos = {100, 100, 200, 200};

SDL_RenderCopy(renderer, texture, NULL, &backgroundPos);
     */

    ++fpsFrameCount;
    fpsMsRemaining -= difMs;

    if (fpsMsRemaining <= 0)
    {
        printf(". fps = %i\n", fpsFrameCount);

        fpsMsRemaining = 1000;
        fpsFrameCount = 0;
    }
}
