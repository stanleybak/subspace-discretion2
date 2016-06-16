/*
 * Ships.h
 *
 *  Created on: May 27, 2016
 *      Author: stan
 */

#pragma once

#include "Module.h"
#include "Players.h"

struct ShipsData;

class Ships : Module
{
   public:
    Ships(Client& c);
    void AdvanceState(i32 difMs);

    // completely in an arena
    void NowInGame();

    // key commands
    void DownPressed(bool isPressed);
    void UpPressed(bool isPressed);
    void LeftPressed(bool isPressed);
    void RightPressed(bool isPressed);

    void RequestChangeShip(ShipType s);
    void ShipChanged(ShipType s);

   private:
    shared_ptr<ShipsData> data;
};
