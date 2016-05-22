/*
 * Util.h
 *
 *  Created on: May 22, 2016
 *      Author: stan
 */
#pragma once

#include "Module.h"

class Util : Module
{
    using Module::Module;

   public:
    i32 getWallMills();
};
