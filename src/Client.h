/*
 * The client object is the main holder of all the interfaces.
 *
 * Stanley Bak (May 2016)
 */

#ifndef SRC_CLIENT_H_
#define SRC_CLIENT_H_

class Client;

class Module
{
   public:
    Module(Client& client) : c(client) {}

   protected:
    Client& c;
};

#include "SDLman.h"
#include "Logman.h"

class Client
{
   public:
    SDLman sdl = SDLman(*this);
    Logman log = Logman(*this);
};

#endif  // SRC_CLIENT_H_
