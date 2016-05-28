#include "Graphics.h"
#include "SDLman.h"
#include "SDL2/SDL_ttf.h"
#include <map>
#include <unordered_set>

class ManagedTexture
{
   public:
    ManagedTexture(SDL_Texture* t) : rawTexture(t) {}
    ~ManagedTexture()
    {
        SDL_DestroyTexture(rawTexture);
        rawTexture = nullptr;
    }

    SDL_Texture* rawTexture = nullptr;
};

// super class for all drawn objects
class DrawnObject
{
   public:
    DrawnObject(shared_ptr<GraphicsData> gd, Layer layer, shared_ptr<ManagedTexture> texture,
                const char* name);
    ~DrawnObject();

    void Draw(SDL_Renderer* renderer);
    string ToString();

    Layer layer;
    shared_ptr<ManagedTexture> texture;
    SDL_Rect src = {0, 0, -1, 0};  // src.w = -1 means use nullptr
    SDL_Rect dest = {0, 0, 0, 0};
    const char* name;

   private:
    shared_ptr<GraphicsData> gd;
};

struct GraphicsData
{
    GraphicsData(Client& client) : c(client) {}
    Client& c;
    string graphicsFolder;

    TTF_Font* font = nullptr;

    i32 windowW = 1, windowH = 1;
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;

    shared_ptr<ManagedTexture> notFoundTexture = nullptr;
    SDL_Texture* SurfaceToTexture(SDL_Surface* s);
    shared_ptr<ManagedTexture> LoadTexture(const char* filename);

    u32 nowMs = 0;  // for tracking single animation expiration time
    multimap<Layer, DrawnObject*> drawnObjs;
    multimap<u32, shared_ptr<DrawnImage>> singleAnimations;  // expirationMs -> DrawnImage
    unordered_set<DrawnImage*> animations;
};

shared_ptr<ManagedTexture> GraphicsData::LoadTexture(const char* filename)
{
    string path = graphicsFolder + "/" + filename;

    shared_ptr<ManagedTexture> rv = nullptr;
    SDL_Surface* surface = nullptr;

    if (strchr(filename, '.') != nullptr)
        surface = IMG_Load(path.c_str());
    else
    {
        // try a bunch of image extensions
        const char* exts[] = {".png", ".bmp", ".jpg", ".gif", ".tif", ".jpeg", ".tiff", ".pnm",
                              ".ppm", ".pgm", ".pbm", ".xpm", ".lbm", ".pcx",  ".tga"};

        for (const char* ext : exts)
        {
            string tryPath = path + ext;

            surface = IMG_Load(tryPath.c_str());

            if (surface)
                break;
        }

        if (!surface)
            path += ".png";  // for the error message
    }

    if (surface)
    {
        c.log->LogDrivel("Loaded image from '%s'", filename);

        if (SDL_SetColorKey(surface, SDL_TRUE, SDL_MapRGB(surface->format, 0, 0, 0)))
            c.log->LogError("SDL_SetColorKey failed on '%s': %s", filename, SDL_GetError());

        SDL_Texture* rawTexture = SurfaceToTexture(surface);
        rv = make_shared<ManagedTexture>(rawTexture);
    }
    else
    {
        c.log->LogError("Error loading image at path '%s'", path.c_str());
        rv = notFoundTexture;
    }

    return rv;
}

SDL_Texture* GraphicsData::SurfaceToTexture(SDL_Surface* s)
{
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, s);

    if (tex == nullptr)
        c.log->FatalError(
            "GraphicsData::SurfaceToTexture - SDL_CreateTextureFromSurface() failed.");

    SDL_FreeSurface(s);

    return tex;
}

void DrawnObject::Draw(SDL_Renderer* r)
{
    if (src.w == -1)
        SDL_RenderCopy(r, texture->rawTexture, nullptr, &dest);
    else
        SDL_RenderCopy(r, texture->rawTexture, &src, &dest);
}

DrawnObject::DrawnObject(shared_ptr<GraphicsData> gd, Layer layer,
                         shared_ptr<ManagedTexture> texture, const char* name)
    : layer(layer), texture(texture), name(name), gd(gd)
{
    // add to render list
    gd->drawnObjs.insert(make_pair(layer, this));
}

string DrawnObject::ToString()
{
    char buf[256];

    snprintf(buf, sizeof(buf), "%s at layer %d, src=(%d, %d, %d, %d), dest=(%d, %d, %d, %d)", name,
             layer, src.x, src.y, src.w, src.h, dest.x, dest.y, dest.w, dest.h);

    return buf;
}

DrawnObject::~DrawnObject()
{
    auto range = gd->drawnObjs.equal_range(layer);
    bool found = false;

    for (auto it = range.first; it != range.second; ++it)
    {
        if (it->second == this)
        {
            found = true;
            gd->drawnObjs.erase(it);
            break;
        }
    }

    if (!found)
        gd->c.log->LogError("Drawable::~Drawable Unregistering drawable not found in draw list.");
}

