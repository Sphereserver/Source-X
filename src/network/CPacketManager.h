/**
* @file CPacketManager.h
* @brief Holds lists of packet handlers
*/

#ifndef _INC_CPACKETMANAGER_H
#define _INC_CPACKETMANAGER_H

#include "../common/common.h"

class CClient;
class Packet;


#define NETWORK_PACKETCOUNT		0x100	// number of unique packets


class PacketManager
{
private:
    Packet* m_handlers[NETWORK_PACKETCOUNT];	// standard packet handlers
    Packet* m_extended[NETWORK_PACKETCOUNT];	// extended packeet handlers (0xbf)
    Packet* m_encoded[NETWORK_PACKETCOUNT];		// encoded packet handlers (0xd7)

public:
    static const char* m_sClassName;
    PacketManager(void);
    virtual ~PacketManager(void);

private:
    PacketManager(const PacketManager& copy);
    PacketManager& operator=(const PacketManager& other);

public:
    void registerStandardPackets(void);	// register standard packet handlers

    void registerPacket(uint id, Packet* handler);		// register packet handler
    void registerExtended(uint id, Packet* handler);	// register extended packet handler
    void registerEncoded(uint id, Packet* handler);		// register encoded packet handler

    void unregisterPacket(uint id);		// remove packet handler
    void unregisterExtended(uint id);	// remove extended packet handler
    void unregisterEncoded(uint id);	// remove encoded packet handler

    Packet* getHandler(uint id) const;			// get handler for packet
    Packet* getExtendedHandler(uint id) const;	// get handler for extended packet
    Packet* getEncodedHandler(uint id) const;	// get handler for encoded packet
};

#endif // _INC_CPACKETMANAGER_H
