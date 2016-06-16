#include "Graphics.h"
#include "SDLman.h"
#include "Players.h"
#include "Map.h"

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

    shared_ptr<Image> image;
    u32 animMs;
    u32 animFrameOffset;
    u32 animNumFrames;
};

// super class for all drawn objects
class DrawnObject
{
   public:
    DrawnObject(shared_ptr<GraphicsData> gd, Layer layer, shared_ptr<ManagedTexture> texture,
                bool isMapImage, const char* name);
    ~DrawnObject();

    void Draw(SDL_Renderer* renderer);
    void Draw(SDL_Renderer* renderer, int xOffset, int yOffset);
    string ToString();

    Layer layer;
    bool isMapImage;
    shared_ptr<ManagedTexture> texture;
    SDL_Rect src = {0, 0, -1, 0};  // src.w = -1 means use nullptr
    SDL_Rect dest = {0, 0, 0, 0};
    bool visible = true;
    const char* name = nullptr;

   private:
    shared_ptr<GraphicsData> gd;
};

struct GraphicsData
{
    GraphicsData(Client& client) : c(client) {}
    Client& c;
    string graphicsFolder;

    TTF_Font* font = nullptr;
    bool useBlendedFont = false;

    u32 windowW = 1, windowH = 1;
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;

    shared_ptr<ManagedTexture> notFoundTexture = nullptr;

    void LoadIcon();
    void RenderWrappedSurface(vector<SDL_Surface*>& store, const char* playerName, const char* utf8,
                              SDL_Color textColor, i32 wrapPixels);
    SDL_Surface* RenderName(const char* playerNameUtf8, SDL_Color textColor, i32 maxLen);
    SDL_Surface* LoadSurface(const string* path);
    SDL_Surface* MakeMemorySurface(i32 w, i32 h);  // won't return nul
    SDL_Texture* SurfaceToTexture(SDL_Surface* s);
    shared_ptr<ManagedTexture> LoadTexture(const char* filename);
    void MakeDrawnText(vector<shared_ptr<DrawnText>>& store, shared_ptr<GraphicsData> gd,
                       Layer layer, TextColor color, u32 wrapPixels, const char* playerNameUtf8,
                       const char* utf8, bool isMap);
    void MakeWrappedTextSurfaces(vector<SDL_Surface*>& surfStore, vector<string>& lines,
                                 SDL_Surface* nameSurface, SDL_Color textColor, i32 wrapPixels);
    void DrawImageFrame(shared_ptr<Image> i, i32 frame, i32 pixelX, i32 pixelY);

    u32 nowMs = 0;  // for tracking single animation expiration time
    multimap<Layer, DrawnObject*> drawnObjs;
    multimap<u32, shared_ptr<DrawnImage>> singleAnimations;  // expirationMs -> DrawnImage
    unordered_set<DrawnImage*> animations;

    map<TextColor, SDL_Color> colorMap = {
        {Color_Grey, {0xb5, 0xb5, 0xb5, 255}},
        {Color_Green, {0x73, 0xff, 0x63, 255}},
        {Color_Blue, {0xb5, 0xb5, 0xff, 255}},
        {Color_Red, {0xde, 0x31, 0x08, 255}},
        {Color_Yellow, {0xff, 0xbd, 0x29, 255}},
        {Color_Purple, {0xad, 0x6b, 0xf7, 255}},
        {Color_Orange, {0xef, 0x6b, 0x18, 255}},
        {Color_Pink, {0xff, 0xb5, 0xb5, 255}},
    };
};