DrawnImage::DrawnImage(shared_ptr<GraphicsData> gd, Layer layer, u32 animMs, u32 animFrameOffset,
                       u32 animNumFrames, shared_ptr<Image> i)
    : animMs(animMs),
      animFrameOffset(animFrameOffset),
      animNumFrames(animNumFrames),
      gd(gd),
      image(i)
{
    drawnObj = make_shared<DrawnObject>(gd, layer, image->texture, image->filename.c_str());

    drawnObj->dest.w = drawnObj->src.w = image->frameWidth;
    drawnObj->dest.h = drawnObj->src.h = image->frameHeight;

    // if it's an animation, add it to the animation list
    if (animMs > 0)
        gd->animations.insert(this);
}

DrawnImage::~DrawnImage()
{
    if (animMs > 0)
        gd->animations.erase(this);
}

void DrawnImage::AdvanceAnimation(u32 mills)
{
    animMsElapsed += mills;

    while (animMsElapsed >= animMs)
        animMsElapsed -= animMs;

    u32 frameInAnimation = animMsElapsed * animNumFrames / animMs;

    SetFrame(animFrameOffset + frameInAnimation);
}

void DrawnImage::SetFrame(u32 frameNum)
{
    if (lastFrameNum != frameNum)
    {
        lastFrameNum = frameNum;

        i32 x = frameNum % image->numXFrames;  // to get col #, we modulo the width
        i32 y = frameNum / image->numXFrames;  // to get the row #, we divide by width

        if (y >= image->numYFrames)
        {
            y = 0;
            gd->c.log->LogError("Frame (%i) was out of bounds for %dx%d image '%s'", frameNum,
                                image->numXFrames, image->numYFrames, image->filename.c_str());
        }

        drawnObj->src.x = x * image->frameWidth;
        drawnObj->src.y = y * image->frameHeight;
    }
}

u32 DrawnImage::GetFrame()
{
    return lastFrameNum;
}

void DrawnImage::SetCenteredScreenPosition(i32 x, i32 y)
{
    drawnObj->dest.x = x - image->halfFrameHeight;
    drawnObj->dest.y = y - image->halfFrameWidth;
}

Image::Image(int numXFrames, int numYFrames, shared_ptr<ManagedTexture> tex, const char* filename)
    : numXFrames(numXFrames), numYFrames(numYFrames), texture(tex), filename(filename)
{
    SDL_QueryTexture(tex->rawTexture, NULL, NULL, &textureWidth, &textureHeight);

    frameWidth = textureWidth / numXFrames;
    frameHeight = textureHeight / numYFrames;
    halfFrameWidth = frameWidth / 2;
    halfFrameHeight = frameHeight / 2;
}

Animation::Animation(shared_ptr<Image> i, u32 animMs, u32 animFrameOffset, u32 animNumFrames)
    : image(i), animMs(animMs), animFrameOffset(animFrameOffset), animNumFrames(animNumFrames)
{
}

Animation::Animation(shared_ptr<Image> i, u32 animMs)
    : image(i), animMs(animMs), animFrameOffset(0), animNumFrames(i->numXFrames * i->numYFrames)
{
}

Graphics::Graphics(Client& c) : Module(c), data(make_shared<GraphicsData>(c))
{
    data->windowW = c.cfg->GetInt("video", "width", 800, [](i32 val)
                                  {
                                      return val >= 100 && val <= 10000;
                                  });
    data->windowH = c.cfg->GetInt("video", "height", 600, [](i32 val)
                                  {
                                      return val >= 100 && val <= 10000;
                                  });

    c.log->LogDrivel("Creating window: %ix%i", data->windowW, data->windowH);
    const char* title = c.cfg->GetString("video", "title", "Discretion2");

    data->window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                    data->windowW, data->windowH, 0);

    if (data->window == nullptr)
        c.log->FatalError("Failed to create window: %s", SDL_GetError());

    data->renderer =
        SDL_CreateRenderer(data->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (data->renderer == nullptr)
        c.log->FatalError("Failed to create renderer: %s", SDL_GetError());

    // Set size of renderer to the same as window
    SDL_RenderSetLogicalSize(data->renderer, data->windowW, data->windowH);

    // Set color of renderer to black
    SDL_SetRenderDrawColor(data->renderer, 0, 0, 0, 255);

    data->graphicsFolder = c.cfg->GetString("Graphics", "folder", "resources");

    const char* notFoundFilename = c.cfg->GetString("graphics", "not_found_image", "NotFound.png");
    data->notFoundTexture = data->LoadTexture(notFoundFilename);

    if (data->notFoundTexture == nullptr)
        c.log->FatalError("Error Loading Not Found Image from '%s'", notFoundFilename);

    if (TTF_Init() < 0)
        c.log->FatalError("Failed to initialize SDL_ttf: %s", TTF_GetError());

    const SDL_version* linkedVer = TTF_Linked_Version();
    SDL_version compVer;
    SDL_TTF_VERSION(&compVer);
    c.log->LogDrivel("SDL_TTF linked version was %d.%d.%d, compiled version was %d.%d.%d",
                     linkedVer->major, linkedVer->minor, linkedVer->patch, compVer.major,
                     compVer.minor, compVer.patch);

    // load font
    string fontPath =
        data->graphicsFolder + "/" + c.cfg->GetString("text", "font_file", "SourceHanSans.otf");

    i32 size = c.cfg->GetInt("text", "font_size", 12, [](i32 val)
                             {
                                 return val >= 4 && val <= 128;
                             });

    data->font = TTF_OpenFont(fontPath.c_str(), size);

    if (data->font == nullptr)
    {
        c.log->FatalError("Loading font from '%s' with size %d failed: %s", fontPath.c_str(), size,
                          TTF_GetError());
    }
    else
        c.log->LogDrivel("Loaded font from '%s'", fontPath.c_str());
}

