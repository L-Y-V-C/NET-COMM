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
//#include <atomic>
#include "json.hpp"

using json = nlohmann::json;
using namespace std;

//atomic<int> connected(1);

void convertIntToString(int byteSize, int num, char *str);
void sendFunction(int SocketFD);
void receiveFunction(int SocketFD);
void processMessage(int SocketFD);
void print1stMenu(int &op);
void print2ndMenu(int &op);
void checkOption(int op, int menu, int SocketFD);
void makeMessage(int op, int SocketFD);

void print1stMenu(int &op)
{
	char tmp[10];
	printf(
		"-------------------------------\n"
		"|            Menu             |\n"
		"|Options:                     |\n"
		"|  [1] - Login                |\n"
		"|  [2] - Exit                 |\n"
		"-------------------------------\n"
	);
	fflush(stdout);
	fgets(tmp, sizeof(tmp), stdin);
	op = atoi(tmp);
}

void print2ndMenu(int &op)
{
	char tmp[10];
	printf(
		"-------------------------------\n"
		"|           Messages          |\n"
		"|  [1] - Broadcast            |\n"
		"|  [2] - Unicast              |\n"
		"|  [3] - Send file            |\n"
		"|           Others            |\n"
		"|  [4] - Get client list      |\n"
		"|  [0] - Logout               |\n"
		"-------------------------------\n"
	);
	fflush(stdout);
	fgets(tmp, sizeof(tmp), stdin);
	op = atoi(tmp);
}