void GraphicsData::DrawImageFrame(shared_ptr<Image> i, i32 frame, i32 pixelX, i32 pixelY)
{
    int xFrame = frame % i->numXFrames;
    int yFrame = frame / i->numXFrames;

    if (yFrame > i->numYFrames)
    {
        c.log->LogError(
            "Graphics::DrawImageFrame() called with out of bounds frame (%d) for %dx%d image %s",
            frame, i->numXFrames, i->numYFrames, i->filename.c_str());
    }
    else
    {
        int offsetX = xFrame * i->frameWidth;
        int offsetY = yFrame * i->frameHeight;

        SDL_Rect src = {offsetX, offsetY, i->frameWidth, i->frameHeight};
        SDL_Rect dest = {pixelX, pixelY, i->frameWidth, i->frameHeight};

        SDL_RenderCopy(renderer, i->texture->rawTexture, &src, &dest);
    }
}

void GraphicsData::MakeDrawnText(vector<shared_ptr<DrawnText>>& store,
                                 shared_ptr<GraphicsData> data, Layer layer, TextColor color,
                                 u32 wrapPixels, const char* playerNameUtf8, const char* utf8,
                                 bool isMap = false)
{
    SDL_Color textColor = colorMap.find(color)->second;

    vector<SDL_Surface*> surfStore;

    if (wrapPixels == 0)
    {
        SDL_Surface* surf = nullptr;

        if (playerNameUtf8 != nullptr)
            c.log->LogError("wrapPixels == 0 with non-null playerName not supported.");

        if (useBlendedFont)
            surf = TTF_RenderUTF8_Blended(font, utf8, textColor);
        else
            surf = TTF_RenderUTF8_Solid(font, utf8, textColor);

        if (surf == nullptr)
            c.log->FatalError("Error rendering font: %s", TTF_GetError());

        surfStore.push_back(surf);
    }
    else
        RenderWrappedSurface(surfStore, playerNameUtf8, utf8, textColor, wrapPixels);

    for (SDL_Surface* surf : surfStore)
    {
        SDL_Texture* tex = SurfaceToTexture(surf);

        shared_ptr<ManagedTexture> mt = make_shared<ManagedTexture>(tex);
        shared_ptr<DrawnObject> drawn = make_shared<DrawnObject>(data, layer, mt, isMap, "text");

        SDL_QueryTexture(tex, NULL, NULL, &drawn->dest.w, &drawn->dest.h);

        store.push_back(make_shared<DrawnText>(drawn));
    }
}

// won't return null
SDL_Surface* GraphicsData::MakeMemorySurface(i32 w, i32 h)
{
    Uint32 rmask, gmask, bmask, amask;

/* SDL interprets each pixel as a 32-bit number, so our masks must depend
   on the endianness (byte order) of the machine */
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
#else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif

    SDL_Surface* rv = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 32, rmask, gmask, bmask, amask);

    if (rv == NULL)
        c.log->FatalError(" SDL_CreateRGBSurface() failed: %s", SDL_GetError());

    return rv;
}

// returns a valid SDL_Surface
SDL_Surface* GraphicsData::RenderName(const char* playerNameUtf8, SDL_Color textColor, i32 maxLen)
{
    SDL_Surface* nameSurface = nullptr;

    if (playerNameUtf8 != nullptr)
    {
        // use bold font for names
        TTF_SetFontStyle(font, TTF_STYLE_BOLD);

        string renderedPlayerName;
        string originalPlayerName = playerNameUtf8;

        auto itName = originalPlayerName.begin();
        auto endName = utf8::find_invalid(originalPlayerName.begin(), originalPlayerName.end());

        while (itName != endName)
        {
            u32 codePoint = utf8::next(itName, endName);

            // add a single codePoint to curText, if there's space
            unsigned char u[5] = {0, 0, 0, 0, 0};
            unsigned char* end = utf8::append(codePoint, u);
            u32 len = end - u;

            for (u32 i = 0; i < len; ++i)
                renderedPlayerName += u[i];

            // check if length of renderedPlayerName goes over maxNameLen
            i32 w = 0, h = 0;

            if (TTF_SizeUTF8(font, renderedPlayerName.c_str(), &w, &h))
                c.log->FatalError("TTF_SizeUTF8 Failed: '%s'", TTF_GetError());

            if (w > maxLen)
            {
                // pop last character to go back under the limit
                for (u32 i = 0; i < len; ++i)
                    renderedPlayerName.pop_back();

                // stop appending characters
                break;
            }
        }

        // add the name seperator
        renderedPlayerName += ">  ";

        if (useBlendedFont)
            nameSurface = TTF_RenderUTF8_Blended(font, renderedPlayerName.c_str(), textColor);
        else
            nameSurface = TTF_RenderUTF8_Solid(font, renderedPlayerName.c_str(), textColor);

        if (nameSurface == nullptr)
            c.log->FatalError("TTF_RenderUTF8 Failed: '%s'", TTF_GetError());

        // switch back to normal font
        TTF_SetFontStyle(font, TTF_STYLE_NORMAL);
    }

    return nameSurface;
}

