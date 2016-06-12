/*
 * Module.h
 *
 *  Created on: May 22, 2016
 *      Author: stan
 */

#pragma once

#include "Compatibility.h"
#include "Client.h"
using namespace std;

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