void checkOption(int op, int menu, int SocketFD)
{
	switch(menu)
	{
		case 1:
			switch(op)
			{
				case 1:
					makeMessage(0, SocketFD);
					break;
				case 2:
					exit(0);
					break;
				default:
					break;
			}
			break;
		case 2:
			switch(op)
			{
				case 1:
					makeMessage(1, SocketFD);
					break;
				case 2:
					makeMessage(2, SocketFD);
					break;
				case 3:
					makeMessage(3, SocketFD);
					break;
				case 4:
					makeMessage(4, SocketFD);
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}
}

void makeMessage(int op, int SocketFD)
{
	char buffer[256], input[256], inputSizeStr[20];
	int inputSize, offset;
	int byteSize[] = {4, 7, 5, 5};
	int byteSizeAux[] = {0, 0, 7};

	offset = 1;
	bzero(buffer, 256);

	switch(op)
	{
		// Login
		case 0:
		{
			buffer[0] = 'L';

			printf("Nickname: ");
			fflush(stdout);
			fgets(input, 256, stdin);
			//buffer[strcspn(buffer, "\n")] = 0;

			inputSize = strlen(input) - 1;
			convertIntToString(byteSize[op] , inputSize, inputSizeStr);

			memcpy(buffer + offset, inputSizeStr, byteSize[op]);
			offset += byteSize[op];
			memcpy(buffer + offset, input, inputSize);

			//printf("msg:\n[%s]\n", buffer);
			write(SocketFD, buffer, strlen(buffer));

			break;
		}
		// send broadcast msg
		case 1:
		{
			buffer[0] = 'B';

			printf("Message: ");
			fflush(stdout);
			fgets(input, 256, stdin);

			inputSize = strlen(input) - 1;
			convertIntToString(byteSize[op] , inputSize, inputSizeStr);

			memcpy(buffer + offset, inputSizeStr, byteSize[op]);
			offset += byteSize[op];
			memcpy(buffer + offset, input, inputSize);

			//printf("msg:\n[%s]\n", buffer);
			write(SocketFD, buffer, strlen(buffer));

			break;
		}
		// send unicast msg
		case 2:
		{
			buffer[0] = 'U';

			printf("Message: ");
			fflush(stdout);
			fgets(input, 256, stdin);

			inputSize = strlen(input) - 1;
			convertIntToString(byteSize[op] , inputSize, inputSizeStr);

			memcpy(buffer + offset, inputSizeStr, byteSize[op]);
			offset += byteSize[op];
			memcpy(buffer + offset, input, inputSize);
			offset += inputSize;

			printf("To: ");
			fflush(stdout);
			fgets(input, 256, stdin);

			inputSize = strlen(input) - 1;
			convertIntToString(byteSizeAux[op] , inputSize, inputSizeStr);

			memcpy(buffer + offset, inputSizeStr, byteSizeAux[op]);
			offset += byteSizeAux[op];
			memcpy(buffer + offset, input, inputSize);

			//printf("msg:\n[%s]\n", buffer);
			write(SocketFD, buffer, strlen(buffer));

			break;
		}
		// send file
		case 3:
		{
			char fileChunk[10], readBuffer[9999], writeBuffer[9999];
			int bytesRead, readSize = 0;

			bzero(writeBuffer, 9999);

			writeBuffer[0] = 'F';

			printf("File name: ");
			fflush(stdout);
			fgets(input, 256, stdin);

			inputSize = strlen(input) - 1;
			input[inputSize] = '\0';

			string fileName(input);
			fileName = "send/" + fileName;

			FILE *file = fopen(fileName.c_str(), "rb");

			if(!file)
			{
				printf("Cannot open the file\n");
				return;
			}
			while ((bytesRead = fread(fileChunk, 1, 10, file)) > 0)
			{
				memcpy(readBuffer + readSize, fileChunk, bytesRead);
				readSize += bytesRead;
			}

			fclose(file);

			convertIntToString(byteSize[op], readSize, inputSizeStr);

			printf("\nfilesize: %s - %d\n", inputSizeStr, readSize);

			memcpy(writeBuffer + offset, inputSizeStr, byteSize[op]);
			offset += byteSize[op];
			memcpy(writeBuffer + offset, readBuffer, readSize);
			offset += readSize;

			convertIntToString(byteSize[op], inputSize, inputSizeStr);

			memcpy(writeBuffer + offset, inputSizeStr, byteSize[op]);
			offset += byteSize[op];
			memcpy(writeBuffer + offset, input, inputSize);
			offset += inputSize;

			printf("To: ");
			fflush(stdout);
			fgets(input, 256, stdin);

			inputSize = strlen(input) - 1;
			convertIntToString(byteSize[op], inputSize, inputSizeStr);

			memcpy(writeBuffer + offset, inputSizeStr, byteSize[op]);
			offset += byteSize[op];
			memcpy(writeBuffer + offset, input, inputSize);

			//cout<<"\nmsg size: "<<strlen(writeBuffer)<<"\nmsg:\n["<<writeBuffer<<"]\n";
			write(SocketFD, writeBuffer, strlen(writeBuffer));

			break;
		}
		// who is in the server?
		case 4:
		{
			buffer[0] = 'T';

			write(SocketFD, buffer, 1);

			break;
		}
		// Logout
		case 5:
		{
			buffer[0] = 'O';
			write(SocketFD, buffer, 1);
			break;
		}
		default:
			break;
	}
}

void sendFunction(int SocketFD)
{
	int op;//status; // status - x = logged - 0 = logged out
	//status = 0;
	op = 0;

	for(;;)
	{
		if(!op)
		{
			print1stMenu(op);
			checkOption(op, 1, SocketFD);
		}
		print2ndMenu(op);
		checkOption(op, 2, SocketFD);
	}
}

void receiveFunction(int SocketFD)
{
	for(;;)
	{
		processMessage(SocketFD);
	}
}

void processMessage(int SocketFD)
{
	char buffer[256], msg[256], msgType, nickname[256];
	int n, i, tmpSize;
	int byteSize[] = {5, 3, 7, 5, 5};
	int byteSizeAux[] = {0, 7, 5};

	bzero(buffer, 256);

	n = read(SocketFD, buffer, 1);

	if(n == 0)
	{
		printf("\n"
			"----------------------\n"
			"You were disconnected!\n"
			"----------------------\n"
		);
		exit(0);
		//connected = 0;
		//return;
	}

	msgType = buffer[0];

	switch(msgType)
	{
		// correct login msg
		case 'K':
		{
			printf("\n"
				"----------------------------\n"
				"Nickname saved successfully!\n"
				"----------------------------\n"
			);
			break;
		}
		// receive error msg
		case 'E':
		{
			i = 0;
			n = read(SocketFD, buffer, byteSize[i]);
			buffer[n] = '\0';
			tmpSize = atoi(buffer);

			n = read(SocketFD, buffer, tmpSize);
			buffer[n] = '\0';
			strcpy(msg, buffer);

			printf("\n"
				"--------------------------------------------------------\n"
				"ERROR!\n"
				"%s\n"
				"--------------------------------------------------------\n"
				, msg
			);
			fflush(stdout);

			break;
		}
		// receive broadcast msg
		case 'b':
		{
			i = 1;
			n = read(SocketFD, buffer, byteSize[i]);
			buffer[n] = '\0';
			tmpSize = atoi(buffer);

			n = read(SocketFD, buffer, tmpSize);
			buffer[n] = '\0';
			strcpy(nickname, buffer);

			n = read(SocketFD, buffer, byteSizeAux[i]);
			buffer[n] = '\0';
			tmpSize = atoi(buffer);

			n = read(SocketFD, buffer, tmpSize);
			buffer[n] = '\0';
			strcpy(msg, buffer);

			printf("\n"
				"--------------------------------------------------------\n"
				"From: %s\n"
				"Msg: %s\n"
				"--------------------------------------------------------\n"
				, nickname, msg
			);
			fflush(stdout);

			break;
		}
		// receive unicast msg
		case 'u':
		{
			i = 2;
			n = read(SocketFD, buffer, byteSize[i]);
			buffer[n] = '\0';
			tmpSize = atoi(buffer);

			n = read(SocketFD, buffer, tmpSize);
			buffer[n] = '\0';
			strcpy(nickname, buffer);

			n = read(SocketFD, buffer, byteSizeAux[i]);
			buffer[n] = '\0';
			tmpSize = atoi(buffer);

			n = read(SocketFD, buffer, tmpSize);
			buffer[n] = '\0';
			strcpy(msg, buffer);

			printf("\n"
				"--------------------------------------------------------\n"
				"From: %s\n"
				"Msg: %s\n"
				"--------------------------------------------------------\n"
				, nickname, msg
			);
			fflush(stdout);

			break;
		}
		// who is in the server?
		case 't':
		{
			i = 3;
			n = read(SocketFD, buffer, byteSize[i]);
			buffer[n] = '\0';
			tmpSize = atoi(buffer);

			n = read(SocketFD, buffer, tmpSize);
			buffer[n] = '\0';
			strcpy(msg, buffer);

			json clientList = json::parse(msg);

			printf("\n"
				"----------------------------------------\n"
				"Users:\n"
			);
			for(auto u : clientList["users"])
			{
				//printf("- %s\n", u);
				cout<<"- "<<u<<"\n";
			}
			printf("\n"
				"----------------------------------------\n"
			);

			break;
		}
		// receive file
		case 'f':
		{
			i = 4;

			char readBuffer[9999], fileData[9999];
			int fileSize = 0;

			bzero(readBuffer, 9999);

			n = read(SocketFD, readBuffer, byteSize[i]);
			readBuffer[n] = '\0';
			tmpSize = atoi(readBuffer);

			while (fileSize < tmpSize)
			{
				n = read(SocketFD, fileData + fileSize, tmpSize - fileSize);
				fileSize += n;
			}

			n = read(SocketFD, readBuffer, byteSize[i]);
			readBuffer[n] = '\0';
			tmpSize = atoi(readBuffer);

			n = read(SocketFD, readBuffer, tmpSize);
			readBuffer[n] = '\0';
			strcpy(msg, readBuffer);

			n = read(SocketFD, readBuffer, byteSize[i]);
			readBuffer[n] = '\0';
			tmpSize = atoi(readBuffer);

			n = read(SocketFD, readBuffer, tmpSize);
			readBuffer[n] = '\0';
			strcpy(nickname, readBuffer);

			string fileName(msg);
			fileName = "receive/" + fileName;

			FILE *file = fopen(fileName.c_str(), "wb");

			if (!file)
			{
				printf("Cannot create the file\n");
				break;
			}

			fwrite(fileData, 1, fileSize, file);
			fclose(file);

			printf("\n"
				"----------------------------------\n"
				"File received\n"
				"From: %s\n"
				"File name: %s\n"
				"----------------------------------\n"
				, nickname, msg
			);
			fflush(stdout);

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

int main(void)
{
	struct sockaddr_in stSockAddr;
	int Res;
	int SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (-1 == SocketFD)
	{
		perror("cannot create socket");
		exit(EXIT_FAILURE);
	}

	memset(&stSockAddr, 0, sizeof(struct sockaddr_in));

	stSockAddr.sin_family = AF_INET;
	stSockAddr.sin_port = htons(45000);
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

	shutdown(SocketFD, SHUT_RDWR);
	close(SocketFD);

	return 0;
}
