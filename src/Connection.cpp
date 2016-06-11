#include <SDL2/SDL.h>
#include "Connection.h"
#include "Packets.h"
#include "Net.h"
#include "Chat.h"
using namespace std;

static const char* PASSWORD_RESPONSE_MOD_ERROR = "Restricted Zone, Mod Access Required";
static const char* PASSWORD_RESPONSE_UNDEF_ERROR = "Undefined Error Code";

static const char* PASSWORD_RESPONSE_CODES[19] = {
    "Login OK", "Unregistered Player", "Bad Password", "Arena is Full", "Locked Out of Zone",
    "Permission Only Arena", "Permission to Spectate Only", "Too Many Points to Play Here",
    "Connection is too Slow", "Permission Only Arena", "Server is Full", "Invalid Name",
    "Offensive Name", "No Active Biller", "Server Busy, Try Later", "Restricted Zone",
    "Demo Version Detected", "Too Many Demo Users", "Demo Versions not Allowed",
};

static const char* GetPasswordErrorCode(u8 code)
{
    const char* rv = PASSWORD_RESPONSE_UNDEF_ERROR;

    if (code == 0xFF)
        rv = PASSWORD_RESPONSE_MOD_ERROR;
    else if (code < sizeof(PASSWORD_RESPONSE_CODES) / sizeof(PASSWORD_RESPONSE_CODES[0]))
        rv = PASSWORD_RESPONSE_CODES[code];

    return rv;
}

struct ConnectionData
{
    enum ConnectionStatus
    {
        STATUS_NOT_CONNECTED,            // before we connect, or after we disconnect
        STATUS_SENT_ENCRYPTION_REQUEST,  // we sent an encrpyion request, but have not yet received
                                         // a
                                         // response
        STATUS_SENT_PASSWORD,            // we've send the password, but have yet to get a response

        STATUS_SENT_ARENA_REQUEST,  // we've sent out enter arena request, but aren't in the arena
                                    // yet
        STATUS_IN_ARENA,            // we are in an arena, sending and receiving position packets
    };

    Client& c;
    string username;
    string password;
    string connectAddr;
    ConnectionStatus state = STATUS_NOT_CONNECTED;
    i32 protocolVersion = c.cfg->GetInt("Net", "Protocol Version", 0x1);
    i32 encryptionKey = c.cfg->GetInt("Net", "Encryption Key", 0x1);
    i32 clientVersion = c.cfg->GetInt("Net", "Client Version", 0x02);

    i32 numberEncryptionRequests = 0;
    i32 nextEncryptionRequestMs = 0;

    i32 connectRetryMs = c.cfg->GetInt("Net", "Connection Retry Delay Mills", 1000);
    i32 maxConnectionAttempts = c.cfg->GetInt("Net", "Max Connection Attempts", 10);

