/*
 * Timers.h
 *
 *  Created on: May 30, 2016
 *      Author: stan
 */

#pragma once

#include "Module.h"

class Timer;
struct TimersData;

class Timers : public Module
{
   public:
    Timers(Client& c);
    ~Timers();

    // timer will run repeatedly, first after mills time, until the pointer goes out of scope
    // SAVE THE RETURN VALUE OR YOUR TIMER WILL NEVER GET CALLED
    shared_ptr<Timer> PeriodicTimer(const char* name, i32 delayMs, std::function<void()> f);

    void SingleTimer(i32 delayMs, std::function<void()> f);

    void AdvanceTime(i32 ms);

   private:
    shared_ptr<TimersData> data;
};
