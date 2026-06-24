#ifndef CLIENT_H
#define CLIENT_H
#include "Config.h"
#include "Socket.h"
#include "Task.h"
#include "ICMPHeader.h"
#include "Proxy.h"
#include "TcpProxy.h"
#include "UdpProxy.h"

class Client: public Task
{
public:
	// 控制包代码 
	static const char CTRL = 203;
	long clientId;

	Client(Socket socket)
	{
		UID++;
		clientId = UID;
		closed = false;
		loopCount = 1;
		authenticated = false;
		remoteSocket = socket;
		cacheBytesSize = 0;
	}

	~Client()
	{
		if(!closed)
		{
			close();
		}
	}

	void close(int code = 0)
	{
		if(code != 0)
		{
			char msg[IPHeader::IP4_HEADER_SIZE] = {0};
			IPHeader ipheader = IPHeader(msg, 0);
			ipheader.setHeaderLength(IPHeader::IP4_HEADER_SIZE);
			int totalLength = IPHeader::IP4_HEADER_SIZE;
			ipheader.setTotalLength(totalLength);
			ipheader.setProtocol(Client::CTRL);
			ipheader.setFlagsAndOffset(Config::CLIENT_MAX_PROXY);
			ipheader.setTos(100);
			ipheader.setIdentification(code);
			int resTwo = remoteSocket.dataOutPut(msg, totalLength);
			if(resTwo == 0 || (resTwo > 0 && resTwo < totalLength))
			{
				printf("[Client]client(%ld) send closed CTRL packet error.\n", clientId);
			}
		}
		quit();
		closed = true;
		remoteSocket.iClose();
		closeAllProxy();
		printf("[Client]client(%ld) closed.\n", clientId);
	}

	bool isClose()
	{
		return closed;
	}

	/*
	* 接收数据包 建立新代理
	*/
	void processPacketToProxy(char *packet, int size, unsigned char protocol)
	{
		// 清除已关闭代理，防止数据发送给已关闭代理
		clearClosedProxy();
		// 清除长时间未活动代理，减小内存使用，可能会导致网络问题 
		if (proxys.size() > Config::CLIENT_MAX_PROXY)
		{
			int clearNum =  clearExpireProxy();
			if(clearNum > 0) printf("[Client]client(%ld) proxy number max, cleaned up number %d, now proxy number %lu.\n", clientId, clearNum, proxys.size());
		}
		
		// 检查该代理是否创建
		for (int i = 0; i < proxys.size(); i++)
		{
			Proxy *proxy = proxys[i];
			if (proxy->equal(packet))
			{
				proxy->processPacket(packet, size);
				return;
			}
		}

		// 代理没创建，建立新代理
		if(protocol == IPHeader::TCP)
		{
			Proxy *proxy = new TcpProxy(clientId, &remoteSocket, packet);
			proxy->processFirstPacket(packet, size);
			proxys.push_back(proxy);
		}
		else if(protocol == IPHeader::UDP)
		{
			Proxy *proxy = new UdpProxy(clientId, &remoteSocket, packet);
			proxy->processFirstPacket(packet, size);
			proxys.push_back(proxy);
		}
	}

	/*
	 * 清除不活动代理
	 */
	int clearExpireProxy()
	{
		int ret = 0;
		for (int i = 0; i < proxys.size(); i++)
		{
			Proxy *proxy = proxys[i];
			if(proxy->isExpire())
			{
				if(!proxy->isClose())
				{
					proxy->close();
				}
				proxys.erase(proxys.begin() + i);
				i--;
				ret++;
				delete proxy;
			}
		}
		return ret;
	}

	/*
	 * 清除已关闭代理
	 */
	int clearClosedProxy()
	{
		int ret = 0;
		for (int i = 0; i < proxys.size(); i++)
		{
			Proxy *proxy = proxys[i];
			if(proxy->isClose())
			{
				proxys.erase(proxys.begin() + i);
				i--;
				ret++;
				delete proxy;
			}
		}
		return ret;
	}

	/*
	 * 清除所有代理
	 */
	int closeAllProxy()
	{
		int ret = 0;
		for (int i = 0; i < proxys.size(); i++)
		{
			Proxy *proxy = proxys[0];
			if(!proxy->isClose())
			{
				proxy->close();
			}
			proxys.erase(proxys.begin());
			i--;
			ret++;
			delete proxy;
		}
		return ret;
	}

