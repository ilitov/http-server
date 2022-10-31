#include "ThreadData.h"

#include <cassert>

void ThreadData::Event::clear() {
	m_data.buffer.clear();
	m_data.offset = 0;
	m_data.clientClosed = 0;
	m_data.fd = -1;
	m_next = nullptr;
}

ThreadData::ThreadData()
	: m_head{ &m_data.front() }
	, m_tail{ &m_data.back() } {

	for (std::size_t i = 0; i + 1 < m_data.size(); ++i) {
		auto &e = m_data[i];
		e.m_next = &m_data[i + 1];
	}

}

bool ThreadData::add(int fd) {
	{
		std::lock_guard lock(m_mtxTail);
		if (m_head == m_tail) {
			return false;
		}
	}

	Event *oldHead = m_head;
	m_head = m_head->m_next;

	oldHead->m_data.fd = fd;

	epoll_event newEvent;
	newEvent.events = EPOLLIN | EPOLLHUP | EPOLLRDHUP;
	newEvent.data.ptr = oldHead;

	if (epoll_ctl(m_epollfd, EPOLL_CTL_ADD, fd, &newEvent) == -1) {
		release(*oldHead, &newEvent);
		return false;
	}

	return true;
}

void ThreadData::release(Event &myEvnt, epoll_event *evnt) {
	const int fd = myEvnt.m_data.fd;

	myEvnt.clear();

	{
		std::lock_guard lock(m_mtxTail);
		m_tail->m_next = &myEvnt;
		m_tail = m_tail->m_next;
	}

	if (epoll_ctl(m_epollfd, EPOLL_CTL_DEL, fd, evnt) == -1) {
		assert(false && "Error. Can't release the epoll event.");
	}
}

int ThreadData::getEpollFd() const {
	return m_epollfd;
}

void ThreadData::setEpollFd(int id) {
	m_epollfd = id;
}
