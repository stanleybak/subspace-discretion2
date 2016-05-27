#include "Text.h"
#include "Graphics.h"
#include "SDL2/SDL_ttf.h"
#include <stdarg.h>
#include <stdio.h>

struct TextData
{
    string fontFolder;
    TTF_Font* font = nullptr;
};

Text::Text(Client& c) : Module(c), data(make_shared<TextData>())
{
    if (TTF_Init() < 0)
        c.log->FatalError("Failed to initialize SDL_ttf: %s", TTF_GetError());

    const SDL_version* linkedVer = TTF_Linked_Version();
    SDL_version compVer;
    SDL_TTF_VERSION(&compVer);
    c.log->LogDrivel("SDL_TTF linked version was %d.%d.%d, compiled version was %d.%d.%d",
                     linkedVer->major, linkedVer->minor, linkedVer->patch, compVer.major,
                     compVer.minor, compVer.patch);

    // load font
    data->fontFolder = c.cfg->GetString("text", "folder", "resources");

    string fontPath =
        data->fontFolder + "/" + c.cfg->GetString("text", "font_file", "SourceHanSans.otf");

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
}

Text::~Text()
{
    TTF_CloseFont(data->font);

    TTF_Quit();
}

void Text::DrawTextScreen(TextColor color, i32 x, i32 y, const char* format, ...)
{
    char buf[256];
    va_list args;
    va_start(args, format);

    vsnprintf(buf, sizeof(buf), format, args);

    va_end(args);

    SDL_Color textColor = {255, 0, 0, 255};      // red
    SDL_Color backgroundColor = {0, 0, 0, 255};  // black
    SDL_Surface* surf = TTF_RenderText_Shaded(data->font, buf, textColor, backgroundColor);
    SDL_Texture* tex = c.graphics->SurfaceToTexture(surf);

    SDL_Rect rect;
    SDL_QueryTexture(tex, NULL, NULL, &rect.w, &rect.h);

    rect.x = x;
    rect.y = y;

    SDL_RenderCopy(c.graphics->getRenderer(), tex, nullptr, &rect);
}