    std::function<void(const char*)> connectFunc = [this](const char* textUtf8)
    {
        if (string("?connect") == textUtf8)
        {
            if (state != STATUS_NOT_CONNECTED)
                c.chat->InternalMessage("Use ?disconnect first to disconnect.");
            else
            {
                // split into ip and port
                size_t index = connectAddr.find(":");

                if (index == string::npos)
                    c.chat->InternalMessage(
                        string("Malformed address, expected something line 127.0.0.1:5000. Got:" +
                               connectAddr).c_str());
                else
                {
                    string hostname = connectAddr.substr(0, index);
                    const char* portStr = connectAddr.substr(0, index + 1).c_str();
                    i32 port = atoi(portStr);

                    if (port <= 0 || port > 65535)
                        c.chat->InternalMessage(string("Port out of range (expected 1-65535):" +
                                                       string(portStr)).c_str());
                    else
                        c.connection->Connect(username.c_str(), password.c_str(), hostname.c_str(),
                                              (u16)(port));
                }
            }
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
                password = pw;
                string msg = "Password was set.";
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

    ConnectionData(Client& client) : c(client) {}

    void SendConnectRequest()
    {
        PacketInstance p(c, "encryption request");
        p.SetValue("protocol", protocolVersion);
        p.SetValue("key", encryptionKey);
        c.net->SendPacket(&p);

        // reset variables
        state = STATUS_SENT_ENCRYPTION_REQUEST;
        numberEncryptionRequests = 1;
        nextEncryptionRequestMs = connectRetryMs;

        c.log->LogDrivel("Sent First Encryption Request");
    }

    void Connect(const char* username, const char* password, const char* hostname, u16 port)
    {
        string msg = "Connecting to " + string(hostname) + " using port " + to_string(port);
        c.chat->InternalMessage(msg.c_str());

        if (state != STATUS_NOT_CONNECTED)
            c.log->LogError("connect() called when state is not STATUS_NOT_CONNECTED");
        else if (c.net->NewConnection(hostname, port))
        {
            SendConnectRequest();
        }
    }

    std::function<void(const PacketInstance*)> handleEncryptionReponse =
        [this](const PacketInstance* pi)
    {
        if (state == STATUS_SENT_ENCRYPTION_REQUEST)
        {
            c.chat->InternalMessage("Got Encryption Response; Sent Password Request");
            // in the future we might do something with the client key

            // send the password packet
            PacketInstance packet(c, "password request");

            packet.SetValue("name", username.c_str());
            packet.SetValue("password", password.c_str());
            packet.SetValue("client version", clientVersion);

            c.net->SendReliablePacket(&packet);

            PacketInstance p2(c, "sync ping");
            p2.SetValue("timestamp", SDL_GetTicks());
            c.net->SendPacket(&p2);

            state = STATUS_SENT_PASSWORD;
        }
        // otherwise it might have been a packet that we just got late from an earlier request,
        // silently
        // ignore
    };

    std::function<void(const PacketInstance*)> handlePasswordResponse =
        [this](const PacketInstance* pi)
    {
        if (state == STATUS_SENT_PASSWORD)
        {
            const int LOGIN_OK = 0x00;
            const int SHIP_SPEC = 0x08;
            const int RANDOM_PUB = 0xFFFF;

            const int X_RES = 800;
            const int Y_RES = 600;

            u8 loginCode = pi->GetValue("login response");
            char buf[128];
            snprintf(buf, sizeof(buf), "Recevied Password Response: %s (0x%02x)",
                     GetPasswordErrorCode(loginCode), loginCode);
            c.chat->InternalMessage(buf);

            if (loginCode == LOGIN_OK)
            {
                // send cancel stream request (for news.txt, I think)
                PacketInstance cancelStream(c, "cancel stream request");
                c.net->SendReliablePacket(&cancelStream);

                // so we're expecting the ack of the cancel above
                c.net->ExpectStreamTransfer(nullptr, nullptr);

                // send arena login
                PacketInstance arenaLogin(c, "arena login");

                arenaLogin.SetValue("ship", SHIP_SPEC);
                arenaLogin.SetValue("allow audio", 1);
                arenaLogin.SetValue("x resolution", X_RES);
                arenaLogin.SetValue("y resolution", Y_RES);
                arenaLogin.SetValue("arena number", RANDOM_PUB);

                c.net->SendReliablePacket(&arenaLogin);
                c.chat->InternalMessage("Sent Arena Login");

                state = STATUS_SENT_ARENA_REQUEST;
            }
            else
            {
                char buf[128];

                snprintf(buf, sizeof(buf), "WARNING: Login Failed: %s (0x%02x)",
                         GetPasswordErrorCode(loginCode), loginCode);
                c.chat->InternalMessage(buf);
            }
        }
    };

    std::function<void(const PacketInstance*)> handleDisconnect = [this](const PacketInstance* pi)
    {
    };
};

Connection::Connection(Client& c) : Module(c), data(make_shared<ConnectionData>(c))
{
    data->username = c.cfg->GetString("connection", "username", "Player");
    data->password = c.cfg->GetString("connection", "password", "1234");
    data->connectAddr = c.cfg->GetString("connection", "connect_addr", "127.0.0.1:5000");

    c.chat->AddInternalCommand("connect", data->connectFunc);
    c.chat->AddInternalCommand("name", data->nameFunc);
    c.chat->AddInternalCommand("pw", data->pwFunc);
    c.chat->AddInternalCommand("ip", data->ipFunc);

    string intro = "Use ?connect to connect to " + data->connectAddr + " with username " +
                   data->username + " or change using ?name, ?pw, or ?ip.";

    c.chat->ChatMessage(Chat_Remote, nullptr, intro.c_str());

    c.net->AddPacketHandler("encryption response", data->handleEncryptionReponse);
    c.net->AddPacketHandler("password response", data->handlePasswordResponse);
    c.net->AddPacketHandler("disconnect", data->handleDisconnect);
}

Connection::~Connection()
{
}

void Connection::Connect(const char* name, const char* pw, const char* hostname, u16 port)
{
    data->Connect(name, pw, hostname, port);
}

bool Connection::isDisconnected()
{
    return data->state == data->STATUS_NOT_CONNECTED;
}

void Connection::Disconnect()
{
    if (data->state != data->STATUS_NOT_CONNECTED)
    {
        data->state = data->STATUS_NOT_CONNECTED;

        PacketInstance pi(c, "disconnect");
        c.net->SendPacket(&pi);

        c.net->DisconnectSocket();
    }
}

void Connection::UpdateConnectionStatus(i32 ms)
{
    if (data->state == data->STATUS_SENT_ENCRYPTION_REQUEST)
    {
        data->nextEncryptionRequestMs -= ms;

        if (data->nextEncryptionRequestMs < 0)
        {
            if (++data->numberEncryptionRequests > data->maxConnectionAttempts)
            {
                c.chat->InternalMessage("Server did not respond to connection requests.");
                c.log->LogDrivel("Exceeded max connection attempts, giving up.");
                c.connection->Disconnect();
                data->state = data->STATUS_NOT_CONNECTED;
            }
            else
            {
                c.log->LogDrivel("Sending Encryption Request #%d", data->numberEncryptionRequests);
                data->nextEncryptionRequestMs = data->connectRetryMs;

                PacketInstance p(c, "encryption request");
                p.SetValue("protocol", data->protocolVersion);
                p.SetValue("key", data->encryptionKey);
                c.net->SendPacket(&p);
            }
        }
    }
}

const char* Connection::GetPlayerName()
{
    return data->username.c_str();
}
