/*
 * Ships.h
 *
 *  Created on: May 27, 2016
 *      Author: stan
 */

#pragma once

#include "Module.h"

struct ShipsData;

class Ships : Module
{
   public:
    Ships(Client& c);
    void AdvanceState(i32 difMs);

   private:
    shared_ptr<ShipsData> data;
};
