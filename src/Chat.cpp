#include "Chat.h"
#include "utf8.h"
#include "Graphics.h"
#include <deque>

class ChatData
{
   public:
    ChatData(Client& c) : c(c) {}

    Client& c;
    string curTextUtf8;
    u32 maxTypingBytes = 200;
    u32 screenW = 0, screenH = 0;
    i32 fontHeight = 0;
    i32 maxBufferLines = 50;
    i32 displayLines = 5;

    bool escState = false;

    vector<shared_ptr<DrawnText>> curTextLines;
    deque<shared_ptr<DrawnText>> chatBufferLines;

    map<ChatType, TextColor> chatColorMap = {{Chat_Public, Color_Blue},
                                             {Chat_Team, Color_Yellow},
                                             {Chat_EnemyTeam, Color_Purple},
                                             {Chat_Private, Color_Green},
                                             {Chat_Channel, Color_Orange},
                                             {Chat_Remote, Color_Green},
                                             {Chat_Mode, Color_Pink},
                                             {Chat_Internal, Color_Green}};

    map<u8, ChatType> prefixMap = {
        {'\'', Chat_Team}, {'\"', Chat_EnemyTeam}, {'/', Chat_Private}, {';', Chat_Channel}};

    void UpdateTypingText();
    ChatType GetTypingChatType();
    void UpdateChatLineVisuals();
};

Chat::Chat(Client& c) : Module(c), data(make_shared<ChatData>(c))
{
    data->maxTypingBytes = c.cfg->GetInt("Chat", "max_typing_bytes", 200);
    data->maxBufferLines = c.cfg->GetInt("Chat", "buffer_lines", 50);
    data->displayLines = c.cfg->GetInt("Chat", "display_lines", 5);

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

ChatType ChatData::GetTypingChatType()
{
    ChatType type = Chat_Public;
    u8 firstChar = curTextUtf8[0];
    auto it = prefixMap.find(firstChar);

    if (it != prefixMap.end())
        type = it->second;

    return type;
}

void Chat::TextEnter()
{
    if (data->curTextUtf8.length() > 0)
    {
        ChatType type = data->GetTypingChatType();

        // pop off first character
        if (type != Chat_Public)
            data->curTextUtf8 = data->curTextUtf8.substr(1, data->curTextUtf8.length() - 1);

        ChatMessage(type, "Player", data->curTextUtf8.c_str());

        data->curTextUtf8 = "";
        data->UpdateTypingText();
    }
}

void Chat::SetEscPressedState(bool state)
{
    data->escState = state;

    data->UpdateChatLineVisuals();
}

void ChatData::UpdateTypingText()
{
    if (curTextUtf8.length() == 0)
        curTextLines.clear();
    else
    {
        ChatType type = GetTypingChatType();
        TextColor col = Color_Grey;

        if (type != Chat_Public)  // special case when they are sending a public message
            col = chatColorMap.find(type)->second;

        curTextLines.clear();
        c.graphics->MakeDrawnChat(curTextLines, Layer_Chat, col, screenW, nullptr,
                                  curTextUtf8.c_str());
    }

    UpdateChatLineVisuals();
}

void ChatData::UpdateChatLineVisuals()
{
    i32 offsetY = screenH;
    i32 displayed = 0;

    for (auto line = curTextLines.rbegin(); line != curTextLines.rend(); ++line)
    {
        offsetY -= fontHeight;
        (*line)->SetPosition(0, offsetY);

        if (escState == true || ++displayed <= displayLines)
            (*line)->SetVisible(true);
        else
            (*line)->SetVisible(false);
    }

    for (shared_ptr<DrawnText> line : chatBufferLines)
    {
        offsetY -= fontHeight;
        line->SetPosition(0, offsetY);

        if (escState == true || ++displayed <= displayLines)
            line->SetVisible(true);
        else
            line->SetVisible(false);
    }
}

void Chat::ChatMessage(ChatType type, const char* playerNameUtf8, const char* textUtf8)
{
    // chatBufferLines

    TextColor col = data->chatColorMap.find(type)->second;

    vector<shared_ptr<DrawnText>> newLines;
    c.graphics->MakeDrawnChat(newLines, Layer_Chat, col, data->screenW, playerNameUtf8, textUtf8);

    for (shared_ptr<DrawnText> line : newLines)
        data->chatBufferLines.push_front(line);

    while ((i32)data->chatBufferLines.size() > data->maxBufferLines)
        data->chatBufferLines.pop_back();

    data->UpdateChatLineVisuals();
}
