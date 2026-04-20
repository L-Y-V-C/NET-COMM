/* Server code in C */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <iostream>
#include <thread>
#include <map>
#include <string>

#include "json.hpp"

using json = nlohmann::json;
using namespace std;

map<string, int> clients;

void processMessage(int ConnectFD);
void sendFunction(int ConnectFD);
void receiveFunction(int ConnectFD);
void prepareMessage(int op, char *buffer);
void convertIntToString(int byteSize, int num, char *str);

void sendFunction(int ConnectFD)
{
	char buffer[256];
	for(;;)
	{
		bzero(buffer, 256);
		fgets(buffer, 256, stdin);
		write(ConnectFD, buffer, strlen(buffer));
	}
}

void receiveFunction(int ConnectFD)
{
	for(;;)
	{
		processMessage(ConnectFD);

		/*
		n = read(ConnectFD, buffer, 255);

		if(n <= 0)
		{
			close(ConnectFD);
			break;
		}
		*/
	}
}

void prepareMessage(int op, char *buffer, char (*data)[256])
{
	char msg[256], msgSizeStr[20], tmpSizeStr[20];
	int msgSize, tmpSize, offset;
	int byteSize[] = {0, 5, 3, 7, 5};
	int byteSizeAux[] = {0, 0, 7, 5};

	offset = 1;
	bzero(msg, 256);

	switch(op)
	{
		// notify correct login
		case 0:
		{
			buffer[0] = 'K';
			buffer[1] = '\0';
			break;
		}
		// notify error
		case 1:
		{
			buffer[0] = 'E';

			strcpy(msg, "ERROR FROM SERVER!");
			msgSize = strlen(msg);
			convertIntToString(byteSize[op], msgSize, msgSizeStr);

			memcpy(buffer + offset, msgSizeStr, byteSize[op]);
			offset += byteSize[op];
			memcpy(buffer + offset, msg, msgSize);

			break;
		}
		// send broadcast
		case 2:
		{
			buffer[0] = 'b';

			tmpSize = strlen(data[0]);
			convertIntToString(byteSize[op], tmpSize, tmpSizeStr);

			memcpy(buffer + offset, tmpSizeStr, byteSize[op]);
			offset += byteSize[op];
			memcpy(buffer + offset, data[0], tmpSize);
			offset += tmpSize;

			tmpSize = strlen(data[1]);
			convertIntToString(byteSizeAux[op], tmpSize, tmpSizeStr);

			memcpy(buffer + offset, tmpSizeStr, byteSizeAux[op]);
			offset += byteSizeAux[op];
			memcpy(buffer + offset, data[1], tmpSize);

			break;
		}
		// send unicast
		case 3:
		{
			buffer[0] = 'u';

			tmpSize = strlen(data[0]);
			convertIntToString(byteSize[op], tmpSize, tmpSizeStr);

			memcpy(buffer + offset, tmpSizeStr, byteSize[op]);
			offset += byteSize[op];
			memcpy(buffer + offset, data[0], tmpSize);
			offset += tmpSize;

			tmpSize = strlen(data[1]);
			convertIntToString(byteSizeAux[op], tmpSize, tmpSizeStr);

			memcpy(buffer + offset, tmpSizeStr, byteSizeAux[op]);
			offset += byteSizeAux[op];
			memcpy(buffer + offset, data[1], tmpSize);

			break;
		}
		// send client list
		case 4:
		{
			buffer[0] = 't';

			tmpSize = strlen(data[0]);
			convertIntToString(byteSize[op], tmpSize, tmpSizeStr);

			memcpy(buffer + offset, tmpSizeStr, byteSize[op]);
			offset += byteSize[op];
			memcpy(buffer + offset, data[0], tmpSize);

			break;
		}
		// send file
		case 5:
		{
			buffer[0] = 'f';

			break;
		}
		default:
			break;
	}
}

void convertIntToString(int byteSize, int num, char *str)
{
	int power10 = 1;

	for(int i = 1; i < byteSize; i++)
		power10 *= 10;

	for(int i = 0; i < byteSize; i++)
	{
		int digit = num / power10;
		str[i] = '0' + digit;
		num %= power10;
		power10 /= 10;
	}
}