// returns a valid SDL_Surface
void GraphicsData::RenderWrappedSurface(vector<SDL_Surface*>& surfStore, const char* playerNameUtf8,
                                        const char* textUtf8, SDL_Color textColor, i32 wrapPixels)
{
    SDL_Surface* nameSurface = RenderName(playerNameUtf8, textColor, wrapPixels / 4);

    string utf8 = textUtf8;

    vector<string> lines;
    string curLine;

    auto it = utf8.begin();
    auto end = utf8::find_invalid(utf8.begin(), utf8.end());

    while (it != end)
    {
        u32 codePoint = utf8::next(it, end);

        // add a single codePoint to curText, if there's space
        unsigned char u[5] = {0, 0, 0, 0, 0};
        unsigned char* end = utf8::append(codePoint, u);
        u32 len = end - u;

        for (u32 i = 0; i < len; ++i)
            curLine += u[i];

        // check if length of curline goes over wrappixels
        i32 w = 0, h = 0;

        if (TTF_SizeUTF8(font, curLine.c_str(), &w, &h))
            c.log->FatalError("TTF_SizeUTF8 Failed: '%s'", TTF_GetError());

        // on the first line, include extra width for the name
        i32 extraWidth = (lines.size() == 0 && nameSurface != nullptr) ? nameSurface->w : 0;

        if (w + extraWidth > wrapPixels)
        {
            // pop last characters off until a space is encountered, and start a new line
            string::iterator spacePos = curLine.end();

            while (spacePos != curLine.begin())
            {
                u32 codePoint = utf8::prior(spacePos, curLine.begin());

                if (codePoint == (u32)' ')
                    break;
            }

            if (spacePos == curLine.begin())
            {
                // no space was found in curLine, force a break
                for (u32 i = 0; i < len; ++i)
                    curLine.pop_back();

                lines.push_back(curLine);
                curLine = "";

                for (u32 i = 0; i < len; ++i)
                    curLine += u[i];
            }
            else
            {
                // a space was found, copy everything after the space to the next line
                string nextLine = string(spacePos + 1, curLine.end());
                curLine = string(curLine.begin(), spacePos);

                lines.push_back(curLine);
                curLine = nextLine;
            }
        }
    }

    if (curLine.length() > 0)
        lines.push_back(curLine);

    // at this point, lines contains the string to render for each line
    MakeWrappedTextSurfaces(surfStore, lines, nameSurface, textColor, wrapPixels);
}

