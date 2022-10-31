#ifndef _THREAD_DATA_H_
#define _THREAD_DATA_H_

#include "ServerConstants.h"

#include <mutex>
#include <array>
#include <sys/epoll.h>

class ThreadData {
public:
	static constexpr int MSG_BUFFER_AVG_SIZE = 1024;

	struct Event {
		struct Data {
			Data()
				: offset(0)
				, clientClosed(0)
				, fd(-1) {
				buffer.reserve(MSG_BUFFER_AVG_SIZE);
			}
			Data(const Data &) = delete;
			Data& operator=(const Data &) = delete;
			Data(Data &&) = default;
			Data& operator=(Data &&) = default;
			~Data() = default;

			std::string buffer;
			std::uint32_t offset : 31;
			std::uint32_t clientClosed : 1;
			int fd;
		} m_data;

		Event *m_next{ nullptr };

		void clear();
	};

	static_assert(sizeof(Event) == 48, "Broken Event size");

	explicit ThreadData();

	bool add(int fd);
	void release(Event &myEvnt, epoll_event *evnt);

	int getEpollFd() const;
	void setEpollFd(int id);

private:
	int m_epollfd;
	std::array<Event, CONNECTIONS_PER_THREAD + 1> m_data;
	Event *m_head;
	Event *m_tail;
	std::mutex m_mtxTail;
};

#endif // !_THREAD_DATA_H_