void processMessage(int ConnectFD)
{
	char buffer[256], data[3][256], msgType;
	int n, op, tmpSize;
	int byteSize[] = {4, 7, 5, 5};
	int byteSizeAux[] = {0, 0, 7};

	bzero(buffer, 256);

	n = read(ConnectFD, buffer, 1);
	//buffer[n] = '\0';

	msgType = buffer[0];

	switch(msgType)
	{
		// request Login
		case 'L':
		{
			op = 0;
			n = read(ConnectFD, buffer, byteSize[op]);
			buffer[n] = '\0';
			tmpSize = atoi(buffer);

			n = read(ConnectFD, buffer, tmpSize);
			buffer[n] = '\0';
			strcpy(data[0], buffer);

			printf("\nuser: %s\n", data[0]);

			if(clients.find(data[0]) != clients.end())
			{
				prepareMessage(1, buffer, data);
			}
			else
			{
				clients[data[0]] = ConnectFD;
				prepareMessage(0, buffer, data);
			}
			write(ConnectFD, buffer, strlen(buffer));
			//printf("\nmsg: %s\n", buffer);

			break;
		}
		// request Logout
		case 'O':
		{
			for(auto it = clients.begin(); it != clients.end(); it++)
			{
				if(it->second == ConnectFD)
				{
					clients.erase(it);
					break;
				}
			}
			shutdown(ConnectFD, SHUT_RDWR);
			close(ConnectFD);

			break;
		}
		// request broadcast msg
		case 'B':
		{
			int found;
			found = 0;

			op = 1;

			n = read(ConnectFD, buffer, byteSize[op]);
			buffer[n] = '\0';
			tmpSize = atoi(buffer);

			n = read(ConnectFD, buffer, tmpSize);
			buffer[n] = '\0';
			strcpy(data[1], buffer);

			for(auto c : clients)
			{
				if(c.second == ConnectFD)
				{
					found = 1;
					strcpy(data[0], c.first.c_str());
					prepareMessage(2, buffer, data);
				}
			}

			if(!found)
			{
				prepareMessage(1, buffer, data);
				write(ConnectFD, buffer, strlen(buffer));
				break;
			}

			//printf("\nmsg: %s\n", buffer);
			for(auto c : clients)
			{
				write(c.second, buffer, strlen(buffer));
			}

			break;
		}
		// request unicast msg
		case 'U':
		{
			op = 2;

			n = read(ConnectFD, buffer, byteSize[op]);
			buffer[n] = '\0';
			tmpSize = atoi(buffer);

			n = read(ConnectFD, buffer, tmpSize);
			buffer[n] = '\0';
			strcpy(data[1], buffer);

			n = read(ConnectFD, buffer, byteSizeAux[op]);
			buffer[n] = '\0';
			tmpSize = atoi(buffer);

			n = read(ConnectFD, buffer, tmpSize);
			buffer[n] = '\0';
			strcpy(data[2], buffer);

			for(auto c : clients)
			{
				if(c.second == ConnectFD)
				{
					strcpy(data[0], c.first.c_str());
				}
			}

			string nickname = data[2];

			if(auto search = clients.find(nickname); search != clients.end())
			{
				prepareMessage(3, buffer, data);
				//printf("\nmsg: %s\n", buffer);
				write(search->second, buffer, strlen(buffer));
			}
			else
			{
				// TODO: ask the professor
				printf("\nERROR: user not found\n");
			}

			break;
		}
		// who is in the server?
		case 'T':
		{
			json clientListJson;

			for(auto c : clients)
			{
				clientListJson["users"].push_back(c.first);
			}

			string clientListStr = clientListJson.dump();
			strcpy(data[0], clientListStr.c_str());

			prepareMessage(4, buffer, data);

			//printf("\nmsg: %s\n", buffer);
			write(ConnectFD, buffer, strlen(buffer));

			break;
		}
		// request file transfer
		case 'F':
		{
			op = 3;

			char rwBuffer[9999], fileData[9999];
			int fileSize = 0;

			bzero(rwBuffer, 9999);

			n = read(ConnectFD, rwBuffer, byteSize[op]);
			rwBuffer[n] = '\0';
			tmpSize = atoi(rwBuffer);

			while (fileSize < tmpSize)
			{
				n = read(ConnectFD, fileData + fileSize, tmpSize - fileSize);
				fileSize += n;
			}

			n = read(ConnectFD, rwBuffer, byteSize[op]);
			rwBuffer[n] = '\0';
			tmpSize = atoi(rwBuffer);

			n = read(ConnectFD, rwBuffer, tmpSize);
			rwBuffer[n] = '\0';
			strcpy(data[0], rwBuffer);

			n = read(ConnectFD, rwBuffer, byteSize[op]);
			rwBuffer[n] = '\0';
			tmpSize = atoi(rwBuffer);

			n = read(ConnectFD, rwBuffer, tmpSize);
			rwBuffer[n] = '\0';
			strcpy(data[2], rwBuffer);

			for(auto c : clients)
			{
				if(c.second == ConnectFD)
				{
					strcpy(data[1], c.first.c_str());
				}
			}

			string nickname = data[2];

			auto it = clients.find(nickname);

			if(it == clients.end())
			{
				// TODO: ask the professor
				printf("\nERROR: user not found\n");
				break;
			}

			int TargetFD = it->second;

			// fileSize - fileData - size data[0] - data[0] - size data[1] - data[1]

			bzero(rwBuffer, 9999);

			rwBuffer[0] = 'f';

			char tmpSizeStr[20];
			int offset = 1;

			convertIntToString(byteSize[op], fileSize, tmpSizeStr);

			memcpy(rwBuffer + offset, tmpSizeStr, byteSize[op]);
			offset += byteSize[op];
			memcpy(rwBuffer + offset, fileData, fileSize);
			offset += fileSize;

			tmpSize = strlen(data[0]);
			convertIntToString(byteSize[op], tmpSize, tmpSizeStr);

			memcpy(rwBuffer + offset, tmpSizeStr, byteSize[op]);
			offset += byteSize[op];
			memcpy(rwBuffer + offset, data[0], tmpSize);
			offset += tmpSize;

			tmpSize = strlen(data[1]);
			convertIntToString(byteSize[op], tmpSize, tmpSizeStr);

			memcpy(rwBuffer + offset, tmpSizeStr, byteSize[op]);
			offset += byteSize[op];
			memcpy(rwBuffer + offset, data[1], tmpSize);

			//cout<<"\nmsg:\n"<<rwBuffer<<endl;
			write(TargetFD, rwBuffer, strlen(rwBuffer));

			break;
		}
		default:
			break;
	}

	//buffer[strcspn(buffer, "\n")] = 0;
}

int main(void)
{
	struct sockaddr_in stSockAddr;
	int SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	if(-1 == SocketFD)
	{
		perror("can not create socket");
		exit(EXIT_FAILURE);
	}

	memset(&stSockAddr, 0, sizeof(struct sockaddr_in));

	stSockAddr.sin_family = AF_INET;
	stSockAddr.sin_port = htons(45000);
	stSockAddr.sin_addr.s_addr = INADDR_ANY;

	if(-1 == bind(SocketFD,(const struct sockaddr *)&stSockAddr, sizeof(struct sockaddr_in)))
	{
		perror("error bind failed");
		close(SocketFD);
		exit(EXIT_FAILURE);
	}

	if(-1 == listen(SocketFD, 10))
	{
		perror("error listen failed");
		close(SocketFD);
		exit(EXIT_FAILURE);
	}

	for(;;)
	{
		int ConnectFD = accept(SocketFD, NULL, NULL);

		//thread(sendFunction, ConnectFD).detach();
		thread(receiveFunction, ConnectFD).detach();

		//shutdown(ConnectFD, SHUT_RDWR);
		//close(ConnectFD);
	}

	close(SocketFD);

	return 0;
}
