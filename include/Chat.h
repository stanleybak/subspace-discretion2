/*
 * Chat.h
 *
 *  Created on: May 28, 2016
 *      Author: stan
 */

#pragma once

#include "Module.h"

enum ChatType
{
    Chat_Public,
    Chat_Team,
    Chat_EnemyTeam,
    Chat_Private,
    Chat_Channel,
    Chat_Remote,
    Chat_Mode,
    Chat_Internal
};

class ChatData;

class Chat : Module
{
   public:
    Chat(Client& c);

    void SetEscPressedState(bool state);

    void TextTyped(const char* utf8);
    void TextBackspace();
    void TextEnter();

    void ChatMessage(ChatType type, const char* playerNameUtf8, const char* textUtf8);

    void InternalMessage(const char* textUtf8);

    // the argument to func is the full utf8 text, which starts with ?command
    void AddInternalCommand(const char* command, std::function<void(const char*)> func);

   private:
    shared_ptr<ChatData> data;
};
