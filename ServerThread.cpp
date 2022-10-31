#include "ServerThread.h"

#include <cassert>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

ServerThread::ServerThread() {
	m_responseBuffer.reserve(ThreadData::MSG_BUFFER_AVG_SIZE);
}

ServerThread::~ServerThread() {
	if (m_thread.joinable()) {
		m_thread.join();
	}
}

void ServerThread::runThread(Server &server, int epollfd) {
	m_data.setEpollFd(epollfd);

	m_server = &server;
	m_thread = std::thread{ &ServerThread::threadLoop, this };
}

bool ServerThread::addClient(int fd) {
	return m_data.add(fd);
}

void ServerThread::threadLoop() {
	const int epollfd = m_data.getEpollFd();

	epoll_event events[CONNECTIONS_PER_THREAD];

	auto closeConnection = [this](ThreadData::Event &eventData, epoll_event *event) {
		const int fd = eventData.m_data.fd;

		shutdown(fd, SHUT_RDWR);

		m_data.release(eventData, event);

		close(fd);
	};

	while (true) {
		const int eventsNum = epoll_wait(epollfd, events, CONNECTIONS_PER_THREAD, -1);
		if (eventsNum == -1) {
			perror("epoll_wait");
			continue;
		}

		for (int i = 0; i < eventsNum; ++i) {
			auto &event = events[i];

			assert(event.data.ptr && "The pointer must point to pre-allocated and valid data");

			ThreadData::Event &eventData = *static_cast<ThreadData::Event *>(event.data.ptr);

			if (event.events & EPOLLRDHUP || event.events & EPOLLHUP) {
				closeConnection(eventData, &event);
				continue;
			}

			if (event.events & EPOLLIN) {

				if (!readData(eventData, event)) {
					closeConnection(eventData, &event);
					continue;
				}
			}
			else if (event.events & EPOLLOUT) {

				if (!sendData(eventData, event)) {
					closeConnection(eventData, &event);
					continue;
				}
			}
		}
	}
}

bool ServerThread::readData(ThreadData::Event &event, epoll_event &epollEvent) {
	const int fd = event.m_data.fd;

	constexpr auto BUFF_SIZE = 4096;
	char buff[BUFF_SIZE + 1];

	const int nr = recv(fd, buff, BUFF_SIZE, 0);

	switch (nr) {
	case -1:
		// Do nothing, just loop and wait for the next epoll_wait()
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			return true;
		}

		return false; // error
	case 0:
		// The connection must be closed
		return false;
	default:
		buff[nr] = '\0';

		// Parse the input message
		HttpRequest inputMessage = m_server->parseRequest(buff, static_cast<std::size_t>(nr));

		m_responseBuffer.clear();

		m_server->createRawResponse(inputMessage, m_responseBuffer);

		// Save the response to a buffer
		event.m_data.buffer.swap(m_responseBuffer);
		event.m_data.offset = 0;
		event.m_data.clientClosed = m_server->checkCloseRequested(inputMessage);

		if (event.m_data.clientClosed) {
			shutdown(fd, SHUT_RD); // Won't read anymore
		}

		epollEvent.events = EPOLLOUT | EPOLLHUP | EPOLLRDHUP;

		if (epoll_ctl(m_data.getEpollFd(), EPOLL_CTL_MOD, fd, &epollEvent) == -1) {
			assert(false && "This should not happen.");
			return false;
		}

		return true;
	}
}

bool ServerThread::sendData(ThreadData::Event &event, epoll_event &epollEvent) {
	auto &data = event.m_data;
	const int fd = data.fd;

	const int nw = send(fd, data.buffer.c_str() + data.offset, data.buffer.size() - data.offset, 0);

	switch (nw) {
	case -1:
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			return true;
		}

		return false;
	default:
		data.offset += nw;

		// Everything has been sent
		if (data.offset == data.buffer.size()) {

			if (data.clientClosed) {
				shutdown(fd, SHUT_WR); // Won't write anymore
				return false;
			}

			epollEvent.events = EPOLLIN | EPOLLHUP | EPOLLRDHUP;

			if (epoll_ctl(m_data.getEpollFd(), EPOLL_CTL_MOD, fd, &epollEvent) == -1) {
				assert(false && "This should not happen.");
				return false;
			}
		}

		return true;
	}
}
