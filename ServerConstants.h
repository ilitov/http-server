#ifndef _SERVER_CONSTANTS_H_
#define _SERVER_CONSTANTS_H_

inline constexpr auto NUM_THREADS = 7; // N - 1 threads where N is the number of cores
inline constexpr auto MAX_CONNECTIONS_NUM = 10000;
inline constexpr auto CONNECTIONS_PER_THREAD = MAX_CONNECTIONS_NUM / NUM_THREADS + 1;

inline constexpr auto BACKLOG_SIZE = 10000;
inline constexpr auto SERVER_PORT = "3490";

#endif // !_SERVER_CONSTANTS_H_
