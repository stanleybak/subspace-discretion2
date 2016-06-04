#include "Packets.h"
#include <map>
#include <set>

enum FieldType
{
    FT_INT,
    FT_STRING,    // string of set length
    FT_NTSTRING,  // null terminated string
    FT_RAW,       // raw data
};

struct TemplateEntry
{
    string name;
    FieldType type;
    int length;
};

struct PacketTemplate
{
    u8 type;
    bool isCore;
    vector<TemplateEntry> chunks;

    PacketTemplate()
    {
        isCore = false;
        type = 0x00;
    }
};

struct PacketsData
{
    PacketsData(Client& c) : c(c) {}

    Client& c;
    u8 CORE_HEADER = 0x00;
    u8 RELIABLE_HEADER = 0x03;

    multimap<string, std::function<void(const PacketInstance*)>> nameToFunctionMap;
    map<u16, map<u16, string>> incomingPacketIdToLengthToNameMap;  // second one is a length map

    map<string, PacketTemplate> incomingOrCoreNameToTemplateMap;
    map<string, PacketTemplate> outgoingNameToTemplateMap;

    // the types of packets our template functions is already listening for
    set<u16> listeningForTypes;
    set<u8> ignoreGamePackets;

    i32 GetPacketLength(PacketInstance* pi, PacketTemplate* pt, bool reliable)
    {
        i32 sum = reliable ? 6 : 0;
        sum += pt->isCore ? 2 : 1;

        for (i32 x = 0; x < (i32)pt->chunks.size(); ++x)
        {
            if (pt->chunks[x].type == FT_NTSTRING)  // null terminated string
            {
                sum += (i32)pi->cStrValues[pt->chunks[x].name].length() +
                       1;  // plus 1 for null terminator
            }
            else if (pt->chunks[x].type == FT_RAW)
            {
                sum += (i32)pi->rawValues[pt->chunks[x].name].size();
            }
            else
                sum += pt->chunks[x].length;
        }

        return sum;
    }

    FieldType FieldTypeStringToFieldType(const char* type)
    {
        FieldType rv = FT_INT;

        if (strcasecmp(type, "int") == 0)
            rv = FT_INT;
        else if (strcasecmp(type, "string") == 0)
            rv = FT_STRING;
        else if (strcasecmp(type, "ntstring") == 0)
            rv = FT_NTSTRING;
        else if (strcasecmp(type, "raw") == 0)
            rv = FT_RAW;
        else
            c.log->LogError("Unknown packet field type: '%s'", type);

        return rv;
    }

