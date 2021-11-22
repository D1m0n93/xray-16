#pragma once

#include "Common/GUID.hpp"
#include "xrCore/_fbox.h"

enum fsL_Chunks
{
    fsL_HEADER = 1, //*
    fsL_SHADERS = 2, //*
    fsL_VISUALS = 3, //*
    fsL_PORTALS = 4, //* - Portal polygons
    fsL_LIGHT_DYNAMIC = 6, //*
    fsL_GLOWS = 7, //* - All glows inside level
    fsL_SECTORS = 8, //* - All sectors on level
    fsL_VB = 9, //* - Static geometry
    fsL_IB = 10, //*
    fsL_SWIS = 11, //* - collapse info, usually for trees
    fsL_forcedword = 0xFFFFFFFF
};

enum fsESectorChunks
{
    fsP_Portals = 1, // - portal polygons
    fsP_Root = 2, // - geometry root
    fsP_forcedword = u32(-1)
};

enum fsSLS_Chunks
{
    fsSLS_Description = 1, // Name of level
    fsSLS_ServerState = 2,
    fsSLS_forcedword = u32(-1)
};

enum EBuildQuality
{
    ebqDraft = 0,
    ebqHigh = 1,
    ebqCustom = 2,
    ebq_force_u16 = u16(-1)
};

#pragma pack(push, 8)
struct hdrLEVEL
{
    u16 XRLC_version;
    u16 XRLC_quality;
};

struct hdrCFORM
{
    u32 version;
    u32 vertcount;
    u32 facecount;
    Fbox aabb;
};

struct hdrNODES
{
    u32 version;
    u32 count;
    float size;
    float size_y;
    Fbox aabb;
    xrGUID guid;
};
#pragma pack(pop)

#pragma pack(push, 1)
#pragma pack(1)
#ifndef _EDITOR
class NodePosition6
{
    u8 data[5];

    ICF void xz(u32 value) { CopyMemory(data, &value, 3); }
    ICF void y(u16 value) { CopyMemory(data + 3, &value, 2); }
public:
    // под xz-координаты отводится 3 байта
    static const u32 MAX_XZ = (1 << 24) - 1;
    // под y-координату отводится 2 байта
    static const u32 MAX_Y = (1 << 16) - 1;

    ICF u32 xz() const { return ((*((u32*)data)) & 0x00ffffff); }
    ICF u32 x(u32 row) const { return (xz() / row); }
    ICF u32 z(u32 row) const { return (xz() % row); }
    ICF u32 y() const { return (*((u16*)(data + 3))); }
    friend class CLevelGraph;
    friend struct CNodePositionCompressor;
    friend struct CNodePositionConverter;
};

struct NodeCompressed10
{
public:
    static const u32 NODE_BIT_COUNT = 23;
    static const u32 LINK_MASK = (1 << NODE_BIT_COUNT) - 1;

    u8 data[12];

private:
    ICF void link(u8 link_index, u32 value)
    {
        value &= 0x007fffff;
        switch (link_index)
        {
        case 0:
            value |= *(u32*)data & 0xff800000;
            CopyMemory(data, &value, sizeof(u32));
            break;
        case 1:
            value <<= 7;
            value |= *(u32*)(data + 2) & 0xc000007f;
            CopyMemory(data + 2, &value, sizeof(u32));
            break;
        case 2:
            value <<= 6;
            value |= *(u32*)(data + 5) & 0xe000003f;
            CopyMemory(data + 5, &value, sizeof(u32));
            break;
        case 3:
            value <<= 5;
            value |= *(u32*)(data + 8) & 0xf000001f;
            CopyMemory(data + 8, &value, sizeof(u32));
            break;
        }
    }

    ICF void light(u8 value) { data[10] |= value << 4; }
public:
    struct SCover
    {
        u16 cover0 : 4;
        u16 cover1 : 4;
        u16 cover2 : 4;
        u16 cover3 : 4;

        ICF u16 cover(u8 index) const
        {
            switch (index)
            {
            case 0: return cover0;
            case 1: return cover1;
            case 2: return cover2;
            case 3: return cover3;
            default: NODEFAULT;
            }
#ifdef DEBUG
            return u8(-1);
#endif
        }
    };

    SCover high;
    SCover low;
    u16 plane;
    NodePosition6 p;
    // 32 + 16 + 40 + 92 = 180 bits = 24.5 bytes => 25 bytes

    ICF u32 link(u8 index) const
    {
        switch (index)
        {
        case 0: return ((*(u32*)data) & 0x007fffff);
        case 1: return (((*(u32*)(data + 2)) >> 7) & 0x007fffff);
        case 2: return (((*(u32*)(data + 5)) >> 6) & 0x007fffff);
        case 3: return (((*(u32*)(data + 8)) >> 5) & 0x007fffff);
        default: NODEFAULT;
        }
#ifdef DEBUG
        return (0);
#endif
    }

