/**
* @file packet.h
* @brief Basic packet sending/receiving support.
*/

#ifndef _INC_PACKET_H
#define _INC_PACKET_H

#include <list>
#include "../common/common.h"
#include "../game/game_macros.h"

#define PACKET_BUFFERDEFAULT 4
#define PACKET_BUFFERGROWTH 4


class NetState;
class SimplePacketTransaction;
class AbstractString;
class CClient;

/***************************************************************************
 *
 *
 *	class Packet				Base packet class for both sending/receiving
 *
 *
 ***************************************************************************/
class Packet
{
protected:
	byte* m_buffer;				// raw data
	uint m_bufferSize;		// size of raw data

	uint m_length;			// length of packet
	uint m_position;			// current position in packet
	uint m_expectedLength;	// expected length of this packet (0 = dynamic)

public:
	explicit Packet(uint size = 0);
	Packet(const Packet& other);
	Packet(const byte* data, uint size);
	virtual ~Packet(void);

private:
	Packet& operator=(const Packet& other);

public:
	bool isValid(void) const;
	uint getLength(void) const; // get total packet length
	uint getPosition(void) const; // get current position
	byte* getData(void) const; // get packet data
	byte* getRemainingData(void) const; // get packet data from current position
	uint getRemainingLength(void) const; // get length of data from current position
	void dump(AbstractString& output) const; // write packet data to string

	void expand(uint size = 0); // expand packet (resize whilst maintaining position)
	void resize(uint newsize); // resize packet
	void seek(uint pos = 0); // seek to position
	void skip(int count = 1); // skip count bytes

	byte &operator[](uint index);
	const byte &operator[](uint index) const;

	// write
	void writeBool(const bool value); // write boolean (1 byte)
	void writeCharASCII(const char value); // write ASCII character (1 byte)
	void writeCharUNICODE(const wchar value); // write UNICODE character (2 bytes)
	void writeCharNUNICODE(const wchar value); // write UNICODE character, network order (2 bytes)
	void writeByte(const byte value); // write 8-bit integer (1 byte)
	void writeInt16(const word value); // write 16-bit integer (2 bytes)
	void writeInt32(const dword value); // write 32-bit integer (4 bytes)
	void writeInt64(const dword hi, const dword lo); // write 64-bit integer (8 bytes)
	void writeInt64(const int64 value); // write 64-bit integer (8 bytes)
	void writeStringASCII(const char* value, bool terminate = true); // write ascii string until null terminator found
	void writeStringASCII(const wchar* value, bool terminate = true); // write ascii string until null terminator found
	void writeStringFixedASCII(const char* value, uint size, bool terminate = false); // write fixed-length ascii string
	void writeStringFixedASCII(const wchar* value, uint size, bool terminate = false); // write fixed-length ascii string
	void writeStringUNICODE(const char* value, bool terminate = true); // write unicode string until null terminator found
	void writeStringUNICODE(const wchar* value, bool terminate = true); // write unicode string until null terminator found
	void writeStringFixedUNICODE(const char* value, uint size, bool terminate = false); // write fixed-length unicode string
	void writeStringFixedUNICODE(const wchar* value, uint size, bool terminate = false); // write fixed-length unicode string
	void writeStringNUNICODE(const char* value, bool terminate = true); // write unicode string until null terminator found, network order
	void writeStringNUNICODE(const wchar* value, bool terminate = true); // write unicode string until null terminator found, network order
	void writeStringFixedNUNICODE(const char* value, uint size, bool terminate = false); // write fixed-length unicode string, network order
	void writeStringFixedNUNICODE(const wchar* value, uint size, bool terminate = false); // write fixed-length unicode string, network order
	void writeData(const byte* buffer, uint size); // write block of data
	void fill(void); // zeroes remaining buffer
	uint sync(void);
	void trim(void); // trim packet length down to current position

	// read
	bool readBool(void); // read boolean (1 byte)
	char readCharASCII(void); // read ASCII character (1 byte)
	wchar readCharUNICODE(void); // read UNICODE character (2 bytes)
	wchar readCharNUNICODE(void); // read UNICODE character, network order (2 bytes)
	byte readByte(void); // read 8-bit integer (1 byte)
	word readInt16(void); // read 16-bit integer (2 bytes)
	dword readInt32(void); // read 32-bit integer (4 bytes)
	int64 readInt64(void); // read 64-bit integer (8 bytes)
	void readStringASCII(char* buffer, uint length, bool includeNull = true); // read fixed-length ascii string
	void readStringASCII(wchar* buffer, uint length, bool includeNull = true); // read fixed-length ascii string
	void readStringUNICODE(char* buffer, uint bufferSize, uint length, bool includeNull = true); // read fixed length unicode string
	void readStringUNICODE(wchar* buffer, uint length, bool includeNull = true); // read fixed length unicode string
	void readStringNUNICODE(char* buffer, uint bufferSize, uint length, bool includeNull = true); // read fixed length unicode string, network order
	void readStringNUNICODE(wchar* buffer, uint length, bool includeNull = true); // read fixed length unicode string, network order
	uint readStringNullASCII(char* buffer, uint maxlength); // read ascii string until null terminator found
	uint readStringNullASCII(wchar* buffer, uint maxlength); // read ascii string until null terminator found
	uint readStringNullUNICODE(char* buffer, uint bufferSize, uint maxlength); // read unicode-string until null terminator found
	uint readStringNullUNICODE(wchar* buffer, uint maxlength); // read unicode-string until null terminator found
	uint readStringNullNUNICODE(char* buffer, uint bufferSize, uint maxlength); // read unicode-string until null terminator found, network order
	uint readStringNullNUNICODE(wchar* buffer, uint maxlength); // read unicode-string until null terminator found, network order

