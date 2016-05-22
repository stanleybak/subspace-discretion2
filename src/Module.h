/*
 * Module.h
 *
 *  Created on: May 22, 2016
 *      Author: stan
 */

#pragma once

#include <cstdint>
typedef int32_t i32;
typedef int16_t i16;
typedef int8_t i8;
typedef uint8_t u8;

#include "Client.h"

class Module
{
   public:
    Module(Client& client) : c(client) {}

   protected:
    Client& c;
};

// include common headers that most modules will use
#include "Config.h"
#include "Logman.h"
