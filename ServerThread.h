#ifndef _SERVER_THREAD_H_
#define _SERVER_THREAD_H_

#include "ThreadData.h"
#include "Server.h"

#include <string>
#include <thread>

class ServerThread {
public:
	explicit ServerThread();
	~ServerThread();

	void runThread(Server &server, int epollfd);

	bool addClient(int fd);

private:
	void threadLoop();

	bool readData(ThreadData::Event &event, epoll_event &epollEvent);
	bool sendData(ThreadData::Event &event, epoll_event &epollEvent);

private:
	Server *m_server;

    ThreadData m_data;
    std::thread m_thread;

    std::string m_responseBuffer;
};


#endif // !_SERVER_THREAD_H_
