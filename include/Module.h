/*
 * Module.h
 *
 *  Created on: May 22, 2016
 *      Author: stan
 */

#pragma once

#include <cstdint>
typedef int64_t i64;
typedef uint64_t u64;
typedef int32_t i32;
typedef uint32_t u32;
typedef int16_t i16;
typedef uint16_t u16;
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
