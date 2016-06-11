
#include "Config.h"
#include <memory>
#include <fstream>
#include <stdlib.h>
using namespace std;

// the only thing that needs to be hardcoded
static const char* CONFIG_FILENAME = "config.ini";

Config::Config(Client& c) : Module(c)
{
    LoadSettings(CONFIG_FILENAME);
}

static void trimLeadingWhitespace(string* s)
{
    int eraseSpaces = 0;
    int y;
    int len = (int)s->length();

    for (y = 0; y < len; ++y)
    {
        char c = (*s)[y];
        if (c != ' ' && c != '\t')
        {
            break;
        }

        eraseSpaces++;
    }

    if (eraseSpaces != 0)  // we have trailing spaces
    {
        s->erase(0, eraseSpaces);
    }
}

static void trimComments(char commentChar, char escapeChar, string* l)
{
    string result = "";
    bool lastEscape = false;
    int len = (int)l->length();
    int x;

    for (x = 0; x < len; ++x)
    {
        if ((*l)[x] == commentChar)
        {
            if (lastEscape)
            {
                lastEscape = false;
                result += commentChar;
            }
            else
                break;
        }
        else if ((*l)[x] == escapeChar)
        {
            if (lastEscape)
            {
                result += escapeChar;
                lastEscape = false;
            }
            else
            {
                lastEscape = true;
            }
        }
        else
        {
            if (lastEscape)
                result += "\\";

            result += (*l)[x];
            lastEscape = false;
        }
    }

    if (lastEscape)
        result += "\\";

    *l = result;
}

// remove carriage returns from a string
static void removeCr(string* s)
{
    string newString;

    for (int x = 0; x < (int)s->length(); ++x)
    {
        if ((*s)[x] != '\r')
            newString += (*s)[x];
    }

    *s = newString;
}

static void trimTrailingWhitespace(string* s)
{
    int eraseSpaces = 0;
    int y;
    int len = (int)s->length();

    for (y = len - 1; y >= 0; --y)
    {
        char c = (*s)[y];
        if (c != ' ' && c != '\t')
        {
            y++;  // fix offset
            break;
        }

        eraseSpaces++;
    }

    if (eraseSpaces != 0)  // we have trailing spaces
    {
        s->erase(y, eraseSpaces);
    }
}

static void trimWhitespace(string* s)
{
    trimLeadingWhitespace(s);
    trimTrailingWhitespace(s);
}

static void toLower(string* s)
{
    int len = (int)s->length();
    for (int x = 0; x < len; ++x)
    {
        (*s)[x] = tolower((*s)[x]);
    }
}

void Config::LoadSettings(const char* path)
{
    ifstream fin(path);
    string line;
    string category = "<none>";

    if (!fin)
    {
        fprintf(stderr, "Error Loading Config File '%s'", path);
        // I guess we can continue... it will just use default settings
    }
    else
    {
        while (!fin.eof())
        {
            getline(fin, line);

            trimComments(';', '\\', &line);
            trimLeadingWhitespace(&line);
            removeCr(&line);

            if (line.length() > 0)
            {  // if we have an actual line

                trimTrailingWhitespace(&line);

                // handle []'s
                if (line[0] == '[')
                {
                    line = line.substr(1);
                    trimComments(']', 0, &line);
                    trimWhitespace(&line);
                    toLower(&line);

                    category = line;
                }
                else
                {
                    // handle ::'s
                    string::size_type pos = line.find("::");

                    if (pos != string::npos)
                    {  // has a ::
                        string cat = line.substr(0, pos);
                        string sett = line.substr(pos + 2);
                        // now we need to split setting into setting=value

                        pos = sett.find("=");

                        if (pos != string::npos)
                        {
                            string value = sett.substr(pos + 1);
                            sett = sett.substr(0, pos);

                            trimWhitespace(&cat);
                            trimWhitespace(&sett);
                            trimWhitespace(&value);

                            toLower(&cat);
                            toLower(&sett);

                            settingsMap[cat][sett] = value;
                        }
                        else
                        {
                            string err = "Line contains '::' but no '=', line = ";
                            err += line;
                            fprintf(stderr, "%s", err.c_str());
                        }
                    }
                    else
                    {  // handle regular setting=value
                        pos = line.find("=");

                        if (pos != string::npos)
                        {
                            string value = line.substr(pos + 1);
                            line = line.substr(0, pos);

                            trimWhitespace(&line);
                            trimWhitespace(&value);

                            toLower(&line);

                            settingsMap[category][line] = value;
                        }
                        else
                        {
                            string err = "Line in config file does not contain '=', line = ";
                            err += line;
                            fprintf(stderr, "%s", err.c_str());
                        }
                    }
                }
            }
        }

        fin.close();
    }
}

const char* Config::GetString(const char* section, const char* name, const char* def)
{
    const char* rv = def;
    const char* val = GetStringNoDefault(section, name);

    if (val == nullptr && c.log)
        c.log->LogInfo("String setting %s::%s not found; defaulting to '%s'", section, name, def);
    else
        rv = val;

    return rv;
}

const char* Config::GetString(const char* section, const char* name, const char* def,
                              bool (*IsAllowed)(const char*))
{
    const char* val = GetString(section, name, def);

    if (!IsAllowed(val))
    {
        c.log->LogError("Invalid value for setting %s::%s='%s'. Using default of '%s'.", section,
                        name, val, def);
        val = def;
    }

    return val;
}

i32 Config::GetInt(const char* section, const char* name, i32 def)
{
    i32 rv = def;
    const char* val = GetStringNoDefault(section, name);

    if (val == nullptr)
    {
        c.log->LogInfo("Int setting %s::%s not found; defaulting to %d", section, name, def);
    }
    else
    {
        char* end;
        i32 i = strtol(val, &end, 0);

        if (*end == '\0')
            rv = i;
        else
            c.log->LogError("Malformed int setting %s::%s='%s'; defaulting to %d", section, name,
                            val, def);
    }

    return rv;
}

i32 Config::GetInt(const char* section, const char* name, i32 def, bool (*IsAllowed)(i32))
{
    i32 val = GetInt(section, name, def);

    if (!IsAllowed(val))
    {
        c.log->LogError("Invalid value for setting %s::%s=%i. Using default of %i.", section, name,
                        val, def);
        val = def;
    }

    return val;
}

// may return nullptr
const char* Config::GetStringNoDefault(const char* sectionCstr, const char* nameCstr)
{
    const char* rv = nullptr;

    string section = sectionCstr;
    toLower(&section);

    string name = nameCstr;
    toLower(&name);

    auto sec = settingsMap.find(section);

    if (sec != settingsMap.end())
    {
        auto val = sec->second.find(name);

        if (val != sec->second.end())
            rv = val->second.c_str();
    }

    return rv;
}

double Config::GetDouble(const char* section, const char* name, double def)
{
    double rv = def;
    const char* val = GetStringNoDefault(section, name);

    if (val == nullptr)
        c.log->LogInfo("Double setting %s::%s not found; defaulting to %f", section, name, def);
    else
    {
        char* end;
        double d = strtod(val, &end);

        if (*end == '\0')
            rv = d;
        else
            c.log->LogError("Malformed double setting %s::%s='%s'; defaulting to %f", section, name,
                            val, def);
    }

    return rv;
}

double Config::GetDouble(const char* section, const char* name, double def,
                         bool (*IsAllowed)(double))
{
    double val = GetDouble(section, name, def);

    if (!IsAllowed(val))
    {
        c.log->LogError("Invalid value for setting %s::%s=%i. Using default of %f.", section, name,
                        val, def);
        val = def;
    }

    return val;
}
