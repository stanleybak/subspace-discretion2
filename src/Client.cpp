#include "SDLman.h"
#include "Logman.h"
#include "Config.h"
#include "Graphics.h"
#include "Client.h"

Client::Client()
    :  // initialization order matters, add to the end
      cfg(make_shared<Config>(*this)),
      log(make_shared<Logman>(*this)),
      sdl(make_shared<SDLman>(*this)),
      graphics(make_shared<Graphics>(*this))
{
}

void Client::Start()
{
    sdl->MainLoop();
}
