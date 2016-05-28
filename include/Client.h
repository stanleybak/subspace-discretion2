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
class Text;
class Ships;
class Chat;

class Client
{
   public:
    Client();
    void Start();

    // these must be pointers because we use forward declarations (since the sizes of the types are
    // unknown)
    shared_ptr<Config> cfg;
    shared_ptr<Logman> log;
    shared_ptr<SDLman> sdl;
    shared_ptr<Graphics> graphics;
    shared_ptr<Chat> chat;
    shared_ptr<Ships> ships;
};
