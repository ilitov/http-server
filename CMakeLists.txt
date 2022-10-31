cmake_minimum_required (VERSION 3.8)

project(http-server)

set(CMAKE_CXX_STANDARD 17)

set(CPP_FILES
    main.cpp
    HttpMessage.cpp
    Server.cpp
    ThreadData.cpp
    ServerThread.cpp
)

set(HEADER_FILES
    ServerConstants.h
    ServerResponses.h
    HttpMessage.h
    Server.h
    ThreadData.h
    ServerThread.h
)

add_executable(${PROJECT_NAME} ${CPP_FILES} ${HEADER_FILES})

target_link_libraries(${PROJECT_NAME} pthread)
