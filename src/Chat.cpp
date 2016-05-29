#include "Chat.h"
#include "utf8.h"
#include "Graphics.h"

class ChatData
{
   public:
    ChatData(Client& c) : c(c) {}

    Client& c;
    string curTextUtf8;
    u32 maxTypingBytes = 200;
    u32 screenW = 0, screenH = 0;
    i32 fontHeight = 0;
    shared_ptr<DrawnText> curTextLineImage = nullptr;

    void UpdateTypingText();
};

Chat::Chat(Client& c) : Module(c), data(make_shared<ChatData>(c))
{
    data->maxTypingBytes = c.cfg->GetInt("Chat", "max_typing_bytes", 200);
    c.graphics->GetScreenSize(&data->screenW, &data->screenH);

    data->fontHeight = c.graphics->GetFontHeight();
}

void Chat::TextTyped(const char* utf8)
{
    string typed = utf8;

    auto it = typed.begin();
    auto end = utf8::find_invalid(typed.begin(), typed.end());

    while (it != end)
    {
        u32 codePoint = utf8::next(it, end);

        // add a single codePoint to curText, if there's space
        unsigned char u[5] = {0, 0, 0, 0, 0};
        unsigned char* end = utf8::append(codePoint, u);
        u32 len = end - u;

        if (data->curTextUtf8.length() + len <= data->maxTypingBytes)
        {
            for (u32 i = 0; i < len; ++i)
                data->curTextUtf8 += u[i];
        }
    }

    data->UpdateTypingText();
}

void Chat::TextBackspace()
{
    if (data->curTextUtf8.length() > 0)
    {
        auto it = data->curTextUtf8.end();
        u32 codePoint = utf8::prior(it, data->curTextUtf8.begin());

        unsigned char u[5] = {0, 0, 0, 0, 0};
        unsigned char* end = utf8::append(codePoint, u);
        u32 codeLen = end - u;

        for (u32 i = 0; i < codeLen; ++i)
            data->curTextUtf8.pop_back();
    }

    data->UpdateTypingText();
}

void Chat::TextEnter()
{
    data->curTextUtf8 = "";
    data->UpdateTypingText();
}

void ChatData::UpdateTypingText()
{
    if (curTextUtf8.length() == 0)
        curTextLineImage = nullptr;
    else
    {
        curTextLineImage = c.graphics->MakeDrawnChat(Layer_Chat, Color_Grey, screenW, "Player",
                                                     curTextUtf8.c_str());

        u32 h = curTextLineImage->GetHeight();

        curTextLineImage->SetPosition(0, screenH - h);
    }
}
