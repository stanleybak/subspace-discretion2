/*
 * The client object is the main holder of all the interfaces.
 *
 * Stanley Bak (May 2016)
 */
#pragma once

#include <memory>
using namespace std;

// Use forward declarations for modules, rather than including all headers
// This speed up compilation time on changes to any header file
// When adding a new module, be sure to initialize the associated pointer in Client::Client()
class Logman;
class Config;
class SDLman;
class Graphics;
class SegFaultHandler;
class Timers;
class Ships;
class Chat;
class Net;
class Packets;
class Connection;
class Players;

// only works on non-windows
void PrintStackTrace();

class Client
{
   public:
    Client();
    void Start();

    // these must be pointers because we use forward declarations (since the sizes of the types are
    // unknown)
    shared_ptr<SegFaultHandler> segFaultHandler;  // created first to catch initialization errors
    shared_ptr<Config> cfg;
    shared_ptr<Logman> log;
    shared_ptr<Packets> packets;
    shared_ptr<Net> net;
    shared_ptr<SDLman> sdl;
    shared_ptr<Graphics> graphics;
    shared_ptr<Timers> timers;
    shared_ptr<Chat> chat;
    shared_ptr<Ships> ships;
    shared_ptr<Connection> connection;
    shared_ptr<Players> players;
};
