# HTTP Server

## Description:
- There is one listener thread that distributes the incoming connections to a set of worker threads;
- The number of worker threads and the maximum number of active connections are specified in **ServerConstants.h**;
- There are only two endpoints: **"/"** and **"/text"**;
- The server supports only GET requests with HTTP version 1.0 or 1.1;
- The server supports HTTP requests up to 4KB, but can return HTTP responses of an arbitrary length;
- The only way to stop/close the server is with Ctr+C;

## How to build:
```
$ cmake -E make_directory build
$ cd build
$ cmake .. -DCMAKE_BUILD_TYPE=Release
$ cmake --build .
$ ./http-server
```

## Test setup:
```
OS: WSL2 / Ubuntu 20.04 LTS
Linux version: 5.10.102.1-microsoft-standard-WSL2
CPU: Intel Core i7-1165G7(8 cores) @ 2.80GHz
Test tool: github.com/wg/wrk
```

## Load test results:
```
$ ./wrk -c10000 -d30s -t2 http://localhost:3490
Running 30s test @ http://localhost:3490
  2 threads and 10000 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency     1.53ms  622.62us  34.03ms   95.69%
    Req/Sec   167.24k    28.42k  220.75k    88.09%
  9958702 requests in 30.05s, 2.70GB read
Requests/sec: 331352.42
Transfer/sec:     91.96MB
```
```
$ ./wrk -c10000 -d30s -t6 http://localhost:3490
Running 30s test @ http://localhost:3490
  6 threads and 10000 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency     1.25ms    2.74ms  61.04ms   93.66%
    Req/Sec   113.76k    29.88k  216.04k    72.80%
  20342324 requests in 30.09s, 5.51GB read
Requests/sec: 676076.57
Transfer/sec:    187.62MB
```
