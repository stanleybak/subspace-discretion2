#include "Graphics.h"
#include "SDLman.h"
#include "SDL2/SDL_ttf.h"
#include "utf8.h"
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

    u32 windowW = 1, windowH = 1;
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;

    shared_ptr<ManagedTexture> notFoundTexture = nullptr;

    void LoadIcon();
    SDL_Surface* LoadSurface(const string* path);
    SDL_Texture* SurfaceToTexture(SDL_Surface* s);
    shared_ptr<ManagedTexture> LoadTexture(const char* filename);

    u32 nowMs = 0;  // for tracking single animation expiration time
    multimap<Layer, DrawnObject*> drawnObjs;
    multimap<u32, shared_ptr<DrawnImage>> singleAnimations;  // expirationMs -> DrawnImage
    unordered_set<DrawnImage*> animations;
};

// may return null
SDL_Surface* GraphicsData::LoadSurface(const string* path)
{
    SDL_Surface* surface = nullptr;

    if (strchr(path->c_str(), '.') != nullptr)
        surface = IMG_Load(path->c_str());
    else
    {
        // try a bunch of image extensions
        const char* exts[] = {".png", ".bmp", ".jpg", ".gif", ".tif", ".jpeg", ".tiff", ".pnm",
                              ".ppm", ".pgm", ".pbm", ".xpm", ".lbm", ".pcx",  ".tga"};

        for (const char* ext : exts)
        {
            string tryPath = *path + ext;

            surface = IMG_Load(tryPath.c_str());

            if (surface)
                break;
        }
    }

    return surface;
}

shared_ptr<ManagedTexture> GraphicsData::LoadTexture(const char* filename)
{
    string path = graphicsFolder + "/" + filename;

    SDL_Surface* surface = LoadSurface(&path);
    shared_ptr<ManagedTexture> rv = nullptr;

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
        if (strchr(path.c_str(), '.') == nullptr)
            path += ".png";  // for the error message

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

void DrawnText::SetPosition(i32 x, i32 y)
{
    d->dest.x = x;
    d->dest.y = y;
}

u32 DrawnText::GetHeight()
{
    return d->dest.h;
}

u32 DrawnText::GetWidth()
{
    return d->dest.w;
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
    data->graphicsFolder = c.cfg->GetString("Graphics", "folder", "resources");

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

    data->LoadIcon();

    data->renderer =
        SDL_CreateRenderer(data->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (data->renderer == nullptr)
        c.log->FatalError("Failed to create renderer: %s", SDL_GetError());

    // Set size of renderer to the same as window
    SDL_RenderSetLogicalSize(data->renderer, data->windowW, data->windowH);

    // Set color of renderer to black
    SDL_SetRenderDrawColor(data->renderer, 0, 0, 0, 255);

    const char* notFoundFilename =
        c.cfg->GetString("graphics", "not_found_image_name", "not_found");
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
        data->graphicsFolder + "/" + c.cfg->GetString("text", "font_file", "source_han_sans.otf");

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

void GraphicsData::LoadIcon()
{
    // load icon
    string iconName =
        graphicsFolder + "/" + c.cfg->GetString("graphics", "icon_image_name", "icon");

    SDL_Surface* icon = LoadSurface(&iconName);

    if (icon)
    {
        SDL_SetWindowIcon(window, icon);
        SDL_FreeSurface(icon);
        icon = nullptr;

        c.log->LogDrivel("Set window icon to image '%s'", iconName.c_str());
    }
    else
        c.log->LogError("Error loading icon image from '%s'", iconName.c_str());
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

void Graphics::GetScreenSize(u32* w, u32* h)
{
    *w = data->windowW;
    *h = data->windowH;
}

i32 Graphics::GetFontHeight()
{
    return TTF_FontHeight(data->font) + 3;  // TTF_FontHeight = 17, TTF_FontLineSkip=22
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

shared_ptr<DrawnText> Graphics::MakeDrawnText(Layer layer, TextColor color, u32 wrapPixels,
                                              const char* utf8)
{
    SDL_Color textColor = {64, 64, 64, 255};

    pair<TextColor, SDL_Color> colors[] = {
        {Color_Grey, {0xb5, 0xb5, 0xb5, 255}},
        {Color_Green, {0x73, 0xff, 0x63, 255}},
        {Color_Red, {0xde, 0x31, 0x08, 255}},
        {Color_Yellow, {0xff, 0xbd, 0x29, 255}},
        {Color_Purple, {0xad, 0x6b, 0xf7, 255}},
        {Color_Orange, {0xef, 0x6b, 0x18, 255}},
        {Color_Pink, {0xff, 0xb5, 0xb5, 255}},
    };

    for (auto p : colors)
    {
        if (p.first == color)
        {
            textColor = p.second;
            break;
        }
    }

    SDL_Surface* surf = nullptr;

    if (wrapPixels == 0)
        surf = TTF_RenderUTF8_Blended(data->font, utf8, textColor);
    else
    {
        surf = TTF_RenderUTF8_Blended_Wrapped(data->font, utf8, textColor, wrapPixels);

        if (surf && surf->w > (i32)wrapPixels)
            c.log->LogInfo("Encountered TTF_RenderUTF8_Blended_Wrapped bug where wrapping fails.");
    }

    if (surf == nullptr)
        c.log->FatalError("Error rendering font: %s", TTF_GetError());

    SDL_Texture* tex = data->SurfaceToTexture(surf);

    shared_ptr<ManagedTexture> mt = make_shared<ManagedTexture>(tex);
    shared_ptr<DrawnObject> drawn = make_shared<DrawnObject>(data, layer, mt, "text");

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