	/*
	 * 获取TCP代理数量 
	 */
	int getTcpProxyNum()
	{
		int ret = 0;
		for (int i = 0; i < proxys.size(); i++)
		{
			Proxy *proxy = proxys[i];
			if(proxy->getProtocol() == IPHeader::TCP)
			{
				ret++;
			}
		}
		return ret;
	}

	/*
	 * 获取UDP代理数量 
	 */
	int getUdpProxyNum()
	{
		int ret = 0;
		for (int i = 0; i < proxys.size(); i++)
		{
			Proxy *proxy = proxys[i];
			if(proxy->getProtocol() == IPHeader::UDP)
			{
				ret++;
			}
		}
		return ret;
	}
	
	int getUpDataBytes()
	{
		return remoteSocket.upDataBytes;
	} 
	
	int getDownDataBytes()
	{
		return remoteSocket.downDataBytes;
	} 

	/*
	 * 处理控制包 
	 */
	int processCTRLPacket(char *packet, int size)
	{
		int res = 0;
		IPHeader revHeader = IPHeader(packet, 0);
		int state =  revHeader.getTos();
		int totalLength = IPHeader::IP4_HEADER_SIZE + 8;
		char msg[totalLength] = {0};
		IPHeader ipheader = IPHeader(msg, 0);
		ipheader.setHeaderLength(IPHeader::IP4_HEADER_SIZE);
		ipheader.setTotalLength(totalLength);
		ipheader.setProtocol(Client::CTRL);
		ipheader.setFlagsAndOffset(Config::CLIENT_MAX_PROXY);
		ipheader.setTos(state);

		if(state == 100)
		{
			// header.getSourceIP() 为用户名
			// header.getDestinationIP() 为密码
			if (revHeader.getSourceIP() == Config::USER_NAME && revHeader.getDestinationIP() == Config::USER_PASSWD)
			{
				authenticated = true;
				printf("[Client]client(%ld) user %u verify success, establish connection.\n", clientId, revHeader.getSourceIP());
				ipheader.setIdentification(200);
			}
			else
			{
				printf("[Client]client(%ld) establish connection verify user name and password fail, closeing.\n", clientId);
				ipheader.setIdentification(403);
				close(401); //401 收到错误包 
				res = -4;
			}
		}
		else if(state == 101)
		{
			ipheader.setSourceIP(getTcpProxyNum());
			ipheader.setDestinationIP(getUdpProxyNum());
			CommonMethods::writeInt(msg, IPHeader::IP4_HEADER_SIZE, getUpDataBytes());
			CommonMethods::writeInt(msg, IPHeader::IP4_HEADER_SIZE + 4, getDownDataBytes());
		}

		int resTwo = remoteSocket.dataOutPut(msg, totalLength);
		if(resTwo == 0 || (resTwo > 0 && resTwo < totalLength))
		{
			printf("[Client]client(%ld) send ctrl data lose connection, closeing.\n", clientId);
			close();
			res = -5;
		}

		return res;
	}
	
	void onIPCMPacketReceived(char* bytes, int offset, int size) {
        IPHeader ipHeader = IPHeader(bytes, offset);
        ICMPHeader icmpHeader = ICMPHeader(bytes, offset + IPHeader::IP4_HEADER_SIZE);
        int srcIp = ipHeader.getSourceIP();
        int destIp = ipHeader.getDestinationIP();
        ipHeader.setSourceIP(destIp);
        ipHeader.setDestinationIP(srcIp);

        bool canReply = true;
        if(icmpHeader.getType() ==  8 && icmpHeader.getCode() == 0){
            icmpHeader.setType((char) 0);
            icmpHeader.setCode((char) 0);
        }else if(icmpHeader.getType() == 13 && icmpHeader.getCode() == 0){
            icmpHeader.setType((char) 14);
            icmpHeader.setCode((char) 0);
        }else if(icmpHeader.getType() == 17 && icmpHeader.getCode() == 0){
            icmpHeader.setType((char) 18);
            icmpHeader.setCode((char) 0);
        }else{
            canReply = false;
        }
        if(canReply){
            icmpHeader.ComputeICMPChecksum(ipHeader);
        	int res = remoteSocket.dataOutPut(bytes + offset, size);
			if(res == 0 || (res > 0 && res < size))
			{
				printf("[Client]client(%ld) send icmp data lose connection, closeing.\n", clientId);
				close();
				res = -6;
			}
        }
    }

