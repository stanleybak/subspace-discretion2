/*
 * Text.h
 *
 *  Created on: May 27, 2016
 *      Author: stan
 */

#pragma once
#include <stdarg.h>
#include <stdio.h>
#include "Module.h"

struct TextData;

enum TextColor
{
    Text_Red,
    Text_Blue,
};

class Text : public Module
{
   public:
    Text(Client& c);
    ~Text();

    // called in render loop
    void DrawTextScreen(TextColor color, i32 x, i32 y, const char* format, ...);

   private:
    shared_ptr<TextData> data;
};
