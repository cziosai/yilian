#ifndef UDPPROXY_H
#define UDPPROXY_H
#include <ctime>
#include "Config.h"
#include "Socket.h"
#include "Task.h"
#include "IPHeader.h"
#include "UDPHeader.h"
#include "Proxy.h"


class UdpProxy: public Proxy
{
public:
	UdpProxy()
	{
	}

	UdpProxy(long clientId, Socket *clientSocket, char *packet) : Proxy(clientId, clientSocket)
	{
		protocol = IPHeader::UDP;
		dataBuffer = buffer + Proxy::UDP_HEADER_SIZE;
		IPHeader oldIPHeader = IPHeader(packet, 0);
		srcIp = oldIPHeader.getSourceIP();
		destIp = oldIPHeader.getDestinationIP();
		UDPHeader oldUDPHeader = UDPHeader(packet, oldIPHeader.getHeaderLength());
		srcPort = oldUDPHeader.getSourcePort();
		destPort = oldUDPHeader.getDestinationPort();

		IPHeader ipHeader = IPHeader(buffer, 0);
		ipHeader.setVersion(IPHeader::V4_VERSION);
		ipHeader.setHeaderLength(IPHeader::IP4_HEADER_SIZE);
		ipHeader.setTos(0);
		ipHeader.setIdentification(0);
		ipHeader.setFlagsAndOffset(0);
		ipHeader.setTTL(32);
		ipHeader.setProtocol(IPHeader::UDP);
		ipHeader.setSourceIP(destIp);
		ipHeader.setDestinationIP(srcIp);

		UDPHeader udpHeader = UDPHeader(buffer, IPHeader::IP4_HEADER_SIZE);
		udpHeader.setSourcePort(destPort);
		udpHeader.setDestinationPort(srcPort);
	}

	~UdpProxy()
	{
		if(!closed)
		{
			close();
		}
	}

	void close(const std::string msg = "")
	{
		quit();
		closed = true;
		destSocket.iClose();
	}

	bool isClose()
	{
		return closed;
	}

	/*
	 * 发送数据给服务器
	 */
	int sendToServer(char *bytes, int size)
	{
		int res = sendData(destSocket, bytes, 0, size);
		if(res < size)
		{
			perror("[UdpProxy]socket error msg");
			printf("[UdpProxy](%s) send data to server fail, total %d bytes, success send %d bytes.\n", toString().c_str(), size, res);
			close();
		}
		return res;
	}

	/*
	 * 发送数据给客户端
	 */
	int sendToClient(char *bytes, int size)
	{
		int res = sendData(*clientSocket, bytes, 0, size);
		if(res < size)
		{
			perror("[UdpProxy]socket error msg");
			printf("[UdpProxy](%s) send data to client fail, total %d bytes, success send %d bytes.\n", toString().c_str(), size, res);
			close();
			clientSocket->iClose();
		}
		return res;
	}

	/*
	 * 处理第一个包，建立远程连接
	 */
	void processFirstPacket(char *packet, int size)
	{
		destSocket = Socket(destIp, destPort, Socket::UDP);
		if(!destSocket.isClose())
		{
			processPacket(packet, size);
		}
		else
		{
			close("socket create fail");
			printf("[UdpProxy](%s) socket create fail.\n", toString().c_str());
		}
	}

	/*
	 * 处理数据包
	 */
	void processPacket(char *packet, int size)
	{
		IPHeader ipHeader = IPHeader(packet, 0);
		int headerLen = ipHeader.getHeaderLength() + UDPHeader::UDP_HEADER_SIZE;
		int dataSize = ipHeader.getTotalLength() - headerLen;
		if(dataSize > 0)
		{
			// 转发给服务器
			sendToServer(packet + headerLen, dataSize);
		}
	}

	bool loop()
	{
		// 从服务器接收数据
		int size = destSocket.dataRecv(dataBuffer, Proxy::UDP_BUFFER_SIZE);
		if (size > 0)
		{
			updateUDPBuffer(size);
			// 转发给客户端
			sendToClient(buffer, Proxy::UDP_HEADER_SIZE + size);
		}
		else if(size == 0)
		{
			close();
			return true;
		}
		else
		{
			this->tick = Config::TASK_TICK; 
		}
		return false;
	}

	/*
	 * 过期判断
	 */
	bool isExpire()
	{
		time_t now = std::time(NULL);
		return (now - destSocket.lastClientRefreshTime) > Config::UDP_PROXY_EXPIRE_TIME || (now - destSocket.lastServerRefreshTime) > Config::UDP_PROXY_EXPIRE_TIME;
	}

	/*
	 * 数据包是否为同一个套接字
	 */
	bool equal(char *packet)
	{
		IPHeader ipHeader = IPHeader(packet, 0);
		UDPHeader udpHeader = UDPHeader(packet, ipHeader.getHeaderLength());
		return protocol == ipHeader.getProtocol() && srcIp == ipHeader.getSourceIP() && srcPort == udpHeader.getSourcePort() && destIp == ipHeader.getDestinationIP() && destPort == udpHeader.getDestinationPort();
	}

	/*
	 * 更新UDP头部
	 */
	void updateUDPBuffer(int size)
	{
		identification++;
		IPHeader ipHeader = IPHeader(buffer, 0);
		UDPHeader udpHeader = UDPHeader(buffer, ipHeader.getHeaderLength());
		ipHeader.setTotalLength(Proxy::UDP_HEADER_SIZE + size);
		ipHeader.setIdentification(identification);
		udpHeader.setTotalLength(UDPHeader::UDP_HEADER_SIZE + size);
		udpHeader.ComputeUDPChecksum(ipHeader);
	}
};

#endif