	uint checkLength(NetState* client, Packet* packet);
	virtual uint getExpectedLength(NetState* client, Packet* packet);
	virtual bool onReceive(NetState* client);

protected:
	void clear(void);
	void copy(const Packet& other);
};


/***************************************************************************
 *
 *
 *	class SendPacket			Send-type packet with priority and target client
 *
 *
 ***************************************************************************/
class PacketSend : public Packet
{
public:
	enum Priority
	{
		PRI_IDLE,
		PRI_LOW,
		PRI_NORMAL,
		PRI_HIGH,
		PRI_HIGHEST,
		PRI_QTY
	};

protected:
	int m_priority; // packet priority
	NetState* m_target; // selected network target for this packet
	uint m_lengthPosition; // position of length-byte

public:
	explicit PacketSend(byte id, uint len = 0, Priority priority = PRI_NORMAL);
	PacketSend(const PacketSend* other);
	virtual ~PacketSend() { };

private:
	PacketSend& operator=(const PacketSend& other);

public:
	void initLength(void); // write empty length and ensure that it is remembered

	void target(const CClient* client); // sets person to send packet to

	void send(const CClient* client = nullptr, bool appendTransaction = true); // adds the packet to the send queue
	void push(const CClient* client = nullptr, bool appendTransaction = true); // moves the packet to the send queue (will not be used anywhere else)

	int getPriority() const { return m_priority; }; // get packet priority
	NetState* getTarget() const { return m_target; }; // get target state

	virtual bool onSend(const CClient* client);
	virtual void onSent(CClient* client);
	virtual bool canSendTo(const NetState* client) const;

#ifndef _MTNETWORK
	friend class NetworkOut;
#else
	friend class NetworkOutput;
#endif
	friend class SimplePacketTransaction;

protected:
	void fixLength(); // write correct packet length to it's slot
	virtual PacketSend* clone(void) const;
};


/***************************************************************************
 *
 *
 *	class PacketTransaction		Class for defining data to be sent
 *
 *
 ***************************************************************************/
class PacketTransaction
{
protected:
	PacketTransaction(void) { };

private:
	PacketTransaction(const PacketTransaction& copy);
	PacketTransaction& operator=(const PacketTransaction& other);

public:
	virtual ~PacketTransaction(void) { };

	virtual PacketSend* front(void) = 0; // get first packet in the transaction
	virtual void pop(void) = 0; // remove first packet from the transaction
	virtual bool empty(void) = 0; // check if any packets are available

	virtual NetState* getTarget(void) const = 0; // get target of the transaction
	virtual int getPriority(void) const = 0; // get priority of the transaction
	virtual void setPriority(int priority) = 0; // set priority of the transaction
};


/***************************************************************************
 *
 *
 *	class SimplePacketTransaction		Class for defining a single packet to be sent
 *
 *
 ***************************************************************************/
class SimplePacketTransaction : public PacketTransaction
{
private:
	PacketSend* m_packet;

public:
	explicit SimplePacketTransaction(PacketSend* packet) : m_packet(packet) { };
	~SimplePacketTransaction(void);

private:
	SimplePacketTransaction(const SimplePacketTransaction& copy);
	SimplePacketTransaction& operator=(const SimplePacketTransaction& other);

public:
	NetState* getTarget(void) const { return m_packet->getTarget(); }
	int getPriority(void) const { return m_packet->getPriority(); }
	void setPriority(int priority) { m_packet->m_priority = priority; }

	PacketSend* front(void) { return m_packet; };
	void pop(void) { m_packet = nullptr; }
	bool empty(void) { return m_packet == nullptr; }
};


/***************************************************************************
 *
 *
 *	class ExtendedPacketTransaction		Class for defining a set of packets to be sent together
 *
 *
 ***************************************************************************/
class ExtendedPacketTransaction : public PacketTransaction
{
private:
	std::list<PacketSend*> m_packets;
	NetState* m_target;
	int m_priority;

public:
	ExtendedPacketTransaction(NetState* target, int priority) : m_target(target), m_priority(priority) { };
	~ExtendedPacketTransaction(void);

private:
	ExtendedPacketTransaction(const ExtendedPacketTransaction& copy);
	ExtendedPacketTransaction& operator=(const ExtendedPacketTransaction& other);

public:
	NetState* getTarget(void) const	{ return m_target; }
	int getPriority(void) const { return m_priority; }
	void setPriority(int priority) { m_priority = priority; }

	inline void push_back(PacketSend* packet) { m_packets.push_back(packet); }
    inline void emplace_back(PacketSend* packet) { m_packets.emplace_back(packet); }
    inline PacketSend* front(void) { return m_packets.front(); };
    inline void pop(void) { m_packets.pop_front(); }
    inline bool empty(void) { return m_packets.empty(); }
};



/***************************************************************************
 *
 *
 *	class OpenPacketTransaction		Class to automatically begin and end a transaction
 *
 *
 ***************************************************************************/
class OpenPacketTransaction
{
private:
	NetState* m_client;

public:
	OpenPacketTransaction(const CClient* client, int priority);
	~OpenPacketTransaction(void);

private:
	OpenPacketTransaction(const OpenPacketTransaction& copy);
	OpenPacketTransaction& operator=(const OpenPacketTransaction& other);
};


#endif // _INC_PACKET_H