    friend class CLevelGraph;
    friend struct CNodeCompressed;
    friend class CNodeRenumberer;
    friend class CRenumbererConverter;
};

struct NodeCompressed8
{
    u8 data[12];
    NodeCompressed10::SCover cover;
    u16 plane;
    NodePosition6 p;

    operator NodeCompressed10() const
    {
        NodeCompressed10  node;
        CopyMemory      (node.data, data, sizeof(data) / sizeof(u8));
        node.high   =   cover;
        node.low    =   cover;
        node.plane  =   plane;
        node.p      =   p;
        return node;
    }
};

class NodePosition12
{
    u32 m_xz;
    u16 m_y;

    ICF void xz(u32 value) { m_xz = value; }
    ICF void y(u16 value) { m_y = value; }

public:
    // под xz-координаты отводится 4 байта
    static const u32 MAX_XZ = std::numeric_limits<u32>::max();
    // под y-координату отводится 2 байта
    static const u32 MAX_Y = std::numeric_limits<u16>::max();

    ICF u32 xz() const { return m_xz; }
    ICF u32 x(u32 row) const { return (xz() / row); }
    ICF u32 z(u32 row) const { return (xz() % row); }
    ICF u32 y() const { return m_y; }

    friend class CLevelGraph;
    friend struct CNodePositionCompressor;
    friend struct CNodePositionConverter;
};

struct NodeCompressed12
{
public:
    static const u32 NODE_BIT_COUNT = 25;
    static const u32 LINK_MASK = (1 << NODE_BIT_COUNT) - 1;

    u8 data[13];

private:
    ICF void link(u8 link_index, u32 value)
    {
        value &= LINK_MASK;
        switch (link_index)
        {
        case 0: {
            value |= (*(u32*)data) & ~LINK_MASK;
            CopyMemory(data, &value, sizeof(u32));
            break;
        }
        case 1: {
            value <<= 1;
            value |= (*(u32*)(data + 3)) & ~(LINK_MASK << 1);
            CopyMemory(data + 3, &value, sizeof(u32));
            break;
        }
        case 2: {
            value <<= 2;
            value |= (*(u32*)(data + 6)) & ~(LINK_MASK << 2);
            CopyMemory(data + 6, &value, sizeof(u32));
            break;
        }
        case 3: {
            value <<= 3;
            value |= (*(u32*)(data + 9)) & ~(LINK_MASK << 3);
            CopyMemory(data + 9, &value, sizeof(u32));
            break;
        }
        }
    }

    ICF void light(u8 value) { data[12] = (data[12] & 0x0f) | (value << 4); }

public:
    NodeCompressed10::SCover high;
    NodeCompressed10::SCover low;
    u16 plane;
    NodePosition12 p;
    // 13 + 2 + 2 + 2 + 6 = 25 bytes

    ICF u32 link(u8 index) const
    {
        switch (index)
        {
        case 0: return ((*(u32*)data) & LINK_MASK);
        case 1: return (((*(u32*)(data + 3)) >> 1) & LINK_MASK);
        case 2: return (((*(u32*)(data + 6)) >> 2) & LINK_MASK);
        case 3: return (((*(u32*)(data + 9)) >> 3) & LINK_MASK);
        default: NODEFAULT;
        }
#ifdef DEBUG
        return (0);
#endif
    }

    u8 light() const { return data[12] >> 4; }

    friend class CLevelGraph;
    friend struct CNodeCompressed;
    friend class CNodeRenumberer;
    friend class CRenumbererConverter;
};

#endif

struct SNodePositionOld
{
    s16 x;
    u16 y;
    s16 z;
};
#pragma pack(pop)

#ifdef _EDITOR
typedef SNodePositionOld NodePosition;
#endif

constexpr cpcstr LEVEL_GRAPH_NAME = "level.ai";

const u32 XRCL_CURRENT_VERSION = 18; // input
const u32 XRCL_PRODUCTION_VERSION = 14; // output
const u32 CFORM_CURRENT_VERSION = 4;

enum xrAI_Versions
{
    XRAI_VERSION_SOC = 8,
    XRAI_VERSION_CS = 9,
    XRAI_VERSION_COP = 10,
    XRAI_VERSION_BIG = 12,

    XRAI_VERSION_ALLOWED = XRAI_VERSION_SOC,
    XRAI_VERSION_OPENXRAY = XRAI_VERSION_BIG,

    XRAI_CURRENT_VERSION = XRAI_VERSION_OPENXRAY
};

using NodeCompressed = NodeCompressed12;
using NodePosition = NodePosition12;
const u32 NODE_MAX_COUNT = NodeCompressed::LINK_MASK;
const u32 NODE_MAX_XZ = NodePosition::MAX_XZ;

#define ASSERT_XRAI_VERSION_MATCH(version, description)\
    R_ASSERT2((version) >= XRAI_VERSION_ALLOWED && (version) <= XRAI_CURRENT_VERSION, description);
