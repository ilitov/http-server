#include <iostream>
#include <cstring>
#include <thread>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <unistd.h>

#include <sys/epoll.h>

#include "ServerConstants.h"
#include "Server.h"
#include "ThreadData.h"
#include "ServerThread.h"

int setupListenerSocket() {
	struct addrinfo hints;
	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	struct addrinfo *results = nullptr;

	if (getaddrinfo(nullptr, SERVER_PORT, &hints, &results) != 0) {
		perror("getaddrinfo");
		return -1;
	}

	int listener = -1;
	auto *tmp = results;
	for (; tmp; tmp = tmp->ai_next) {
		const int sid = socket(tmp->ai_family, tmp->ai_socktype, tmp->ai_protocol);
		if (sid == -1) {
			perror("socket");
			continue;
		}

		if (int yes = 1; setsockopt(sid, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &yes, sizeof(yes)) == -1) {
			perror("setsockopt");
			close(sid);
			continue;
		}

		if (bind(sid, tmp->ai_addr, tmp->ai_addrlen) == -1) {
			perror("bind");
			close(sid);
			continue;
		}

		{
			struct sockaddr_in address;
			std::memcpy(&address, tmp->ai_addr, sizeof(address));

			char addressString[INET_ADDRSTRLEN];

			inet_ntop(tmp->ai_family, &address.sin_addr, addressString, INET_ADDRSTRLEN);

			std::cout << "Server address: " << addressString << ':' << SERVER_PORT << '\n';
		}

		listener = sid;
		break;
	}

	freeaddrinfo(results);

	if (listener != -1 && listen(listener, BACKLOG_SIZE) == -1) {
		perror("listen");
		close(listener);
		return -1;
	}

	return listener;
}

// ###################################################

int main() {
	const int listener = setupListenerSocket();
	if (listener == -1) {
		std::cout << "Setup failed! Exiting...\n";
		return 1;
	}

	std::cout << "Server: Allocating resources...\n";

	Server httpServer;

	int epollfds[NUM_THREADS];
	ServerThread threads[NUM_THREADS];

	for (int i = 0; i < NUM_THREADS; ++i) {
		epollfds[i] = epoll_create1(0);
		if (epollfds[i] == -1) {
			perror("epoll_create1");
			return 1;
		}

		threads[i].runThread(httpServer, epollfds[i]);
	}

	socklen_t clientAddrSize = sizeof(struct sockaddr_in);

	struct sockaddr clientAddress;
	std::memset(&clientAddress, 0, sizeof(struct sockaddr_in));

	int threadIdx = 0;

	std::cout << "Server: Waiting for connections...\n";

	while (true) {
		const int clientSocket = accept4(listener, &clientAddress, &clientAddrSize, SOCK_NONBLOCK);
		if (clientSocket == -1) {
			continue;
		}

		bool connectionReceived = false;
		int probeIdx = threadIdx;

		do {
			connectionReceived = threads[probeIdx].addClient(clientSocket);
			probeIdx = (probeIdx + 1) % NUM_THREADS;
		} while (!connectionReceived && probeIdx != threadIdx);

		if (!connectionReceived) {
			std::cout << "Connection error\n";

			close(clientSocket);
		}

		threadIdx = probeIdx;
	}

	for (int fd : epollfds) {
		close(fd);
	}

	close(listener);

    return 0;
}
