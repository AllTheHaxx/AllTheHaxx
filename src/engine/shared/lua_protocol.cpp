#include "lua_protocol.h"
#include <base/system.h>
#include <base/math.h>
/*
void CLuaProtocol::Init(lua_State *L)
{
    m_pLua = L;
}

bool CLuaProtocol::SendRawPacket(CLuaPacket *pPacket)
{
    if (!pPacket)
        return false;
    if (pPacket->m_Type == LUA_MSG_INVALID || pPacket->m_Size <= 0)
        return false;

    char aBuffer[16]; //type and size
    union
    {
        char *pBuffer;
        unsigned char *pUBuffer;
    };
    pBuffer = aBuffer;
    pUBuffer = CVariableUInt64::Pack(pUBuffer, pPacket->m_Type);
    pUBuffer = CVariableUInt64::Pack(pUBuffer, pPacket->m_Size);


    int HeaderSize = pBuffer - aBuffer;
    m_SendBuffer.Push(aBuffer, HeaderSize);
    m_SendBuffer.Push(pPacket->m_pData, pPacket->m_Size);
    return true;
}

bool CLuaProtocol::ProcessPacket(CLuaPacket *pPacket)
{
    //dint Type;
    //dint Size;
    char aBuffer[16];
    union
    {
        char *pBuffer;
        const unsigned char *pUBuffer;
    };
    m_RecvBuffer.Get(aBuffer, sizeof(aBuffer));
    pBuffer = aBuffer;
    pUBuffer = CVariableUInt64::Unpack(pUBuffer, &pPacket->m_Type);
    pUBuffer = CVariableUInt64::Unpack(pUBuffer, &pPacket->m_Size);

    int HeaderSize = pBuffer - aBuffer;
    if (m_RecvBuffer.Size() >= HeaderSize + pPacket->m_Size)
    {
        m_RecvBuffer.Remove(HeaderSize);
        pPacket->m_pData = new char[pPacket->m_Size];
        m_RecvBuffer.Pop(pPacket->m_pData, pPacket->m_Size);
        return true;
    }
    pPacket->m_Type = LUA_MSG_INVALID;
    pPacket->m_Size = 0;
    return false;
}*/
