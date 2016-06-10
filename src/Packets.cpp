#include "Packets.h"
#include "Net.h"
#include <map>
#include <set>
using namespace std;

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

    map<string, PacketTemplate> incomingOrCoreNameToTemplateMap;
    map<string, PacketTemplate> outgoingNameToTemplateMap;

    map<u16, map<u16, string>> incomingPacketIdToLengthToNameMap;  // second one is a length map

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

    PacketType GetPacketType(const char* templateName, bool isOutgoing)
    {
        PacketTemplate* t = GetTemplate(templateName, isOutgoing);

        PacketType rv;

        rv.first = t->isCore;
        rv.second = t->type;

        return rv;
    }

    void PopulatePacketInstance(PacketInstance* store, u8* data, int len)
    {
        const char* templateName = nullptr;

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
                                store->SetValue(te->name.c_str(), data[curByte]);
                            else if (te->length == 2)
                                store->SetValue(te->name.c_str(), GetU16(data + curByte));
                            else if (te->length == 4)
                                store->SetValue(te->name.c_str(), GetU32(data + curByte));

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

                            store->SetValue(te->name.c_str(), value.c_str());
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

                            store->SetValue(te->name.c_str(), value.c_str());
                        }
                        else if (te->type == FT_RAW)
                        {
                            vector<u8> bytes;

                            for (; curByte < len; ++curByte)
                                bytes.push_back(data[curByte]);

                            store->SetValue(te->name.c_str(), &bytes);
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
                        store->templateName = templateName;
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
    PacketTemplate* GetTemplate(const char* templateName, bool outgoingPacket)
    {
        PacketTemplate* rv = nullptr;
        map<string, PacketTemplate>::iterator i =
            incomingOrCoreNameToTemplateMap.find(templateName);

        bool found = (i != incomingOrCoreNameToTemplateMap.end());

        if (!found && outgoingPacket)
        {
            i = outgoingNameToTemplateMap.find(templateName);

            found = (i != outgoingNameToTemplateMap.end());
        }

        if (!found)
        {  // load it
            const char* cat = "Packet Templates";
            char buf[256];
            u16 len = 1;

            snprintf(buf, sizeof(buf), "%s Field Count", templateName);
            i32 count = c.cfg->GetInt(cat, buf, -1);

            snprintf(buf, sizeof(buf), "%s Type", templateName);
            i32 packetId = c.cfg->GetInt(cat, buf, -1);

            bool core = false;

            snprintf(buf, sizeof(buf), "%s iscore", templateName);
            core = c.cfg->GetInt(cat, buf, 0) != 0;

            if (core)
                ++len;

            if (packetId < 1 || packetId > 255)
                c.log->LogError(
                    "%s::%s Type is not a valid setting (is the packet defined in the conf?)", cat,
                    templateName);
            else if (count < 0)
                c.log->LogError("%s::%s Field Count is not a valid setting", cat, templateName);
            else
            {
                PacketTemplate pt;

                // for each field
                int x;
                for (x = 0; x < count; ++x)
                {
                    snprintf(buf, sizeof(buf), "%s Field %i Name", templateName, x);
                    const char* fieldName = c.cfg->GetStringNoDefault(cat, buf);

                    if (fieldName == nullptr)
                    {
                        c.log->LogError("%s::%s is invalid or undefined", cat, buf);
                        break;
                    }

                    snprintf(buf, sizeof(buf), "%s Field %i Type", templateName, x);
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
                        snprintf(buf, sizeof(buf), "%s Field %i Length", templateName, x);
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
                        AddIncomingPacketId(core ? packetId : packetId << 8, len, templateName);

                        incomingOrCoreNameToTemplateMap[templateName] = pt;
                        rv = &incomingOrCoreNameToTemplateMap[templateName];
                    }
                    else
                    {
                        outgoingNameToTemplateMap[templateName] = pt;
                        rv = &outgoingNameToTemplateMap[templateName];
                    }
                }
            }
        }
        else
            rv = &i->second;

        if (rv == nullptr)
            c.log->FatalError("GetTemplate() could not load packet template for '%s'",
                              templateName);

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

    // check to see packet does not contain any extra fields, and that the length is valid
    bool CheckPacketAgainstTemplate(PacketInstance* packet, PacketTemplate* pt, bool reliable)
    {
        bool rv = true;

        // check strings and ntstrings
        for (map<string, string>::iterator i = packet->cStrValues.begin();
             i != packet->cStrValues.end(); ++i)
        {
            bool found = false;

            for (int x = 0; x < (int)pt->chunks.size(); ++x)
            {
                if (pt->chunks[x].name == i->first &&
                    (pt->chunks[x].type == FT_STRING || pt->chunks[x].type == FT_NTSTRING))
                {
                    if (pt->chunks[x].type == FT_STRING)  // do a length check
                    {
                        int len = (int)i->second.length();

                        if (len >= pt->chunks[x].length)
                        {
                            c.log->LogError(
                                "Packet Template string type '%s' (%s) is longer than "
                                "permitted by the packet(%i >= %i)",
                                i->first.c_str(), i->second.c_str(), len, pt->chunks[x].length);

                            i->second = i->second.substr(0, pt->chunks[x].length - 1);
                        }
                    }

                    found = true;
                    break;
                }
            }

            if (!found)
            {
                c.log->LogError(
                    "Packet Template string type '%s'=%s does not exist in the packet template",
                    i->first.c_str(), i->second.c_str());
            }
        }

        // check ints
        for (map<string, int>::iterator i = packet->intValues.begin(); i != packet->intValues.end();
             ++i)
        {
            bool found = false;

            for (int x = 0; x < (int)pt->chunks.size(); ++x)
            {
                if (pt->chunks[x].name == i->first && pt->chunks[x].type == FT_INT)
                {
                    u32 max = 0xFFFFFFFF;

                    if (pt->chunks[x].length == 1)
                        max = 0xFF;
                    else if (pt->chunks[x].length == 2)
                        max = 0xFFFF;

                    u32 val = (u32)i->second;

                    // negative values may be valid
                    if (i->second > 0 && val > max)
                    {
                        c.log->LogError(
                            "Packed template int '%s'(%i bytes) is out of bounds (%i > %i)",
                            i->first.c_str(), pt->chunks[x].length, val, max);

                        val = max;
                    }

                    found = true;
                    break;
                }
            }

            if (!found)
            {
                c.log->LogError("core = %i\n", pt->isCore ? 1 : 0);

                for (int x = 0; x < (int)pt->chunks.size(); ++x)
                {
                    TemplateEntry te = pt->chunks[x];

                    c.log->LogError("entry %i = '%s'\n", x, te.name.c_str());
                }

                c.log->LogError(
                    "Packet Template int type '%s'=%i does not exist in the packet template "
                    "type 0x%04x. dumped packet template",
                    i->first.c_str(), i->second, pt->type);
            }
        }

        return rv;
    }

    bool CheckPacket(PacketInstance* pi, bool reliable)
    {
        bool rv = false;
        PacketTemplate* pt = GetTemplate(pi->templateName.c_str(), true);

        if (pt)  // error will be logged if this is null in getTemplate
        {
            // check to make sure there are no extra fields and that the length is valid
            if (CheckPacketAgainstTemplate(pi, pt, reliable))
                rv = true;
        }

        return rv;
    }

    void PacketTemplateToRaw(PacketInstance* pi, bool reliable, vector<u8>* data)
    {
        PacketTemplate* pt = GetTemplate(pi->templateName.c_str(), reliable);
        int len = GetPacketLength(pi, pt, reliable);
        i32 cur = 0;
        int checksumOffset = -1;  // automagically generates 1 byte checksums for packets if they
                                  // have a "checksum" field
        data->resize(len);

        // handle reliable packets
        if (reliable)
        {
            (*data)[cur++] = CORE_HEADER;
            (*data)[cur++] = RELIABLE_HEADER;

            // put in a filler reliable id (correct one will replace this later)
            PutU32(&((*data)[0]) + cur, -1);
            cur += 4;
        }

        // header
        if (pt->isCore)
        {
            (*data)[cur++] = CORE_HEADER;
        }

        (*data)[cur++] = pt->type;

        // header is complete, now populate the fields
        for (vector<TemplateEntry>::iterator i = pt->chunks.begin(); i != pt->chunks.end(); ++i)
        {
            TemplateEntry* te = &(*i);
            string* fieldName = &(i->name);

            if (*fieldName == "checksum" && i->length == 1 && te->type == FT_INT)
            {
                checksumOffset = cur;
                (*data)[cur++] = 0;
            }
            else if (te->type == FT_INT)
            {
                int val = pi->GetValue(fieldName->c_str());

                if (i->length == 1)
                    (*data)[cur] = val;
                else if (i->length == 2)
                    PutU16(&((*data)[0]) + cur, val);
                else if (i->length == 4)
                    PutU32(&((*data)[0]) + cur, val);

                cur += i->length;
            }
            else if (te->type == FT_STRING)
            {
                pi->GetValue(fieldName->c_str(), (char*)(&((*data)[0]) + cur), i->length);

                cur += i->length;
            }
            else if (te->type == FT_NTSTRING)
            {
                string* val = &(pi->cStrValues[fieldName->c_str()]);

                for (int x = 0; x < (int)val->length(); ++x)
                {
                    (*data)[cur++] = (*val)[x];
                }

                (*data)[cur++] = '\0';  // null terminated!
            }
            else if (te->type == FT_RAW)
            {
                int rawlen;
                const u8* rawdata = pi->GetValue(fieldName->c_str(), &rawlen);

                if (rawdata != nullptr)
                {
                    for (int x = 0; x < rawlen; ++x)
                        (*data)[cur++] = rawdata[x];
                }
            }
        }

        if (cur != len)
        {
            c.log->LogError(
                "Packet length doesn't match calculated length when converting from template to "
                "raw");
        }

        // automatically generate 1-byte checksum if we found such a field
        if (checksumOffset >= 0)
        {
            u8 ck = 0;
            int start = reliable ? 4 : 0;

            for (int i = start; i < len; ++i)
                ck ^= (*data)[i];

            (*data)[checksumOffset] = ck;
        }
    }
};