    void TemplatePacketRecevied(u8* data, int len)
    {
        // populate the packet instance and call the handlers
        u16 type = data[0] == CORE_HEADER ? (u16)data[1] : ((u16)data[0]) << 8;
        map<u16, map<u16, string>>::iterator j = incomingPacketIdToLengthToNameMap.find(type);

        if (j == incomingPacketIdToLengthToNameMap.end())
            c.log->LogError("Packet received but no known template exists: type 0x%04x", type);
        else
        {
            // map length -> template name
            map<u16, string>::iterator i = j->second.begin();

            // some types don't have a set length (use raw data or ntstring, for example)
            if (i->first != 0)
                i = j->second.find((u16)len);

            if (i == j->second.end())
            {
                c.log->LogError(
                    "Packet Received (0x%04x, len=%i) does not match any template lengths for that "
                    "type; packet dropped. Did you register a handler for that type (and thus "
                    "cause the template to be loaded)?",
                    type, len);

                string msg = "Valid lengths: ";

                for (map<u16, string>::iterator i = j->second.begin(); i != j->second.end(); ++i)
                    msg += to_string(i->first) + " ";

                c.log->LogError("%s", msg.c_str());

                LogPacketError("Invalid Packet Length", data, len);
            }
            else
            {
                PacketTemplate* pt = GetTemplate(i->second.c_str(), false);

                if (!pt)
                    c.log->LogError(
                        "packet type found in packetIdToNameMap, but template doesn't exist? (this "
                        "shouldn't ever happen)");
                else
                {
                    // convert the raw data to a PacketInstance
                    PacketInstance pi;

                    int curByte = pt->isCore ? 2 : 1;  // skip the header
                    bool ok = true;

                    for (int x = 0; x < (int)pt->chunks.size(); ++x)
                    {
                        TemplateEntry* te = &(pt->chunks[x]);

                        if (te->type == FT_INT)
                        {
                            if (len - curByte < te->length)
                            {
                                c.log->LogError(
                                    "Packet Recevied (0x%04x) does not match template %s; packet "
                                    "dropped.",
                                    type, i->second.c_str());

                                LogPacketError("Template Mismatch", data, len);

                                ok = false;
                                break;
                            }

                            if (te->length == 1)
                                pi.setValue(te->name.c_str(), data[curByte]);
                            else if (te->length == 2)
                                pi.setValue(te->name.c_str(), GetU16(data + curByte));
                            else if (te->length == 4)
                                pi.setValue(te->name.c_str(), GetU32(data + curByte));

                            curByte += te->length;
                        }
                        else if (te->type == FT_STRING)
                        {  // cstring

                            if (len - curByte < te->length)
                            {
                                c.log->LogError(
                                    "Packet Recevied (0x%04x) does not match template %s; "
                                    "packet dropped.",
                                    type, i->second.c_str());

                                LogPacketError("TEMPLATE MISMATCH", data, len);

                                ok = false;
                                break;
                            }

                            string value;

                            for (int count = 0; count < te->length; ++count)
                            {
                                if (data[curByte + count] == '\0')
                                    break;

                                if (!isprint(data[curByte + count]))
                                    c.log->LogError(
                                        "non printable character passed in an fixed length string "
                                        "in a %s packet; skipping",
                                        i->second.c_str());
                                else
                                    value += data[curByte + count];
                            }

                            pi.setValue(te->name.c_str(), value.c_str());
                            curByte += te->length;
                        }
                        else if (te->type == FT_NTSTRING)
                        {
                            string value;

                            for (int count = 0; /* incr in loop */; ++count)
                            {
                                if (len - curByte < 1)
                                {
                                    c.log->LogError(
                                        "Packet Received (0x%04x) does not match template %s; "
                                        "packet dropped.",
                                        type, i->second.c_str());

                                    LogPacketError("TEMPLATE MISMATCH", data, len);

                                    ok = false;
                                    break;
                                }

                                char letter = data[curByte++];

                                if (letter == '\0')
                                    break;

                                if (!isprint(letter))
                                    c.log->LogError(
                                        "non printable character passed in an ntstring in a %s "
                                        "packet; skipping",
                                        i->second.c_str());
                                else
                                    value += letter;
                            }

                            if (!ok)  // we errored reading in the string
                                break;

                            pi.setValue(te->name.c_str(), value.c_str());
                        }
                        else if (te->type == FT_RAW)
                        {
                            vector<u8> bytes;

                            for (; curByte < len; ++curByte)
                                bytes.push_back(data[curByte]);

                            pi.setValue(te->name.c_str(), &bytes);
                        }
                    }

                    if (curByte != len)
                    {
                        c.log->LogError(
                            "Packet Received (0x%04x) does not match template %s (extra data "
                            "received); packet dropped.",
                            type, i->second.c_str());

                        LogPacketError("TEMPLATE MISMATCH (too much)", data, len);
                    }
                    else if (ok)
                    {
                        // ok good enough, now call the appropriate functions
                        pair<multimap<string, std::function<void(const PacketInstance*)>>::iterator,
                             multimap<string, std::function<void(const PacketInstance*)>>::iterator>
                            range = nameToFunctionMap.equal_range(i->second);

                        for (; range.first != range.second; ++range.first)
                            range.first->second(&pi);
                    }
                }
            }
        }
    }

    void LogPacketError(const char* header, u8* data, int len)
    {
        string msg = header;
        msg += " (len = " + to_string(len) + " bytes):";
        for (int x = 0; x < len; ++x)
        {
            char buf[8];
            snprintf(buf, sizeof(buf), " %02x", data[x]);
            msg += buf;
        }

        c.log->LogError("%s", msg.c_str());
    }

