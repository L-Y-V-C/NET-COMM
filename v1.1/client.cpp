 /* Client code in C */

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

using namespace std;

//std::mutex mtx;

void convertIntToString(int num, char *str);
void prepareMessage(char *buffer);
void sendFunction(int SocketFD);
void receiveFunction(int SocketFD);
void processMessage(int SocketFD, char *buffer, char *nickname, char *message);

void sendFunction(int SocketFD)
{
	char buffer[256];

	bzero(buffer, 256);
	fgets(buffer, 256, stdin);
	buffer[strcspn(buffer, "\n")] = 0;
	write(SocketFD, buffer, strlen(buffer));

	for(;;)
	{
		bzero(buffer, 256);
		// protocol message ====================
		prepareMessage(buffer);
		//fgets(buffer, 256, stdin);
		write(SocketFD, buffer, strlen(buffer));
	}
}

void receiveFunction(int SocketFD)
{
	char buffer[256], nickname[256], message[256];
	int n;
	for(;;)
	{
		bzero(buffer, 256);
		processMessage(SocketFD, buffer, nickname, message);
		//n = read(SocketFD, buffer, 255);
		/*
		if(n <= 0)
		{
			printf("\ndisconnected\n");
			break;
		}
		buffer[n] = '\0';
		*/
		//mtx.lock();
		//printf("\nServer.- %s\n", buffer);
		printf("\nfrom.- %s\nmessage.- %s\n", nickname, message);
		//mtx.unlock();
	}
}

void processMessage(int SocketFD, char *buffer, char *nickname, char *message)
{
	int n, userSize, msgSize;

	n = read(SocketFD, buffer, 3);
	buffer[n] = '\0';
	userSize = atoi(buffer);

	n = read(SocketFD, buffer, userSize);
	buffer[n] = '\0';
	strcpy(nickname, buffer);

	n = read(SocketFD, buffer, 3);
	buffer[n] = '\0';
	msgSize = atoi(buffer);

	n = read(SocketFD, buffer, msgSize);
	buffer[n] = '\0';
	strcpy(message, buffer);
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

void prepareMessage(char *buffer)
{
	char input[256], inputSizeStr[256];
	int inputSize, offset = 0;

	printf("destination user: ");
	fflush(stdout);
	fgets(input, 256, stdin);

	inputSize = strlen(input) - 1;
	convertIntToString(inputSize, inputSizeStr);

	memcpy(buffer + offset, inputSizeStr, 3);
	offset += 3;
	memcpy(buffer + offset, input, inputSize);
	offset += inputSize;

	printf("message: ");
	fflush(stdout);
	fgets(input, 256, stdin);

	inputSize = strlen(input) - 1;
	convertIntToString(inputSize, inputSizeStr);

	memcpy(buffer + offset, inputSizeStr, 3);
	offset += 3;
	memcpy(buffer + offset, input, inputSize);
	offset += inputSize;
}

int main(void)
{
	struct sockaddr_in stSockAddr;
	int Res;
	int SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	//int n;
	//char buffer[256];


	if (-1 == SocketFD)
	{
		perror("cannot create socket");
		exit(EXIT_FAILURE);
	}

	memset(&stSockAddr, 0, sizeof(struct sockaddr_in));

	stSockAddr.sin_family = AF_INET;
	stSockAddr.sin_port = htons(8888);
	Res = inet_pton(AF_INET, "192.168.100.110", &stSockAddr.sin_addr);

	if (0 > Res)
	{
		perror("error: first parameter is not a valid address family");
		close(SocketFD);
		exit(EXIT_FAILURE);
	}
	else if (0 == Res)
	{
		perror("char string (second parameter does not contain valid ipaddress");
		close(SocketFD);
		exit(EXIT_FAILURE);
	}

	if (-1 == connect(SocketFD, (const struct sockaddr *)&stSockAddr, sizeof(struct sockaddr_in)))
	{
		perror("connect failed");
		close(SocketFD);
		exit(EXIT_FAILURE);
	}

	thread send = thread(sendFunction, SocketFD);
	thread receive = thread(receiveFunction, SocketFD);

	send.join();
	receive.join();

	/*
	for(;;)
	{
		bzero(buffer, 256);

		prepareMessage(buffer);

		write(SocketFD, buffer, strlen(buffer));

		bzero(buffer, 256);

		n = read (SocketFD, buffer, 255);

		printf("\nserver.- %s\n", buffer);
	}
	*/

	shutdown(SocketFD, SHUT_RDWR);
	close(SocketFD);

	return 0;
}