Packets::Packets(Client& c) : Module(c), data(make_shared<PacketsData>(c))
{
}

Packets::~Packets()
{
}

PacketType Packets::GetPacketType(const char* templateName, bool isOutgoing)
{
    return data->GetPacketType(templateName, isOutgoing);
}

void Packets::PopulatePacketInstance(PacketInstance* store, u8* bytes, int len)
{
    return data->PopulatePacketInstance(store, bytes, len);
}

bool Packets::CheckPacket(PacketInstance* pi, bool reliable)
{
    return data->CheckPacket(pi, reliable);
}

void Packets::PacketTemplateToRaw(PacketInstance* packet, bool reliable, vector<u8>* rawData)
{
    data->PacketTemplateToRaw(packet, reliable, rawData);
}

int PacketInstance::GetValue(const char* type) const
{
    int rv = 0;

    map<string, int>::const_iterator i = intValues.find(type);

    if (i != intValues.end())
    {
        rv = i->second;
    }
    else
        c.log->LogError(
            "GetPacketInstanceValue for int data was not found! returning 0; type = '%s'", type);

    return rv;
}

void PacketInstance::GetValue(const char* type, char* store, int len) const
{
    map<string, string>::const_iterator i = cStrValues.find(type);

    // pad storage with 0's
    for (int x = 0; x < len; ++x)
        store[x] = 0;

    if (i != cStrValues.end())
        snprintf(store, len, "%s", i->second.c_str());
    else
        c.log->LogError(
            "GetPacketInstanceValue for c_string data was not found! returning empty string; "
            "type = '%s'",
            type);
}

const u8* PacketInstance::GetValue(const char* type, int* rawLen) const
{
    const u8* rv = nullptr;
    *rawLen = 0;

    map<string, vector<u8>>::const_iterator i = rawValues.find(type);

    if (i != rawValues.end())
    {
        rv = &(i->second[0]);  // hmm... assumes vector is of type u8* internally, not ideal
        *rawLen = (int)i->second.size();
    }
    else
        c.log->LogError(
            "GetPacketInstanceValue for raw data was not found! returning null pointer; type = "
            "'%s'",
            type);

    return rv;
}
