// a datagram socket server demo

#include <stdio.h>
#include <stdlib.h>
#ifdef WIN32
	#undef UNICODE
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
	#include <sys/wait.h>
	#include <signal.h>
#endif



#define BUFLEN 512
#define PORT "27015" // the port client will be connecting to 
	
void *get_in_addr(struct sockaddr *sa);
void PrintMessage(char* error_str,int ReturnedValue,int ShowLastError);
void close_socket(int ConnectSocket);
void socket_cleanup();




int main(int argc, char *argv[])
{
#ifdef WIN32
    WSADATA wsaData;
#endif
    struct addrinfo hints;
    struct addrinfo *ServerAddress = NULL;
	struct sockaddr_storage ClientAddress;
	int addr_len;
    int ConnectSocket = INVALID_SOCKET;
    int iResult;
    char recvbuf[BUFLEN];
	char ClientAddressString[INET6_ADDRSTRLEN];


    // Initialize Winsock
	//////////////////////////////////////////////////////////////////////////
	#ifdef WIN32
		iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
		if (iResult != 0) {
			PrintMessage("server: WSAStartup failed with error",iResult,0);
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
	hints.ai_flags = AI_PASSIVE; // use my IP
	iResult = getaddrinfo(NULL, PORT, &hints, &ServerAddress);
    if ( iResult != 0 ) {
		fprintf(stderr, "server: getaddrinfo failed with error: %s\n", gai_strerror(iResult));
		socket_cleanup();
		fprintf(stderr, "\nprogram quit in 15 seconds\n"); Sleep(15000);
		return 1;
    }



	// Create a SOCKET for listening
	//////////////////////////////////////////////////////////////////////////
	//if ai_family == AF_UNSPEC then socket sould be open for every connection attempt
	printf("server: socket()\n");
    ConnectSocket = socket(hints.ai_family, hints.ai_socktype, hints.ai_protocol);
    if (ConnectSocket == INVALID_SOCKET) {
		PrintMessage("server:		socket failed",ConnectSocket,1);
		socket_cleanup();
		fprintf(stderr, "\nprogram quit in 15 seconds\n"); Sleep(15000);
		return 1;
	}



	// bind to port
	//////////////////////////////////////////////////////////////////////////
	printf("server: bind()\n\n");
    iResult = bind(ConnectSocket, ServerAddress->ai_addr, (int)ServerAddress->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
		PrintMessage("server:		bind failed",iResult,1);
		ConnectSocket = INVALID_SOCKET;
		close_socket(ConnectSocket);
        freeaddrinfo(ServerAddress);
		socket_cleanup();
		fprintf(stderr, "\nprogram quit in 15 seconds\n"); Sleep(15000);
		return 1;
    }
	freeaddrinfo(ServerAddress); // all done with this structure




    do {
		// Receive
		//////////////////////////////////////////////////////////////////////////
		printf("************************************************\n");
		printf("server: recvfrom()... (waiting)\n");
		addr_len = sizeof(ClientAddress);
		iResult = recvfrom(ConnectSocket, recvbuf, BUFLEN-1 , 0, (struct sockaddr *)&ClientAddress, &addr_len);
        if (iResult > 0) {
			recvbuf[iResult] = NULL;
			inet_ntop(ClientAddress.ss_family, get_in_addr((struct sockaddr *)&ClientAddress), ClientAddressString, INET6_ADDRSTRLEN);
			recvbuf[iResult] = '\0';
			printf("server:		got packet from %s, packet is %d bytes long\n", ClientAddressString, iResult);
			printf("server:		packet contains \"%s\"\n", recvbuf);
		}
        else if (iResult == 0)
			PrintMessage("server:		Connection closing...",iResult,0);
        else  {
			PrintMessage("server:		recv failed",iResult,1);
			close_socket(ConnectSocket);
			socket_cleanup();
			fprintf(stderr, "\nprogram quit in 15 seconds\n"); Sleep(15000);
			return 1;
        }


		// Sendto
		//////////////////////////////////////////////////////////////////////////
		printf("server: sendto()\n");
        if (iResult > 0) {
			// Echo the buffer back to the sender
			iResult = sendto(ConnectSocket,recvbuf,strlen(recvbuf),0,(struct sockaddr *)&ClientAddress,sizeof(ClientAddress));
            if (iResult == SOCKET_ERROR) {
				PrintMessage("server:		send failed",iResult,1);
				freeaddrinfo(ServerAddress);
				close_socket(ConnectSocket);
				socket_cleanup();
				fprintf(stderr, "\nprogram quit in 15 seconds\n"); Sleep(15000);
				return 1;
            }
        }
		printf("************************************************\n\n");

	} while (iResult > 0);




	// cleanup
	//////////////////////////////////////////////////////////////////////////
    closesocket(ConnectSocket);
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




