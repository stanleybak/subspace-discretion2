#include "Net.h"
#include "Chat.h"
#include <string.h>

struct NetData
{
    Client& c;
    string username;
    string pwOverride;
    string connectAddr;

    NetData(Client& c) : c(c) {}

    std::function<void(const char*)> connectFunc = [this](const char* textUtf8)
    {
        if (string("?connect") == textUtf8)
        {
            string msg = "Connecting to " + connectAddr;
            c.chat->InternalMessage(msg.c_str());
        }
        else
            c.chat->InternalMessage("Expected plain '?connect' command.");
    };

    std::function<void(const char*)> nameFunc = [this](const char* textUtf8)
    {
        if (strstr(textUtf8, "?name=") == textUtf8)
        {
            string name = textUtf8 + sizeof("?name");

            if (name.length() > 0)
            {
                username = name;
                string msg = "Set name to " + username + ".";
                c.chat->InternalMessage(msg.c_str());
            }
            else
                c.chat->InternalMessage("Expected '?name=XYZ'.");
        }
        else
            c.chat->InternalMessage("Expected '?name=XYZ'.");
    };

    std::function<void(const char*)> pwFunc = [this](const char* textUtf8)
    {
        if (strstr(textUtf8, "?pw=") == textUtf8)
        {
            string pw = textUtf8 + sizeof("?pw");

            if (pw.length() > 0)
            {
                pwOverride = pw;
                string msg = "Set password.";
                c.chat->InternalMessage(msg.c_str());
            }
            else
                c.chat->InternalMessage("Expected '?pw=XYZ'.");
        }
        else
            c.chat->InternalMessage("Expected '?pw=XYZ'.");
    };

    std::function<void(const char*)> ipFunc = [this](const char* textUtf8)
    {
        if (strstr(textUtf8, "?ip=") == textUtf8)
        {
            string addr = textUtf8 + sizeof("?ip");

            if (addr.length() > 0)
            {
                connectAddr = addr;
                string msg = "Set connect address to " + connectAddr;
                c.chat->InternalMessage(msg.c_str());
            }
            else
                c.chat->InternalMessage("Expected '?ip=XYZ'.");
        }
        else
            c.chat->InternalMessage("Expected '?ip=XYZ'.");
    };
};

Net::Net(Client& c) : Module(c), data(make_shared<NetData>(c))
{
    data->username = c.cfg->GetString("net", "username", "Player");
    data->connectAddr = c.cfg->GetString("net", "connect_addr", "127.0.0.1:5000");

    c.chat->AddInternalCommand("connect", data->connectFunc);
    c.chat->AddInternalCommand("name", data->nameFunc);
    c.chat->AddInternalCommand("pw", data->pwFunc);
    c.chat->AddInternalCommand("ip", data->ipFunc);

    string intro = "Use ?connect to connect to " + data->connectAddr + " with username " +
                   data->username + " or change using ?name, ?pw, or ?ip.";

    c.chat->ChatMessage(Chat_Remote, nullptr, intro.c_str());
}

Net::~Net()
{
}

const char* Net::GetPlayerName()
{
    return data->username.c_str();
}
