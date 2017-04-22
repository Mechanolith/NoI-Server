#pragma once
#ifndef NETWORK_H
#define NETWORK_H
#define NOMINMAX
#include <winsock2.h>
#include <Mswsock.h>
#include "string"
#include "vector"
#include <map>

class Player;

namespace Network
{
	enum PacketTypes
	{
		PT_ClientAnnounce = 0,
		PT_ServerInfo
	};

	class ReliablePacket
	{
	public:
		ReliablePacket()
		{

		}
		Connection m_destination;
		unsigned int m_id;
		kf::MemBlock m_mb;
	};

	class ClientInfo
	{
	public:
		ClientInfo() : m_reliablePacketID(0)
		{}
		std::string m_name;
		time_t m_timeout;
		unsigned int m_id;
		Player *m_player;
		unsigned int m_reliablePacketID;
		std::vector<ReliablePacket> m_reliablePackets;
	};

	class Connection :public sockaddr_in
	{
	public:
		Connection(const sockaddr_in &a)
		{
			sin_addr = a.sin_addr;
			sin_port = a.sin_port;
			sin_family = a.sin_family;
		}
		sockaddr_in get() const
		{
			return *(sockaddr_in*)this;
		}
		bool operator<(const Connection &a) const
		{
			if (sin_addr.S_un.S_addr < a.sin_addr.S_un.S_addr)
				return true;
			else if (sin_addr.S_un.S_addr == a.sin_addr.S_un.S_addr)
			{
				if (sin_port < a.sin_port)
				{
					return true;
				}
			}
			return false;
		}
		bool operator==(const Connection &a) const
		{
			return (sin_addr.S_un.S_addr == a.sin_addr.S_un.S_addr && sin_port == a.sin_port && sin_family == sin_family);
		}
		bool operator!=(const Connection &a) const
		{
			return (sin_addr.S_un.S_addr != a.sin_addr.S_un.S_addr || sin_port != a.sin_port || sin_family != sin_family);
		}
	};
}

#endif
