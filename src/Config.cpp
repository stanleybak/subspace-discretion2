#include "Config.h"
#include <memory>
using namespace std;

void Config::ReadConfig()
{
    reader = make_shared<INIReader>("config.ini");
}

const char* Config::Get(const char* section, const char* name, const char* def)
{
    return reader->Get(section, name, def);
}

i32 Config::GetInt(const char* section, const char* name, i32 def)
{
    return reader->GetInt(section, name, def);
}

double Config::GetDouble(const char* section, const char* name, double def)
{
    return reader->GetDouble(section, name, def);
}

bool Config::GetBoolean(const char* section, const char* name, bool def)
{
    return reader->GetBoolean(section, name, def);
}