	/*
	 * 处理IP数据包 TCP包让tcpProxy处理 UDP包让udpProxy处理 其他包不处理 并关闭客户端
	 */
	int processIPPacket(char *packet, int size)
	{
		IPHeader header = IPHeader(packet, 0);
		char protocol = header.getProtocol();
		
		if(!authenticated)
		{
			
			if(protocol == Client::CTRL)
			{
				return processCTRLPacket(packet, size);
			}
			else
			{
				printf("[Client]client(%ld) login fail, closeing.\n", clientId);
				close(401); //401 收到错误包 
				return -2; //第一个包格式错误	
			}
		}
		else
		{
			if (protocol == IPHeader::TCP)
			{
				processPacketToProxy(packet, size, IPHeader::TCP);
			}
			else if(protocol == IPHeader::UDP)
			{
				processPacketToProxy(packet, size, IPHeader::UDP);
			}
			else if(protocol == IPHeader::ICMP)
			{
				onIPCMPacketReceived(packet, 0, size);
			}
			else if(protocol == Client::CTRL)
			{
				return processCTRLPacket(packet, size);
			}
			else
			{
				printf("[Client]client(%ld) recvive unknown protocol packet, closeing.\n", clientId);
				close(401); //401 收到错误包 
				return -3; //无法处理的IP协议
			}
		}
		return 0;
	}

	/*
	 * 对接收的数据分包
	 */
	int processRecvBytes(char *bytes, int size)
	{
		int ret = 0;
		if (cacheBytesSize > 0)
		{
			memmove(cacheBytes + cacheBytesSize, bytes, size);
			size = cacheBytesSize + size;
			cacheBytesSize = 0;
			ret = processRecvBytes(cacheBytes, size);
			return ret;
		}

		if (size < IPHeader::IP4_HEADER_SIZE)
		{
			memmove(cacheBytes + cacheBytesSize, bytes, size);
			cacheBytesSize += size;
			return 0;
		}

		IPHeader IpHeader = IPHeader(bytes, 0);
		int totalLength = IpHeader.getTotalLength();
		if(totalLength <= 0 || totalLength > Config::MUTE)
		{
			printf("[Client]client(%ld) recvive bad length packet, closeing\n", clientId);
			close(401); //401 收到错误包 
			return -1; //长度非法
		}

		if (totalLength < size)
		{
			ret = processIPPacket(bytes, totalLength);
			if(ret != 0)
			{
				return ret;
			}
			int nextDataSize = size - totalLength;
			ret = processRecvBytes(bytes + totalLength, nextDataSize);
		}
		else if (totalLength == size)
		{
			ret = processIPPacket(bytes, size);
		}
		else
		{
			memmove(cacheBytes + cacheBytesSize, bytes, size);
			cacheBytesSize += size;
		}
		return ret;
	}


	bool loop()
	{
		int size = remoteSocket.dataInPut(buffer, Config::MUTE);
		if (size > 0)
		{
			processRecvBytes(buffer, size);
		}
		else if(size == 0)
		{
			printf("[Client]client(%ld) socket remote closed, closeing.\n", clientId);
			close();
			return true;
		}
		else
		{
			this->tick = Config::TASK_TICK; 
		}
		this->loopCount = proxys.size() + 1;
		return false;
	}

	/*
	 * 过期判断 
	 */
	bool isExpire()
	{
		time_t now = std::time(NULL);
		return (now - remoteSocket.lastClientRefreshTime) > Config::CLIENT_EXPIRE_TIME || (now - remoteSocket.lastClientRefreshTime) > Config::CLIENT_EXPIRE_TIME;
	}

private:
	// 客户端套接字
	Socket remoteSocket;
	// 缓存数据
	char cacheBytes[Config::MUTE * 2];
	// 缓存数据大小
	int cacheBytesSize;
	// 已关闭状态
	bool closed;
	//已登录
	bool authenticated;
	// 接收缓存
	char buffer[Config::MUTE];
	// 代理连接容器
	std::vector<Proxy*> proxys;
	// 客户端ID生成
	static long UID;
};

long Client::UID = 0;

#endif