    // loads it if necessary
    PacketTemplate* GetTemplate(const char* type, bool outgoingPacket)
    {
        PacketTemplate* rv = 0;
        map<string, PacketTemplate>::iterator i = incomingOrCoreNameToTemplateMap.find(type);

        bool found = (i != incomingOrCoreNameToTemplateMap.end());

        if (!found && outgoingPacket)
        {
            i = outgoingNameToTemplateMap.find(type);

            found = (i != outgoingNameToTemplateMap.end());
        }

        if (!found)
        {  // load it
            const char* cat = "Packet Templates";
            char buf[256];
            u16 len = 1;

            snprintf(buf, sizeof(buf), "%s Field Count", type);
            i32 count = c.cfg->GetInt(cat, buf, -1);

            snprintf(buf, sizeof(buf), "%s Type", type);
            i32 packetId = c.cfg->GetInt(cat, buf, -1);

            bool core = false;

            snprintf(buf, sizeof(buf), "%s iscore", type);
            core = c.cfg->GetInt(cat, buf, 0) != 0;

            if (core)
                ++len;

            if (packetId < 1 || packetId > 255)
                c.log->LogError(
                    "%s::%s Type is not a valid setting (is the packet defined in the conf?)", cat,
                    type);
            else if (count < 0)
                c.log->LogError("%s::%s Field Count is not a valid setting", cat, type);
            else
            {
                PacketTemplate pt;

                // for each field
                int x;
                for (x = 0; x < count; ++x)
                {
                    snprintf(buf, sizeof(buf), "%s Field %i Name", type, x);
                    const char* fieldName = c.cfg->GetStringNoDefault(cat, buf);

                    if (fieldName == nullptr)
                    {
                        c.log->LogError("%s::%s is invalid or undefined", cat, buf);
                        break;
                    }

                    snprintf(buf, sizeof(buf), "%s Field %i Type", type, x);
                    const char* fieldType = c.cfg->GetStringNoDefault(cat, buf);

                    if (fieldType == nullptr)
                    {
                        c.log->LogError("%s::%s is invalid", cat, buf);
                        break;
                    }

                    FieldType ft = FieldTypeStringToFieldType(fieldType);  // will report errors
                    i32 fieldLength = 0;

                    if (ft == FT_NTSTRING || ft == FT_RAW)
                    {
                        len = 0;  // these types don't allow for multiple handlers/type

                        if (c.cfg->GetStringNoDefault(cat, buf) != nullptr)
                        {
                            c.log->LogError("%s::%s shoudln't be defined (type is %s)", cat, buf,
                                            fieldType);
                            break;
                        }
                    }
                    else
                    {
                        snprintf(buf, sizeof(buf), "%s Field %i Length", type, x);
                        fieldLength = c.cfg->GetInt(cat, buf, -1);
                        len += fieldLength;

                        if (ft == FT_STRING && fieldLength <= 0)
                        {
                            c.log->LogError("%s::%s is invalid", cat, buf);
                            break;
                        }
                        else if (ft == FT_INT &&
                                 !(fieldLength == 1 || fieldLength == 2 || fieldLength == 4))
                        {
                            c.log->LogError("%s::%s is invalid", cat, buf);
                            break;
                        }
                    }

                    TemplateEntry te;

                    te.name = *fieldName;
                    te.type = ft;
                    te.length = fieldLength;

                    pt.chunks.push_back(te);
                }

                if (x == count)  // there were no errors
                {
                    pt.isCore = core;
                    pt.type = packetId;

                    if (core || !outgoingPacket)
                    {
                        AddIncomingPacketId(core ? packetId : packetId << 8, len, type);

                        incomingOrCoreNameToTemplateMap[type] = pt;
                        rv = &incomingOrCoreNameToTemplateMap[type];
                    }
                    else
                    {
                        outgoingNameToTemplateMap[type] = pt;
                        rv = &outgoingNameToTemplateMap[type];
                    }
                }
            }
        }
        else
        {
            rv = &i->second;
        }

        return rv;
    }

    void AddIncomingPacketId(u16 id, u16 len, const char* name)
    {
        map<u16, map<u16, string>>::iterator i = incomingPacketIdToLengthToNameMap.find(id);

        if (i != incomingPacketIdToLengthToNameMap.end())
        {
            map<u16, string>::iterator j = i->second.find(len);

            if (j != i->second.end())
            {
                c.log->LogError(
                    "Error, id 0x%04x(decimel %i) is used for multiple types of packets"
                    "with the same length=%i? %s and %s",
                    id, id, len, name, incomingPacketIdToLengthToNameMap[id][len].c_str());
            }
            else
                incomingPacketIdToLengthToNameMap[id][len] = name;
        }
        else
            incomingPacketIdToLengthToNameMap[id][len] = name;
    }

    void PutU32(u8* loc, u32 data)
    {
        for (int x = 0; x < 4; ++x)
        {
            loc[x] = data % 256;
            data /= 256;
        }
    }

    void PutU16(u8* loc, u16 data)
    {
        for (int x = 0; x < 2; ++x)
        {
            loc[x] = data % 256;
            data /= 256;
        }
    }

    // extract a little endian u32
    u32 GetU32(u8* loc)
    {
        u32 rv = 0;

        for (int x = 3; x >= 0; --x)
        {
            rv *= 256;
            rv += loc[x];
        }

        return rv;
    }

    u16 GetU16(u8* loc)
    {
        u16 rv = 0;

        for (int x = 1; x >= 0; --x)
        {
            rv *= 256;
            rv += loc[x];
        }

        return rv;
    }
};

Packets::Packets(Client& c) : Module(c), data(make_shared<PacketsData>(c))
{
}

Packets::~Packets()
{
}