void GraphicsData::MakeWrappedTextSurfaces(vector<SDL_Surface*>& surfStore, vector<string>& lines,
                                           SDL_Surface* nameSurface, SDL_Color textColor,
                                           i32 wrapPixels)
{
    if (lines.size() == 0)
        lines.push_back(" ");

    i32 lineHeight = c.graphics->GetFontHeight();

    SDL_Surface* lineSurface = nullptr;

    if (nameSurface != nullptr)
    {
        // render image with name> line0_text
        // copy over the player name image
        i32 textW, textH;

        if (TTF_SizeUTF8(font, lines[0].c_str(), &textW, &textH))
            c.log->FatalError("TTF_SizeUTF8 Failed: '%s'", TTF_GetError());

        i32 firstLineWidth = nameSurface->w + textW;

        lineSurface = MakeMemorySurface(firstLineWidth, lineHeight);

        SDL_Rect dest = {0, 0, nameSurface->w, nameSurface->h};

        if (SDL_BlitSurface(nameSurface, nullptr, lineSurface, &dest))
            c.log->FatalError("SDL_BlitSurface Failed: '%s'", SDL_GetError());
    }

    for (u32 lineIndex = 0; lineIndex < lines.size(); ++lineIndex)
    {
        const string& line = lines[lineIndex];

        SDL_Surface* textSurface = nullptr;

        if (useBlendedFont)
            textSurface = TTF_RenderUTF8_Blended(font, line.c_str(), textColor);
        else
            textSurface = TTF_RenderUTF8_Solid(font, line.c_str(), textColor);

        if (textSurface == nullptr)
            c.log->FatalError("TTF_RenderUTF8 Failed: '%s'", TTF_GetError());

        if (lineSurface == nullptr)
        {
            // no existing line surface (not first line)
            surfStore.push_back(textSurface);
        }
        else
        {
            // first line, draw text surface to line surface
            i32 lineXOffset = nameSurface->w;
            SDL_Rect dest = {lineXOffset, 0, textSurface->w, textSurface->h};

            if (SDL_BlitSurface(textSurface, nullptr, lineSurface, &dest))
                c.log->FatalError("SDL_BlitSurface Failed: '%s'", SDL_GetError());

            surfStore.push_back(lineSurface);
            lineSurface = nullptr;
        }
    }

    if (nameSurface != nullptr)
        SDL_FreeSurface(nameSurface);
}

