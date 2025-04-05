// a datagram socket client demo

#include <stdio.h>
#include <stdlib.h>
#ifdef WIN32
	#define WIN32_LEAN_AND_MEAN

	#include <tchar.h>
	#include <windows.h>
	#include <winsock2.h>
	#include <ws2tcpip.h>

	// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
	#pragma comment (lib, "Ws2_32.lib")
	#pragma comment (lib, "Mswsock.lib")
	#pragma comment (lib, "AdvApi32.lib")
#elif
	#include <unistd.h>
	#include <errno.h>
	#include <string.h>
	#include <netdb.h>
	#include <sys/types.h>
	#include <netinet/in.h>
	#include <sys/socket.h>
	#include <arpa/inet.h>
#endif

#define BUFLEN 512
#define PORT "27015" // the port client will be connecting to 
	
void *get_in_addr(struct sockaddr *sa);
void PrintMessage(char* error_str,int ReturnedValue,int ShowLastError);
void close_socket(int ConnectSocket);
void socket_cleanup();
int safe_gets(char* line, size_t size, const char* input);


int main(int argc, char *argv[])
{
#ifdef WIN32
    WSADATA wsaData;
#endif
    int ConnectSocket = INVALID_SOCKET;
    struct addrinfo hints;
    struct addrinfo *ServerAddress = NULL;
    int iResult;
    char sendbuf[BUFLEN];
    char recvbuf[BUFLEN];
	char* responses[3] = { NULL, NULL, NULL };


	// Validate the parameters
	//////////////////////////////////////////////////////////////////////////
	if (argc == 1) {
		fprintf(stderr, "client software can get server name or IP as argument.\n");
		fprintf(stderr, "default is loopback address: 127.0.0.1\n\n");
	}
	else if (argc != 2) {
		fprintf(stderr, "client software usage: %s server-name-or-IP\n or: %s    for loopback address 127.0.0.1\n", argv[0]);
		fprintf(stderr, "\nprogram quit in 15 seconds\n"); Sleep(15000);
		return 1;
	}



    // Initialize Winsock
	//////////////////////////////////////////////////////////////////////////
	#ifdef WIN32
		iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
		if (iResult != 0) {
			PrintMessage("client: WSAStartup failed with error",iResult,0);
			fprintf(stderr, "\nprogram quit in 15 seconds\n"); Sleep(15000);
			return 1;
		}
	#endif



    // Resolve the server address and port
	//////////////////////////////////////////////////////////////////////////
	memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;  //IPv4 address family
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
	if (argc == 2)
		iResult = getaddrinfo(argv[1], PORT, &hints, &ServerAddress);
	else
		iResult = getaddrinfo("127.0.0.1", PORT, &hints, &ServerAddress);
    if ( iResult != 0 ) {
		fprintf(stderr, "client: getaddrinfo failed with error: %s\n", gai_strerror(iResult));
		socket_cleanup();
		fprintf(stderr, "\nprogram quit in 15 seconds\n"); Sleep(15000);
		return 1;
    }



	// Create a SOCKET for connecting to server
	//////////////////////////////////////////////////////////////////////////
	//if ai_family == AF_UNSPEC then socket sould be open for every connection attempt
	printf("client: socket()\n\n");
    ConnectSocket = socket(hints.ai_family, hints.ai_socktype, hints.ai_protocol);
    if (ConnectSocket == INVALID_SOCKET) {
		PrintMessage("client:		socket failed",ConnectSocket,1);
		socket_cleanup();
		fprintf(stderr, "\nprogram quit in 15 seconds\n"); Sleep(15000);
		return 1;
	}


	while (1) {

		// Sendto
		//////////////////////////////////////////////////////////////////////////
		char* commands[] = { "SENSOR GET 1\n", "SENSOR GET 2\n", "SENSOR GET 3\n" };
		for (auto& command : commands) {
			safe_gets(sendbuf, BUFLEN, command);
			iResult = sendto(ConnectSocket, sendbuf, strlen(sendbuf), 0, ServerAddress->ai_addr, ServerAddress->ai_addrlen);

			if (iResult == SOCKET_ERROR) {
				PrintMessage("client: send failed with error", iResult, 1);
				freeaddrinfo(ServerAddress);
				close_socket(ConnectSocket);
				socket_cleanup();
				fprintf(stderr, "\nprogram quit in 15 seconds\n"); Sleep(15000);
				return 1;
			}
		}
		Sleep(1000); // Rest for one second


		// Receivefrom
		//////////////////////////////////////////////////////////////////////////
		for (int i = 0; i < 3; ++i) {
			iResult = recvfrom(ConnectSocket, recvbuf, BUFLEN, 0, NULL, NULL);
			if (iResult >= 0) {
				recvbuf[iResult] = '\0';  // Ensure null termination
				responses[i] = strdup(recvbuf);  // Save the response
			}
			else {
				PrintMessage("client: recv failed", iResult, 1);
				break;
			}
		}
		// Trim "SENSOR DATA" part form the result
		int numbers[3];
		for (int i = 0; i < 3; i++) {
			sscanf(responses[i], "SENSOR DATA %d", &numbers[i]);
		}
		printf("\nWater: %d[deg], Reactor: %d[deg], Pump current: %d[A].",
			numbers[0], numbers[1], numbers[2]);
	}



	// cleanup
	//////////////////////////////////////////////////////////////////////////
	freeaddrinfo(ServerAddress);
	close_socket(ConnectSocket);
	socket_cleanup();
	fprintf(stderr, "\nprogram quit in 15 seconds\n"); Sleep(15000);
	return 0;
}









// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void PrintMessage(char* error_str,int ReturnedValue,int ShowLastError)
{
	#ifdef WIN32
		if(ShowLastError)
			fprintf(stderr, "%s. returned value: %d, last error: %ld.\n", error_str, ReturnedValue, WSAGetLastError());
		else
			fprintf(stderr, "%s: %d.\n", error_str, ReturnedValue);
	#elif
		fprintf(stderr, "%s: %d.\n", error_str, ReturnedValue);
	#endif
}


void close_socket(int ConnectSocket)
{
	#ifdef WIN32
		closesocket(ConnectSocket);
	#elif
		close(ConnectSocket);
	#endif
}

void socket_cleanup()
{
	#ifdef WIN32
		WSACleanup();
	#endif
}

int safe_gets(char* line, size_t size, const char* input)
{
	size_t i;
	static size_t position = 0;  // Keeps track of the position in the input string

	// Reset position before reading each new input string
	position = 0;

	// Loop through the input string until we either reach the end or fill the buffer
	for (i = 0; i < size - 1; ++i)
	{
		int ch = input[position];  // Get the current character from the input string
		if (ch == '\0' || ch == '\n') {  // End of string or newline
			break;
		}
		line[i] = ch;  // Store the character in the buffer
		position++;  // Move to the next character in the string
	}

	line[i] = '\0';  // Null-terminate the string
	return i;  // Return the number of characters read
}

