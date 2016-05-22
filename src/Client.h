/*
 * The client object is the main holder of all the interfaces.
 *
 * Stanley Bak (May 2016)
 */

#ifndef SRC_CLIENT_H_
#define SRC_CLIENT_H_

#include <memory>
using namespace std;

// Use forward declarations for modules, rather than including all headers
// This speed up compilation time on changes to any header file
// When adding a new module, be sure to initialize the associated shared_ptr in Client::Client()
class Logman;
class Config;
class SDLman;
class Graphics;
class Util;

class Client
{
   public:
    Client();
    void Start();

    // these must be pointers because we use forward declarations
    shared_ptr<Logman> log;
    shared_ptr<Config> cfg;
    shared_ptr<SDLman> sdl;
    shared_ptr<Graphics> graphics;
    shared_ptr<Util> util;
};

#endif  // SRC_CLIENT_H_
