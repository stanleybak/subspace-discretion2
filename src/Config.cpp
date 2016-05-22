
#include "Config.h"
#include <memory>
using namespace std;

static const char* CONFIG_FILENAME = "config.ini";

Config::Config(Client& c) : Module(c), reader(CONFIG_FILENAME)
{
}

const char* Config::GetString(const char* section, const char* name, const char* def)
{
    return reader.Get(section, name, def);
}

const char* Config::GetString(const char* section, const char* name, const char* def, bool (*IsAllowed)(const char*))
{
    const char* val = GetString(section, name, def);

    if (!IsAllowed(val))
    {
        c.log->LogError("Invalid value for setting %s::%s='%s'. Using default of '%s'.", section, name, val, def);
        val = def;
    }

    return val;
}

i32 Config::GetInt(const char* section, const char* name, i32 def)
{
    return reader.GetInt(section, name, def);
}

i32 Config::GetInt(const char* section, const char* name, i32 def, bool (*IsAllowed)(i32))
{
    i32 val = GetInt(section, name, def);

    if (!IsAllowed(val))
    {
        c.log->LogError("Invalid value for setting %s::%s=%i. Using default of %i.", section, name, val, def);
        val = def;
    }

    return val;
}

double Config::GetDouble(const char* section, const char* name, double def)
{
    return reader.GetDouble(section, name, def);
}

double Config::GetDouble(const char* section, const char* name, double def, bool (*IsAllowed)(double))
{
    double val = GetDouble(section, name, def);

    if (!IsAllowed(val))
    {
        c.log->LogError("Invalid value for setting %s::%s=%i. Using default of %i.", section, name, val, def);
        val = def;
    }

    return val;
}

bool Config::GetBoolean(const char* section, const char* name, bool def)
{
    return reader.GetBoolean(section, name, def);
}
