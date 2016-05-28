#include "Client.h"
#include "SDLman.h"
#include "Logman.h"
#include "Config.h"
#include "Graphics.h"
#include "Ships.h"
#include "Chat.h"

Client::Client()
    :  // initialization order matters, add to the end
      cfg(make_shared<Config>(*this)),
      log(make_shared<Logman>(*this)),
      sdl(make_shared<SDLman>(*this)),
      graphics(make_shared<Graphics>(*this)),
      chat(make_shared<Chat>(*this)),
      ships(make_shared<Ships>(*this))
{
}

void Client::Start()
{
    sdl->MainLoop();
}
