#include "Connection.h"
#include "Packets.h"
#include "Net.h"

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

    const char* PASSWORD_RESPONSE_CODES[] = {
        "Login OK", "Unregistered Player", "Bad Password", "Arena is Full", "Locked Out of Zone",
        "Permission Only Arena", "Permission to Spectate Only", "Too Many Points to Play Here",
        "Connection is too Slow", "Permission Only Arena", "Server is Full", "Invalid Name",
        "Offensive Name", "No Active Biller", "Server Busy, Try Later", "Restricted Zone",
        "Demo Version Detected", "Too Many Demo Users", "Demo Versions not Allowed",
    };

    Client& c;
    string username;
    string password;
    string connectAddr;
    ConnectionStatus state = STATUS_NOT_CONNECTED;

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
        PacketInstance p;
        p.setValue("protocol", discretionProtocolVersion);
        p.setValue("key", discretionEncryptionKey);
        c.net->sendPacket(&p, "encryption request");

        // reset variables
        shouldDisconnect = false;
        numberSent = 1;

        // start the timer to resend it
        timers->singleTimer(__FILE__, connectRetryDelay, resendEncryptionRequest);

        setStatus("Sent Encryption Request", "grey");
        // and set the state
        state = STATUS_SENT_ENCRYPTION_REQUEST;
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
}

Connection::~Connection()
{
}

void Connection::Connect(const char* name, const char* pw, const char* hostname, u16 port)
{
    data->Connect(name, pw, hostname, port);
}
