#define NOMINMAX
#include <winsock2.h>
#include <Mswsock.h>
#include "windows.h"
#include "string"
#include "vector"
#include "map"
#include "iostream"
#include "fstream"
#include "kf/kf_log.h"
#include "network.h"
#include "game.h"
#include "kf/kf_memBlock.h"
#include "playermanager.h"
#include "player.h"

namespace Network
{
	namespace
	{
		//std::map<unsigned int, Player *> g_players;
		//int g_playerIDCount = 0;
		std::map<Connection, ClientInfo> g_clients;
		unsigned short g_port = 1300;
		struct sockaddr_in g_localAddress, g_fromAddress;
		SOCKET g_receiveSocket;
	}


	void init()
	{
		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		{
			kf_log("WSAStartup failed.");
			WSACleanup();
			return;
		}

		g_localAddress.sin_family = AF_INET;
		g_localAddress.sin_addr.s_addr = INADDR_ANY;
		g_localAddress.sin_port = htons(g_port);

		g_receiveSocket = socket(AF_INET, SOCK_DGRAM, 0);
		if (g_receiveSocket < 0)
		{
			kf_log2(0, "Error Opening socket: Error " << WSAGetLastError());
			WSACleanup();
			return;
		}
		if (bind(g_receiveSocket, (SOCKADDR*)&g_localAddress, sizeof(g_localAddress)) == SOCKET_ERROR)
		{
			kf_log2(0, "bind() failed: Error " << WSAGetLastError());
			closesocket(g_receiveSocket);
			return;
		}

		DWORD dwBytesReturned = 0;
		BOOL bNewBehavior = FALSE;
		// disable an annoying feature of 2000 and XP which causes many problems
		WSAIoctl(g_receiveSocket, SIO_UDP_CONNRESET, &bNewBehavior, sizeof(bNewBehavior), NULL, 0, &dwBytesReturned, NULL, NULL);
	}

	void send(const Connection &dest, kf::MemBlock &mb, int size)
	{
		sendto(g_receiveSocket, (const char *)mb.begin(), size, 0, (SOCKADDR*)&g_fromAddress, sizeof(g_fromAddress));
	}
	void sendReliable(Connection &dest, kf::MemBlock &mb, int size)
	{
		auto it = g_clients.find(dest);
		if (it != g_clients.end())
		{
			mb.seek(1);
			mb.setU32(it->second.m_reliablePacketID);
			sendto(g_receiveSocket, (const char *)mb.begin(), size, 0, (SOCKADDR*)&g_fromAddress, sizeof(g_fromAddress));
			ReliablePacket rp;
			rp.m_destination = dest;
			rp.m_id = it->second.m_reliablePacketID++;
			char *data = new char[size];
			memcpy(data, mb.begin(), size);
			rp.m_mb.setRange((kf::u8*)data, size);
			it->second.m_reliablePackets.push_back(rp);
		}
	}

	void poll()
	{
		char buffer[10000];
		int waiting;

		for (auto it = g_clients.begin(); it != g_clients.end(); ++it)
		{
			for (int i = 0; i < it->second.m_reliablePackets.size(); ++i)
			{
				send(it->first, it->second.m_reliablePackets[i].m_mb, it->second.m_reliablePackets[i].m_mb.size());
			}

		}

		do
		{
			fd_set checksockets;
			checksockets.fd_count = 1;
			checksockets.fd_array[0] = g_receiveSocket;
			struct timeval t;
			t.tv_sec = 0;
			t.tv_usec = 0;
			waiting = select(NULL, &checksockets, NULL, NULL, &t);
			if (waiting > 0)
			{
				int length = sizeof(g_fromAddress);
				int result = recvfrom(g_receiveSocket, buffer, 10000, 0, (SOCKADDR*)&g_fromAddress, &length);
				if (result == SOCKET_ERROR)
				{
					kf_log("recvfrom() returned error " << WSAGetLastError());
				}
				else
				{
					kf_log("Packet from: "
						<< (int)g_fromAddress.sin_addr.S_un.S_un_b.s_b1 << "."
						<< (int)g_fromAddress.sin_addr.S_un.S_un_b.s_b2 << "."
						<< (int)g_fromAddress.sin_addr.S_un.S_un_b.s_b3 << "."
						<< (int)g_fromAddress.sin_addr.S_un.S_un_b.s_b4 << ":"
						<< ntohs(g_fromAddress.sin_port) << "  Size=" << result);

					kf::MemBlock mb((kf::u8*)buffer, 10000);
					kf::u8 type = mb.getU8();

					bool multi = true;
					while (multi && (mb.index() < result))
					{
						switch (type)
						{
						case PT_ClientAnnounce:
						{
							std::string name = mb.getString(16);
							if (g_clients.find(g_fromAddress) == g_clients.end())
							{
								kf_log("Client called " << name << " has announced");
								ClientInfo ci;
								ci.m_player = PlayerManager::addPlayer();
								ci.m_name = name;
								ci.m_id = ci.m_player->m_id;
								ci.m_timeout = 0;
							}
							else
							{
								kf_log("Existing Client called " << name << " is trying to join again");
							}

							break;
						}
						default:
						{
							kf_log2(1, "   Unknown");
							multi = false;
							break;
						}
						}
					}
					//	drawRectangle(image, p->x, p->y, p->w, p->h, p->r, p->g, p->b);
				}
			}
		} while (waiting);
	}

}