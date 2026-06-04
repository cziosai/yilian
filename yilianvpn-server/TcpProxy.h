#ifndef TCPPROXY_H
#define TCPPROXY_H
#include <ctime>
#include "CommonMethods.h"
#include "Config.h"
#include "Socket.h"
#include "Task.h"
#include "IPHeader.h"
#include "TCPHeader.h"
#include "Proxy.h"

class DataList
{
public:
	DataList(char *data, int size)
	{
		this->data = data;
		this->size = size;
	}
	~DataList()
	{
		size = 0;
		delete[] data;
	}
	char *data;
	int size;
};

class TcpProxy: public Proxy
{
public:
	TcpProxy()
	{
	}

	TcpProxy(long clientId, Socket *clientSocket, char *packet) : Proxy(clientId, clientSocket)
	{
		protocol = IPHeader::TCP;
		dataBuffer = buffer + Proxy::TCP_HEADER_SIZE;
		IPHeader oldIPHeader = IPHeader(packet, 0);
		int ipHeaderLen = oldIPHeader.getHeaderLength();
		int tos = oldIPHeader.getTos();
		srcIp = oldIPHeader.getSourceIP();
		destIp = oldIPHeader.getDestinationIP();
		TCPHeader oldTCPHeader = TCPHeader(packet, ipHeaderLen);
		srcPort = oldTCPHeader.getSourcePort();
		destPort = oldTCPHeader.getDestinationPort();

		IPHeader ipHeader = IPHeader(buffer, 0);
		TCPHeader tcpHeader = TCPHeader(buffer, IPHeader::IP4_HEADER_SIZE);
		ipHeader.setHeaderLength(IPHeader::IP4_HEADER_SIZE);
		ipHeader.setTos(tos);
		ipHeader.setIdentification(0);
		ipHeader.setFlagsAndOffset(0);
		ipHeader.setTTL(32);
		ipHeader.setProtocol(IPHeader::TCP);
		ipHeader.setSourceIP(destIp);
		ipHeader.setDestinationIP(srcIp);

		tcpHeader.setSourcePort(destPort);
		tcpHeader.setDestinationPort(srcPort);
		tcpHeader.setHeaderLength(TCPHeader::TCP_HEADER_SIZE);
		tcpHeader.setFlag(0);
		tcpHeader.setWindow(65535);
		tcpHeader.setUrp(0);

		state0 = TCP_CONNECTING;
		state1 = TCP_SYN_WAIT;
		errorRetry = ERROR_RETRY_NUM;
		connected = false;
		myWindow = 65535;
		clientWindow = 65535;
		mFlags = 0;
	}

	~TcpProxy()
	{
		if(!closed)
		{
			close();
		}
	}

	void close(std::string msg = "close done")
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
		if(destSocket.isClose()) 
		{
			close();
			return 0;
		}
		int res = size;
		if(connected)
		{
			res = sendData(destSocket, bytes, 0, size);
		}
		else
		{
			char *data = new char[size];
			CommonMethods::arraycopy(bytes, 0, data, 0, size);
			DataList *list = new DataList(data, size);
			dataList.push_back(list);
		}