Graphics::~Graphics()
{
    TTF_CloseFont(data->font);
    TTF_Quit();

    data->notFoundTexture = nullptr;

    SDL_DestroyRenderer(data->renderer);
    data->renderer = nullptr;

    SDL_DestroyWindow(data->window);
    data->window = nullptr;
}

void Graphics::Render(i32 difMs)
{
    data->nowMs += difMs;

    // possibly delete single animations
    for (auto it = data->singleAnimations.cbegin(); it != data->singleAnimations.cend();)
    {
        if (data->nowMs >= it->first)
            data->singleAnimations.erase(it++);
        else
            ++it;
    }

    // advance all animations
    for (auto it : data->animations)
        it->AdvanceAnimation(difMs);

    // Clear the window
    SDL_RenderClear(data->renderer);

    for (auto it : data->drawnObjs)
        it.second->Draw(data->renderer);

    // Render the changes
    SDL_RenderPresent(data->renderer);
}

void Graphics::GetScreenSize(i32* w, i32* h)
{
    *w = data->windowW;
    *h = data->windowH;
}

shared_ptr<DrawnImage> Graphics::MakeDrawnImage(Layer layer, shared_ptr<Image> image)
{
    shared_ptr<DrawnImage> rv = make_shared<DrawnImage>(data, layer, 0, 0, 0, image);

    return rv;
}

shared_ptr<DrawnImage> Graphics::MakeDrawnAnimation(Layer layer, shared_ptr<Animation> anim)
{
    shared_ptr<DrawnImage> rv = make_shared<DrawnImage>(
        data, layer, anim->animMs, anim->animFrameOffset, anim->animNumFrames, anim->image);

    return rv;
}

void Graphics::MakeSingleDrawnAnimation(Layer layer, i32 x, i32 y, shared_ptr<Animation> anim)
{
    shared_ptr<DrawnImage> rv = MakeDrawnAnimation(layer, anim);
    rv->SetCenteredScreenPosition(x, y);

    data->singleAnimations.insert(make_pair(data->nowMs + anim->animMs, rv));
}

shared_ptr<DrawnText> Graphics::MakeDrawnText(Layer layer, TextColor color, i32 x, i32 y,
                                              const char* format, ...)
{
    char buf[256];
    va_list args;
    va_start(args, format);

    vsnprintf(buf, sizeof(buf), format, args);

    va_end(args);

    SDL_Color textColor = {0, 0, 0, 255};

    switch (color)
    {
        case Text_Red:
            textColor = {255, 0, 0, 255};
            break;
        case Text_Blue:
            textColor = {0, 0, 255, 255};
            break;
        default:
            c.log->LogError("Graphics::MakeText unknown color: %i", color);
    }

    SDL_Color backgroundColor = {0, 0, 0, 255};  // black
    SDL_Surface* surf = TTF_RenderUTF8_Shaded(data->font, buf, textColor, backgroundColor);
    SDL_Texture* tex = data->SurfaceToTexture(surf);

    shared_ptr<ManagedTexture> mt = make_shared<ManagedTexture>(tex);
    shared_ptr<DrawnObject> drawn = make_shared<DrawnObject>(data, layer, mt, "text");

    drawn->dest.x = x;
    drawn->dest.y = y;
    SDL_QueryTexture(tex, NULL, NULL, &drawn->dest.w, &drawn->dest.h);

    return make_shared<DrawnText>(drawn);
}

shared_ptr<Image> Graphics::LoadImage(const char* filename)
{
    return LoadImage(filename, 1, 1);
}

shared_ptr<Image> Graphics::LoadImage(const char* filename, u32 w, u32 h)
{
    shared_ptr<ManagedTexture> mt = data->LoadTexture(filename);
    shared_ptr<Image> rv = make_shared<Image>(w, h, mt, filename);

    return rv;
}
