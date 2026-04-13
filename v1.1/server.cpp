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
#include <mutex>
#include <map>
#include <string>

using namespace std;

map<string, int> clients;
//std::mutex mtx;

void processMessage(int ConnectFD, char *buffer, char *nickname, char *message);
void sendFunction(int ConnectFD);
void receiveFunction(int ConnectFD);
void prepareMessage(char *buffer, char *nickname, char *message);
void convertIntToString(int num, char *str);

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
	char buffer[256], nickname[256], message[256];
	int n;

	bzero(buffer, 256);
	read(ConnectFD, buffer, 255);
	buffer[strcspn(buffer, "\n")] = 0;
	clients[buffer] = ConnectFD;

	printf("\nuser.- %s\n", buffer);

	for(;;)
	{
		bzero(buffer, 256);
		processMessage(ConnectFD, buffer, nickname, message);

		/*
		n = read(ConnectFD, buffer, 255);

		if(n <= 0)
		{
			close(ConnectFD);
			break;
		}
		*/

		//printf("\nmsg: %s\n", buffer);

		// broadcasting ===============================
		//for(auto c : clients)
		//{
		//	write(c.second, buffer, strlen(buffer));
		//}

		//mtx.lock();
		printf("\ndestination user.- %s\nMessage.- %s\n", nickname, message);
		//mtx.unlock();

		// to selected user ===========================
		string user = nickname;
		bzero(buffer, 256);
		if(auto search = clients.find(user); search != clients.end())
		{
			prepareMessage(buffer, nickname, message);
			write(search->second, buffer, strlen(buffer));
		}
		else
			printf("\nuser not found\n");
	}
}

void prepareMessage(char *buffer, char *nickname, char *message)
{
	char inputSizeStr[256];
	int inputSize, offset = 0;

	inputSize = strlen(nickname) - 1;
	convertIntToString(inputSize, inputSizeStr);

	memcpy(buffer + offset, inputSizeStr, 3);
	offset += 3;
	memcpy(buffer + offset, nickname, inputSize);
	offset += inputSize;

	inputSize = strlen(message) - 1;
	convertIntToString(inputSize, inputSizeStr);

	memcpy(buffer + offset, inputSizeStr, 3);
	offset += 3;
	memcpy(buffer + offset, message, inputSize);
}

void convertIntToString(int num, char *str)
{
	int power10[2] = {100, 10};

	for(int i = 0; i < 2; i++)
	{
		if(num > power10[i])
		{
			str[i] = '0' + (num/power10[i]);
			num = num % power10[i];
		}
		else
		{
			str[i] = '0';
		}
	}
	str[2] = '0' + num;
	str[3] = '\0';
}

void processMessage(int ConnectFD, char *buffer, char *nickname, char *message)
{
	int n, userSize, msgSize;

	n = read(ConnectFD, buffer, 3);
	buffer[n] = '\0';
	userSize = atoi(buffer);

	n = read(ConnectFD, buffer, userSize);
	buffer[n] = '\0';
	strcpy(nickname, buffer);

	n = read(ConnectFD, buffer, 3);
	buffer[n] = '\0';
	msgSize = atoi(buffer);

	n = read(ConnectFD, buffer, msgSize);
	buffer[n] = '\0';
	strcpy(message, buffer);
}

int main(void)
{
	struct sockaddr_in stSockAddr;
	int SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	//char buffer[256], nickname[256], message[256];
	//int n, userSize, msgSize;

	if(-1 == SocketFD)
	{
		perror("can not create socket");
		exit(EXIT_FAILURE);
	}

	memset(&stSockAddr, 0, sizeof(struct sockaddr_in));

	stSockAddr.sin_family = AF_INET;
	stSockAddr.sin_port = htons(8888);
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

		//send.join();
		//receive.join();

		//shutdown(ConnectFD, SHUT_RDWR);
		//close(ConnectFD);
	}


	/*
	for(;;)
	{
		if(0 > ConnectFD)
		{
			perror("error accept failed");
			close(SocketFD);
			exit(EXIT_FAILURE);
		}

		bzero(buffer, 256);

		processMessage(ConnectFD, buffer, nickname, message);

		printf("\nUsername.- %s\nMessage.- %s\n", nickname, message);

		bzero(buffer, 256);
		fgets(buffer, 256, stdin);

		write(ConnectFD, buffer, strlen(buffer));
	}
	*/

	close(SocketFD);

	return 0;
}