		if(res < size)
		{
			perror("[TcpProxy]socket error msg");
			printf("[TcpProxy](%s) send data to server fail, total %d bytes, success send %d bytes.\n", toString().c_str(), size, res);
			close();
		}
		return res;
	}

	/*
	 * 发送数据给客户端
	 */
	int sendToClient(char *bytes, int size)
	{
		if(clientSocket->isClose()) 
		{
			close();
			return 0;
		}
		int res = sendData(*clientSocket, bytes, 0, size);
		if(res < size)
		{
			perror("[TcpProxy]socket error msg");
			printf("[TcpProxy](%s) send data to client fail, total %d bytes, success send %d bytes.\n", toString().c_str(), size, res);
			close();
			clientSocket->iClose();
		}
		return res;
	}

	/*
	 * 处理第一个包，建立远程连接
	 */
	void processFisrtPacket(char *packet, int size)
	{
		mIpHeader = IPHeader(packet, 0);
		mTcpHeader = TCPHeader(packet, mIpHeader.getHeaderLength());
		mFlags = mTcpHeader.getFlag();

		destSocket = Socket(destIp, destPort, Socket::TCP);
		if(!destSocket.isClose())
		{
			processPacket(packet, size);
		}
		else
		{
			printf("[TcpProxy](%s) socket create fail.\n", toString().c_str());
			close("socket create fail");
		}
	}
	
	/*
	 * 处理数据包
	 */
	void processPacket(char *packet, int size)
	{
		mIpHeader = IPHeader(packet, 0);
		mTcpHeader = TCPHeader(packet, mIpHeader.getHeaderLength());
		clientWindow = mTcpHeader.getWindow();
		mFlags = mTcpHeader.getFlag();
		
		switch (state0)
		{
		case TCP_CONNECTING:
			processConnecting();
			break;
		case TCP_CONNECTED:
			processConnected();
			break;
		case TCP_DISCONNECT:
			processDisconnect();
			break;
		default:
			printf("[TcpProxy](%s) process packet but state0 abnormal, state0=%d.\n", toString().c_str(), state0);
			break;
		}
	}
	
	void processConnecting()
	{
		switch (state1)
		{
		case TCP_SYN_WAIT:
			processSYNWAITPacket();
			break;
		case TCP_SYN_WAIT_ACK:
			processSYNWAITACKPacket();
			break;
		default:
			printf("[TcpProxy](%s) process connecting packet but state1 abnormal, state1=%d.\n", toString().c_str(), state1);
			break;
		}	
	}
	
	void processConnected()
	{
		switch(mFlags)
		{
		case TCPHeader::ACK:
			processESTABLISHEDACKPacket();
			break;
		case (TCPHeader::ACK | TCPHeader::FIN):
			processESTABLISHEDACKPacket();
			processFINPacket();
			break;
		case (TCPHeader::ACK | TCPHeader::RST):
			processESTABLISHEDACKPacket();
			processRSTPacket();
			break;
		case (TCPHeader::ACK | TCPHeader::PSH):
			processPSHPacket();
			processESTABLISHEDACKPacket();
			break;
		case (TCPHeader::ACK | TCPHeader::PSH | TCPHeader::FIN):
			processPSHPacket();
			processESTABLISHEDACKPacket();
			processFINPacket();
			break;
		case (TCPHeader::ACK | TCPHeader::PSH | TCPHeader::URG):
			processPSHPacket();
			processURGPacket();
			processESTABLISHEDACKPacket();
			break;
		case (TCPHeader::ACK | TCPHeader::FIN | TCPHeader::PSH | TCPHeader::URG):
			processPSHPacket();
			processURGPacket();
			processESTABLISHEDACKPacket();
			processFINPacket();
			break;
		case TCPHeader::RST:
			processRSTPacket();
			break;
		case TCPHeader::PSH:
			processPSHPacket();
			break;
		case TCPHeader::URG:
			processURGPacket();
			break;
		case TCPHeader::SYN:
			processSYNWAITPacket();
			break;
		default :
			printf("[TcpProxy](%s), connected packet flags %d, program unable to process.\n", toString().c_str(), mFlags);
		}
		
	}
	
	void processDisconnect()
	{
		switch (state1)
		{

		case TCP_CLOSE_WAIT:
			processCLOSEWAITACKPacket();
			break;
		case TCP_LAST_ACK:
			processLASTACKPacket();
			break;
		case TCP_CLOSED:
			printf("[TcpProxy](%s) process disconnect packet but state1 closed, state1=%d.\n", toString().c_str(), state1);
			break;
		default:
			printf("[TcpProxy](%s) process disconnect packet but state1 abnormal, state1=%d.\n", toString().c_str(), state1);
			break;
		}
	}

	/*
	 * 处理syn数据包
	 */
	void processSYNWAITPacket()
	{
		if((mFlags & 0xFF) == TCPHeader::SYN)
		{
			mySeq = 0;
			clientSeq = mTcpHeader.getSeqID() + 1;
			updateTCPBuffer((TCPHeader::SYN | TCPHeader::ACK), 0);
			sendToClient(buffer, Proxy::TCP_HEADER_SIZE);
			mySeq += 1;
			state1 = TCP_SYN_WAIT_ACK;
			errorRetry = ERROR_RETRY_NUM;
		}
		else
		{
			errorRetry--;
			if(errorRetry < 0)
			{
				// printf("[TcpProxy](%s) recvive client first packet(%s) is not syn, flags is %d.\n", toString().c_str(), mIpHeader.toString().c_str(), mFlags);
				close("close by fist code is not syn");	
			}
		}
	}
	
	/*
	 * 处理syn_wait数据包
	 */
	void processSYNWAITACKPacket()
	{
		if (mTcpHeader.getSeqID() == clientSeq && mTcpHeader.getAckID() == mySeq)
		{
			state0 = TCP_CONNECTED;
			state1 = TCP_ESTABLISHED;
			errorRetry = ERROR_RETRY_NUM;
			processESTABLISHEDACKPacket();//万一有数据，进行处理 
		}
		else
		{
			errorRetry--;
			if(errorRetry < 0)
			{
				printf("[TcpProxy](%s) SYN_WAIT_ACK fail.\n", toString().c_str());
				close("syn wait check ack fail");
			}
		}
	}

	/*
	 * 处理建立连接数据包
	 */
	void processESTABLISHEDACKPacket()
	{
		unsigned int seq = mTcpHeader.getSeqID();
		if (seq == clientSeq)
		{
			//printf("[TcpProxy](%s) recvive client seq queue number match.\n", toString().c_str());
			int headerLength = mIpHeader.getHeaderLength() + mTcpHeader.getHeaderLength();
			int dataSize = mIpHeader.getTotalLength() - headerLength;
			if(dataSize > 0)
			{
				sendToServer(mIpHeader.mData + headerLength, dataSize);
				// 下一个序列号
				clientSeq += dataSize;
				// 发送数据收到ACK包
				updateTCPBuffer(TCPHeader::ACK, 0);
				sendToClient(buffer, Proxy::TCP_HEADER_SIZE);
			}
		}
	}

	/*
	 * 处理数rst据包
	 */
	void processRSTPacket()
	{
		state0 = TCP_CONNECTING;
		state1 = TCP_SYN_WAIT;
		errorRetry = ERROR_RETRY_NUM;
	}

	/*
	 * 处理urg数据包
	 */
	void processURGPacket()
	{
		
	}

	/*
	 * 处理psh数据包
	 */
	void processPSHPacket()
	{
		
	}

	/*
	 * 处理fin数据包
	 */
	void processFINPacket()
	{
		// printf("[TcpProxy](%s) start closeing by client, state1=%d.\n", toString().c_str(), state1);
		quit();
		destSocket.iClose();
		updateTCPBuffer(TCPHeader::ACK, 0);
		sendToClient(buffer, Proxy::TCP_HEADER_SIZE);
		state0 = TCP_DISCONNECT;
		state1 = TCP_CLOSE_WAIT;
		processCLOSEWAITACKPacket();
	}

	/*
	 * 处理close_wait数据包
	 */
	void processCLOSEWAITACKPacket()
	{
		updateTCPBuffer((TCPHeader::FIN | TCPHeader::ACK), 0);
		sendToClient(buffer, Proxy::TCP_HEADER_SIZE);
		state1 = TCP_LAST_ACK;
	}

	/*
	 * 处理last_ack数据包
	 */
	void processLASTACKPacket()
	{
		unsigned int ack = mTcpHeader.getAckID() - 1;
		unsigned int seq = mTcpHeader.getSeqID() - 1;
		if (ack == mySeq && seq == clientSeq)
		{
			// printf("[TcpProxy](%s) LAST_ACK confirm success, seq %u:%u, ack %u:%u, close success.\n", toString().c_str(), seq, clientSeq, ack, mySeq);
			state1 = TCP_CLOSED;
			// 关闭完成, 释放资源
			close();
		}
		else
		{
			//printf("[TcpProxy](%s) LAST_ACK confirm fail, queue number mismatched, seq %u:%u, ack %u:%u, close fail.\n", toString().c_str(), seq, clientSeq, ack, mySeq);
			errorRetry--;
			if(errorRetry < 0)
			{
				close("close by last ack fail");
			}
		}
	}

	/*
	 * 建立连接
	 */
	bool connect()
	{
		if((std::time(NULL) - createTime) > Config::TCP_CONNECT_TIMEOUT)
		{
			// printf("[TcpProxy](%s) connection init timeout, socket errorno %d.\n", toString().c_str(), errno);
			close("connection init timeout");
			return true;
		}
		
		int ret = false;
		int fd = destSocket.getFd();
		fd_set writefds;
		struct timeval timeout = {0, 0};

		// 初始化读集合
		FD_ZERO(&writefds);
		// 添加文件描述符到读集合
		FD_SET(fd, &writefds);
		// 调用 select() 监视读集合 int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
		select(fd + 1, NULL, &writefds, NULL, &timeout);

		// 检查文件描述符是否准备好读取
		if (FD_ISSET(fd, &writefds))
		{
			// 文件描述符准备好了，可以进行写操作
			// 数据即可发送给服务器
			for (int i = 0; i < dataList.size(); i++)
			{
				DataList *data = dataList[0];
				int res = destSocket.dataSend(data->data, data->size);
				if(res == 0)
				{
					printf("[TcpProxy](%s) send data error(remote server disconnected), size: %d-%d.\n", toString().c_str(), data->size, res);
					close("remote server disconnected");
					ret = true;
					break;

				}
				else if(res == data->size)
				{
					dataList.erase(dataList.begin());
					delete data;
					i--;
				}
				else if(res != -1)
				{
					printf("[TcpProxy](%s) send data error, size: %d-%d.\n", toString().c_str(), data->size, res);
					close("connection init error");
					ret = true;
					break;
				}
			}
			if(ret == false)
			{
				connected = true;
			}
		}
		// 从集合中删除文件描述符
		FD_CLR(fd, &writefds);
		
		return ret;
	} 
	
	bool clearJob()
	{
		for (int i = 0; i < dataList.size(); i++)
		{
			DataList *data = dataList[0];
			dataList.erase(dataList.begin());
			delete data;
			i--;
		}
		return false;
	}

	bool loop()
	{
		if(destSocket.isClose()) 
		{
			close();
			return true;
		}
		bool ret = false;
		
		// 与服务器未建立连接
		if(!connected)
		{
			ret = connect();
			if(ret)
			{
				return ret;
			}
		}
		
		//客户端缓存已满 ，不读取 
		if(clientWindow <= 0) 
		{
			return false;
		}
		
		int maxDataLen = clientWindow < Proxy::TCP_BUFFER_SIZE ? clientWindow : Proxy::TCP_BUFFER_SIZE;
		// 从服务器接收数据
		int size = destSocket.dataRecv(dataBuffer, maxDataLen);
		if (size > 0)
		{
			updateTCPBuffer(TCPHeader::ACK, size);
			// 转发给客户端
			sendToClient(buffer, Proxy::TCP_HEADER_SIZE + size);
			mySeq += size;
		}
		else if(size == 0)
		{
			close("remote server disconnected");
			return true;
		}
		else
		{
			this->tick = Config::TASK_TICK; 
		}
		return ret;
	}

	/*
	 * 过期判断
	 */
	bool isExpire()
	{
		time_t now = std::time(NULL);
		return (now - clientSocket->lastClientRefreshTime) > Config::TCP_PROXY_EXPIRE_TIME || (now - clientSocket->lastServerRefreshTime) > Config::TCP_PROXY_EXPIRE_TIME;
	}

	/*
	 * 数据包是否为同一个套接字
	 */
	bool equal(char *packet)
	{
		IPHeader ipHeader = IPHeader(packet, 0);
		TCPHeader tcpHeader = TCPHeader(packet, ipHeader.getHeaderLength());
		return protocol == ipHeader.getProtocol() && srcIp == ipHeader.getSourceIP() && srcPort == tcpHeader.getSourcePort() && destIp == ipHeader.getDestinationIP() && destPort == tcpHeader.getDestinationPort();
	}

	/*
	 * 更新TCP头部
	 */
	void updateTCPBuffer(char flag, int dataSize)
	{
		identification++;
		IPHeader ipHeader = IPHeader(buffer, 0);
		TCPHeader tcpHeader = TCPHeader(buffer, ipHeader.getHeaderLength());
		ipHeader.setTotalLength(Proxy::TCP_HEADER_SIZE + dataSize);
		ipHeader.setIdentification(identification);
		tcpHeader.setFlag(flag);
		tcpHeader.setSeqID(mySeq);
		tcpHeader.setAckID(clientSeq);
		tcpHeader.setWindow(myWindow);
		tcpHeader.ComputeTCPChecksum(ipHeader);
	}

private:
	// 我的数据队列号
	unsigned int mySeq;
	// 客户端数据队列号
	unsigned int clientSeq;
	// 我的滑动窗口
	unsigned short myWindow;
	// 客户端的滑动窗口
	unsigned short clientWindow;
	// IP/TCP通信状态0
	short state0;
	// IP/TCP通信状态1
	short state1;
	short errorRetry;
	static const int ERROR_RETRY_NUM = 3;
	static const int TCP_CONNECTING = 1;
	static const int TCP_CONNECTED = 2;
	static const int TCP_DISCONNECT = 3;
	static const int TCP_SYN_WAIT = 4;
	static const int TCP_SYN_WAIT_ACK = 5;
	static const int TCP_ESTABLISHED = 6;
	static const int TCP_CLOSE_WAIT = 7;
	static const int TCP_LAST_ACK = 8;
	static const int TCP_CLOSED = 9;
	// 与服务器建立连接
	bool connected;
	// 未连接暂存数据
	std::vector<DataList *> dataList;
	IPHeader mIpHeader;
	TCPHeader mTcpHeader;
	short mFlags;
};

#endif

