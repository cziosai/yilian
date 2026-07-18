#ifndef APP_H
#define APP_H
#include <vector>
#include "Config.h"
#include "Socket.h"
#include "ServerSocket.h"
#include "Client.h"

class App: public Task
{
public:
	App()
	{
		closed = false;
		serverSocket = ServerSocket(0, Config::PORT);
		if(!serverSocket.isClose())
		{
			printf("[App]App mtu=%d.\n", Config::MTU);
			printf("[App]socket bind 0.0.0.0:%d success.\n", Config::PORT);
			printf("[App]use user name %d password %d verify.\n", Config::USER_NAME, Config::USER_PASSWD);
		}
		else
		{
			perror("[App]socket error msg");
			printf("[App]socket bind 0.0.0.0:%d fail.\n", Config::PORT);
			close();
		}
	}

	~App()
	{
		if(!closed)
		{
			close();
		}
	}

	void close()
	{
		quit();
		closed = true;
		serverSocket.iClose();
		closeAllClient();
		printf("[App]App closed.\n");
	}

	bool isClose()
	{
		return closed;
	}
	
	/*
	 * 清除不活动客户端 
	 */
	int clearExpireClient()
	{
		int ret = 0;
		for (int i = 0; i < clients.size(); i++)
		{
			Client *client = clients[i];
			if(client->isExpire())
			{
				if(!client->isClose())
				{
					client->close(400);
				}
				clients.erase(clients.begin() + i);
				i--;
				ret++;
				delete client;
			}
		}	
		return ret;
	}

	/*
	 * 清除已关闭客户端 
	 */
	int clearClosedClient()
	{
		int ret = 0;
		for (int i = 0; i < clients.size(); i++)
		{
			Client *client = clients[i];
			if(client->isClose())
			{
				clients.erase(clients.begin() + i);
				i--;
				ret++;
				delete client;
			}

		}
		return ret;
	}
	
	/*
	 * 清除所有客户端 
	 */
	int closeAllClient()
	{
		int ret = 0;
		for (int i = 0; i < clients.size(); i++)
		{
			Client *client = clients[i];
			if(!client->isClose())
			{
				client->close(400);
			}
			clients.erase(clients.begin());
			i--;
			ret++;
			delete client;
		}
		return ret;
	}
	

	bool loop()
	{
		int socket_fd = serverSocket.getClientSocket();
		if(socket_fd == -1)
		{
			this->asleep = Config::TASK_ASLEEP;
			this->loopCount = 1;
			return false;
		}
		if(socket_fd == 0)
		{
			close();
			perror("[App]socket error msg");
			printf("[App]app server socket error closed.\n");
			return true;
		}
		clearClosedClient();
		clearExpireClient();
		this->loopCount++;
		Socket socket = Socket(socket_fd);
		Client *client = new Client(socket);
		// 是否建立客户端
		if (clients.size() < Config::MAX_CLIENT_NUM)
		{
			clients.push_back(client);
			AppClientNum = clients.size();
			printf("[App]new client(%ld)connecting, total client number %lu.\n", client->clientId, clients.size());
		}
		else
		{
			printf("[App]client connet number reach max, total: %lu, client closeing.\n", clients.size());
			client->close(404);
			delete client;
		}
		return false;
	}

private:
	// 服务器套接字
	ServerSocket serverSocket;
	// 客户端容器
	std::vector<Client*> clients;
	// 已关闭状态
	bool closed;
};

#endif

