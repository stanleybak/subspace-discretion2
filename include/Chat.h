/*
 * Chat.h
 *
 *  Created on: May 28, 2016
 *      Author: stan
 */

#pragma once

#include "Module.h"

class ChatData;

class Chat : Module
{
   public:
    Chat(Client& c);

    void TextTyped(const char* utf8);
    void TextBackspace();
    void TextEnter();

   private:
    shared_ptr<ChatData> data;
};
