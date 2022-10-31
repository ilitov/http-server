#ifndef _SERVER_H_
#define _SERVER_H_

#include "HttpMessage.h"

#include <vector>
#include <algorithm>
#include <string_view>
#include <unordered_map>

class Server {
    using Handler_t = void(*)(std::string&, const HttpRequest&);

public:
    explicit Server();

public:
    static HttpRequest parseRequest(char *rawInput, std::size_t len);

    void createRawResponse(const HttpRequest &request, std::string &response);

    static bool checkCloseRequested(const HttpRequest &request);

private:
    static std::size_t parseRequestLine(HttpRequest::Builder &builder, char *rawInput, const std::size_t len);
    static std::size_t parseRequestHeaders(HttpRequest::Builder &builder, char *rawInput, const int len);
    static std::size_t parseRequestBody(HttpRequest::Builder &builder, char *rawInput, const int len);

    static void skipSpaces(std::size_t &idx, const char *str, const std::size_t len);
    static void skipCRLF(std::size_t &idx, const char *str, const std::size_t len);

    static HTTP_METHOD findHttpMethod(std::string_view method);
    static HTTP_VERSION findHttpVersion(std::string_view version);

    static void invalidRequest(std::string &response);
    static void pageNotFound(std::string &response);

    static void getResponseLine(std::string &response, HTTP_VERSION version, HTTP_RESPONSE_CODE code);
    static std::string_view versionToString(HTTP_VERSION version);
    static std::string_view codeToString(HTTP_RESPONSE_CODE code);

private:
    void addHandler(HTTP_METHOD method, std::string_view path, Handler_t handler);

private:
    std::vector<std::unordered_map<std::string_view, Handler_t>> m_methodHandlers;
};

#endif // !_SERVER_H_