// may return null
SDL_Surface* GraphicsData::LoadSurface(const string* path)
{
    SDL_Surface* surface = nullptr;

    if (strchr(path->c_str(), '.') != nullptr)
    {
        surface = IMG_Load(path->c_str());

        if (!surface)
            c.log->LogError("Error loading image '%s': %s", path->c_str(), IMG_GetError());
    }
    else
    {
        // try a bunch of image extensions
        const char* exts[] = {".png",  ".bmp",  ".bm2", ".jpg", ".gif", ".tif",
                              ".jpeg", ".tiff", ".pnm", ".ppm", ".pgm", ".pbm",
                              ".xpm",  ".lbm",  ".pcx", ".tga", ""};

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

void DrawnObject::Draw(SDL_Renderer* r)
{
    if (visible)
    {
        if (src.w == -1)
            SDL_RenderCopy(r, texture->rawTexture, nullptr, &dest);
        else
            SDL_RenderCopy(r, texture->rawTexture, &src, &dest);
    }
}

void DrawnObject::Draw(SDL_Renderer* r, int xOffset, int yOffset)
{
    if (visible)
    {
        SDL_Rect offsetDest = dest;
        offsetDest.x += xOffset;
        offsetDest.y += yOffset;

        if (src.w == -1)
            SDL_RenderCopy(r, texture->rawTexture, nullptr, &offsetDest);
        else
            SDL_RenderCopy(r, texture->rawTexture, &src, &offsetDest);
    }
}

DrawnObject::DrawnObject(shared_ptr<GraphicsData> gd, Layer layer,
                         shared_ptr<ManagedTexture> texture, bool isMapImage, const char* name)
    : layer(layer), isMapImage(isMapImage), texture(texture), name(name), gd(gd)
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

void DrawnText::SetVisible(bool vis)
{
    d->visible = vis;
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
                       u32 animNumFrames, shared_ptr<Image> i, bool isMap)
    : animMs(animMs),
      animFrameOffset(animFrameOffset),
      animNumFrames(animNumFrames),
      gd(gd),
      image(i)
{
    drawnObj = make_shared<DrawnObject>(gd, layer, image->texture, isMap, image->filename.c_str());

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

const char* DrawnImage::GetName()
{
    return image->filename.c_str();
}

void DrawnImage::SetCenterPosition(i32 x, i32 y)
{
    drawnObj->dest.x = x - image->halfFrameHeight;
    drawnObj->dest.y = y - image->halfFrameWidth;
}

void DrawnImage::SetVisible(bool vis)
{
    drawnObj->visible = vis;
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

    data->useBlendedFont = c.cfg->GetInt("text", "use_blended_font", 1) != 0;
}

Graphics::~Graphics()
{
    // remove all managed animations
    data->singleAnimations.clear();

    // animations should now be empty
    for (auto it : data->animations)
        c.log->LogError("Animation was still in animations list: '%s'", it->GetName());

    // render list should now be empty
    for (auto it2 : data->drawnObjs)
        c.log->LogError("render list still contained item: '%s'", it2.second->name);

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

    shared_ptr<Player> self = c.players->GetSelfPlayer(false);
    int halfWidth = data->windowW / 2;
    int halfHeight = data->windowH / 2;

    bool drewMap = false;
    for (auto it : data->drawnObjs)
    {
        if (!drewMap && it.first >= Layer_Tiles)
        {
            drewMap = true;
            c.map->DrawMap();
        }

        if (it.second->isMapImage)
        {
            if (self != nullptr)
            {
                it.second->Draw(data->renderer, -self->GetXPixel() + halfWidth,
                                -self->GetYPixel() + halfHeight);
            }
        }
        else
            it.second->Draw(data->renderer);
    }

    if (!drewMap)
        c.map->DrawMap();

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

shared_ptr<DrawnImage> Graphics::MakeDrawnImage(Layer layer, shared_ptr<Image> image, bool isMap)
{
    shared_ptr<DrawnImage> rv = make_shared<DrawnImage>(data, layer, 0, 0, 0, image, isMap);

    return rv;
}

shared_ptr<DrawnImage> Graphics::MakeDrawnAnimation(Layer layer, shared_ptr<Animation> anim,
                                                    bool isMap)
{
    shared_ptr<DrawnImage> rv = make_shared<DrawnImage>(
        data, layer, anim->animMs, anim->animFrameOffset, anim->animNumFrames, anim->image, isMap);

    return rv;
}

void Graphics::MakeSingleDrawnAnimation(Layer layer, i32 x, i32 y, shared_ptr<Animation> anim)
{
    shared_ptr<DrawnImage> rv = MakeDrawnAnimation(layer, anim);
    rv->SetCenterPosition(x, y);

    data->singleAnimations.insert(make_pair(data->nowMs + anim->animMs, rv));
}

shared_ptr<DrawnText> Graphics::MakeDrawnText(Layer layer, TextColor color, const char* utf8,
                                              bool isMap)
{
    vector<shared_ptr<DrawnText>> lines;
    data->MakeDrawnText(lines, data, layer, color, 0, nullptr, utf8, isMap);
    return lines[0];
}

void Graphics::MakeDrawnChat(vector<shared_ptr<DrawnText>>& store, Layer layer, TextColor color,
                             i32 wrapPixels, const char* playerNameUtf8, const char* utf8)
{
    data->MakeDrawnText(store, data, layer, color, wrapPixels, playerNameUtf8, utf8);
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

shared_ptr<Animation> Graphics::InitAnimation(shared_ptr<Image> i, u32 animMs, u32 animFrameOffset,
                                              u32 animNumFrames)
{
    return make_shared<Animation>(i, animMs, animFrameOffset, animNumFrames);
}

shared_ptr<Animation> Graphics::InitAnimation(shared_ptr<Image> i, u32 animMs)
{
    return make_shared<Animation>(i, animMs, 0, i->numXFrames * i->numYFrames);
}

void Graphics::DrawImageFrame(shared_ptr<Image> i, i32 frame, i32 pixelX, i32 pixelY)
{
    data->DrawImageFrame(i, frame, pixelX, pixelY);
}
