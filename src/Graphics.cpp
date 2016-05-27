#include "Graphics.h"
#include "SDLman.h"
#include <map>

struct GraphicsData
{
    string graphicsFolder;

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;

    SDL_Texture* notFoundTexture = nullptr;

    multimap<Layer, pair<void (*)(Client*, void*), void*>> drawFuncs;

    SDL_Texture* LoadTexture(const char* path);
};

SDL_Texture* GraphicsData::LoadTexture(const char* path)
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

Image::~Image()
{
    if (texture)
    {
        SDL_DestroyTexture(texture);
        texture = nullptr;
    }
}

Graphics::Graphics(Client& c) : Module(c), data(make_shared<GraphicsData>())
{
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

    data->window =
        SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, 0);

    if (data->window == nullptr)
        c.log->FatalError("Failed to create window: %s", SDL_GetError());

    data->renderer =
        SDL_CreateRenderer(data->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (data->renderer == nullptr)
        c.log->FatalError("Failed to create renderer: %s", SDL_GetError());

    // Set size of renderer to the same as window
    SDL_RenderSetLogicalSize(data->renderer, w, h);

    // Set color of renderer to black
    SDL_SetRenderDrawColor(data->renderer, 0, 0, 0, 255);

    data->graphicsFolder = c.cfg->GetString("Graphics", "folder", "resources");

    string notFoundPath = data->graphicsFolder + "/" +
                          c.cfg->GetString("graphics", "not_found_image", "NotFound.png");
    data->notFoundTexture = data->LoadTexture(notFoundPath.c_str());

    if (data->notFoundTexture == nullptr)
        c.log->FatalError("Error Loading Not Found Image from '%s'", notFoundPath.c_str());
}

Graphics::~Graphics()
{
    SDL_DestroyTexture(data->notFoundTexture);
    data->notFoundTexture = nullptr;

    SDL_DestroyRenderer(data->renderer);
    data->renderer = nullptr;

    SDL_DestroyWindow(data->window);
    data->window = nullptr;
}

shared_ptr<Image> Graphics::LoadAnimation(const char* name, i32 w, i32 h, i32 frameOffset,
                                          i32 numFrames, i32 animMs)
{
    shared_ptr<Image> rv = make_shared<Image>();
    string path = "graphics/" + string(name);

    rv->texture = data->LoadTexture(path.c_str());

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
    // Clear the window
    SDL_RenderClear(data->renderer);

    // SDL_Rect pos = {100, 100, 200, 200};
    // SDL_RenderCopy(data->renderer, data->notFoundTexture, NULL, &pos);

    for (auto it : data->drawFuncs)
    {
        it.second.first(&c, it.second.second);
    }

    // Render the changes
    SDL_RenderPresent(data->renderer);
}

void Graphics::AddDrawFunction(Layer l, void (*drawFunc)(Client* c, void* param), void* param)
{
    data->drawFuncs.insert(make_pair(l, make_pair(drawFunc, param)));
}

void Graphics::RemoveDrawFunction(Layer l, void (*drawFunc)(Client* c, void* param), void* param)
{
    auto range = data->drawFuncs.equal_range(l);

    if (range.first == range.second)
        c.log->LogError("Graphics::RemoveDrawFunction found no entries at layer %d", l);
    else
    {
        bool found = false;

        for (auto it = range.first; it != range.second; ++it)
        {
            if (it->second.first == drawFunc && it->second.second == param)
            {
                found = true;
                data->drawFuncs.erase(it);
                break;
            }
        }

        if (!found)
            c.log->LogError("Graphics::RemoveDrawFunction did not find drawFunc at layer %d", l);
    }
}

SDL_Texture* Graphics::SurfaceToTexture(SDL_Surface* s)
{
    SDL_Texture* tex = SDL_CreateTextureFromSurface(data->renderer, s);

    if (tex == nullptr)
        c.log->FatalError("Graphics::SurfaceToTexture failed.");

    SDL_FreeSurface(s);

    return tex;
}

SDL_Renderer* Graphics::getRenderer()
{
    return data->renderer;
}
