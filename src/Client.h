/*
 * The client object is the main holder of all the interfaces.
 *
 * Stanley Bak (May 2016)
 */

#ifndef SRC_CLIENT_H_
#define SRC_CLIENT_H_

#include <cstdint>
typedef int32_t i32;
typedef int16_t i16;
typedef int8_t i8;
typedef uint8_t u8;

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
#include "Config.h"

class Client
{
   public:
    Client();
    void Start();

    Logman log = Logman(*this);
    Config cfg = Config(*this);
    SDLman sdl = SDLman(*this);
};

#endif  // SRC_CLIENT_H_
