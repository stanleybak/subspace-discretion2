#include "Client.h"

Client::Client()
{
    cfg.ReadConfig();
    log.Open();
    sdl.Init();
}

void Client::Start()
{
    sdl.MainLoop();
}
