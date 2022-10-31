#include "Server.h"
#include "ServerResponses.h"

Server::Server()
    : m_methodHandlers(static_cast<int>(HTTP_METHOD::INVALID_METHOD)) {
    using namespace std::string_view_literals;

    addHandler(
        HTTP_METHOD::GET
        , "/"
        , [](std::string &response, const HttpRequest &request) {
            getResponseLine(response, request.line().m_httpVersion, HTTP_RESPONSE_CODE::_200);

            response += "Content-Type: text/html"sv;
            response += ResponseBody::CRLF;

            const static std::string CONTENT_LENGTH_HEADER = "Content-Length: " + std::to_string(ResponseBody::MAIN_HTML_PAGE.size());
            response += CONTENT_LENGTH_HEADER;
            response += ResponseBody::CRLF;

            response += ResponseBody::CRLF;

            response += ResponseBody::MAIN_HTML_PAGE;
        }
    );

    addHandler(
        HTTP_METHOD::GET
        , "/text"
        , [](std::string &response, const HttpRequest &request) {
            getResponseLine(response, request.line().m_httpVersion, HTTP_RESPONSE_CODE::_200);

            response += "Content-Type: text/plain"sv;
            response += ResponseBody::CRLF;

            const static std::string CONTENT_LENGTH_HEADER = "Content-Length: " + std::to_string(ResponseBody::MAIN_TEXT_PAGE.size());
            response += CONTENT_LENGTH_HEADER;
            response += ResponseBody::CRLF;

            response += ResponseBody::CRLF;

            response += ResponseBody::MAIN_TEXT_PAGE;
        }
    );
}

HttpRequest Server::parseRequest(char *rawInput, std::size_t len) {
    auto builder = HttpRequest::create();

    std::size_t offset = parseRequestLine(builder, rawInput, len);
    rawInput += offset;
    len -= offset;

    offset = parseRequestHeaders(builder, rawInput, len);
    rawInput += offset;
    len -= offset;

    offset = parseRequestBody(builder, rawInput, len);

    return builder;
}

void Server::createRawResponse(const HttpRequest &request, std::string &response) {
    if (request.line().m_method == HTTP_METHOD::INVALID_METHOD || request.line().m_httpVersion == HTTP_VERSION::INVALID_VERSION) {
        invalidRequest(response);
        return;
    }

    auto &methodHandlers = m_methodHandlers[static_cast<int>(request.line().m_method)];
    if (methodHandlers.empty()) {
        invalidRequest(response);
        return;
    }

    auto handlerIt = methodHandlers.find(request.line().m_path);
    if (handlerIt == methodHandlers.end()) {
        pageNotFound(response);
        return;
    }

    handlerIt->second(response, request);
}

bool Server::checkCloseRequested(const HttpRequest &request) {
    const static auto connectionKey = HttpHeader::calcKey("connection");

    switch (request.line().m_httpVersion) {
    case HTTP_VERSION::HTTP_11:
        return request.headers().hasFieldValue(connectionKey, "close");
    case HTTP_VERSION::HTTP_10:
        return !request.headers().hasFieldValue(connectionKey, "keep-alive");
    default:
        return true;
    }
}

std::size_t Server::parseRequestLine(HttpRequest::Builder &builder, char *rawInput, const std::size_t len) {
    std::size_t read = 0;

    skipSpaces(read, rawInput, len);

    const auto methodBegin = read;

    while (read < len && std::isalpha(rawInput[read])) {
        ++read;
    }

    std::transform(rawInput + methodBegin, rawInput + read, rawInput + methodBegin, [](char c) { return std::toupper(c); });
    const HTTP_METHOD method = findHttpMethod({ rawInput + methodBegin, read - methodBegin });

    skipSpaces(read, rawInput, len);

    const auto pathBegin = read;

    while (read < len && rawInput[read] != ' ') {
        ++read;
    }

    const std::string_view path{ rawInput + pathBegin, read - pathBegin };

    skipSpaces(read, rawInput, len);

    const auto versionBegin = read;

    while (read < len && rawInput[read] != ' ' && rawInput[read] != '\r') {
        ++read;
    }

    const HTTP_VERSION version = findHttpVersion({ rawInput + versionBegin, read - versionBegin });

    builder.line()
        .setMethod(method)
        .setPath(path)
        .setHttpVersion(version);

    return read;
}

std::size_t Server::parseRequestHeaders(HttpRequest::Builder &builder, char *rawInput, const int len) {
    std::size_t read = 0;

    skipCRLF(read, rawInput, len);

    auto headers = builder.header();

    while (true) {
        skipSpaces(read, rawInput, len);

        const auto fieldStart = read;

        while (read < len && rawInput[read] != ' ' && rawInput[read] != ':') {
            ++read;
        }

        std::transform(rawInput + fieldStart, rawInput + read, rawInput + fieldStart, [](char c) { return std::tolower(c); });
        const std::string_view field{ rawInput + fieldStart, read - fieldStart };

        if (read >= len || rawInput[read] != ':') {
            return {};
        }

        ++read;
        skipSpaces(read, rawInput, len);

        const auto valueStart = read;

        while (read < len && rawInput[read] != '\r' && rawInput[read] != '\n') {
            ++read;
        }

        const std::string_view value{ rawInput + valueStart, read - valueStart };

        headers.add(field, value);

        skipCRLF(read, rawInput, len);

        if (read >= len || rawInput[read] == '\r') {
            break;
        }
    }

    return read;
}

std::size_t Server::parseRequestBody(HttpRequest::Builder &builder, char *rawInput, const int len) {
    std::size_t read = 0;

    skipCRLF(read, rawInput, len);

    if (read < len) {
        builder.body().set({ rawInput + read, len - read });
    }

    return len;
}

void Server::skipSpaces(std::size_t &idx, const char *str, const std::size_t len) {
    while (idx < len && str[idx] == ' ') {
        ++idx;
    }
}

void Server::skipCRLF(std::size_t &idx, const char *str, const std::size_t len) {
    while (idx < len && str[idx] == '\r') {
        ++idx;
    }

    if (idx < len && str[idx] == '\n') {
        ++idx;
    }
}

HTTP_METHOD Server::findHttpMethod(std::string_view method) {
    using namespace std::string_view_literals;

    static constexpr std::array methodMap = {
        std::make_pair("GET"sv, HTTP_METHOD::GET),
        std::make_pair("HEAD"sv, HTTP_METHOD::HEAD),
        std::make_pair("PUT"sv, HTTP_METHOD::PUT),
        std::make_pair("DELETE"sv, HTTP_METHOD::DELETE),
        std::make_pair("CONNECT"sv, HTTP_METHOD::CONNECT),
        std::make_pair("OPTIONS"sv, HTTP_METHOD::OPTIONS),
        std::make_pair("TRACE"sv, HTTP_METHOD::TRACE),
        std::make_pair("PATCH"sv, HTTP_METHOD::PATCH)
    };

    auto it = std::find_if(methodMap.begin(), methodMap.end(), [method](const auto &pair) { return method == pair.first; });
    if (it == methodMap.end()) {
        return HTTP_METHOD::INVALID_METHOD;
    }

    return it->second;
}

HTTP_VERSION Server::findHttpVersion(std::string_view version) {
    using namespace std::string_view_literals;

    static constexpr std::array versionMap = {
        std::make_pair("HTTP/1.0"sv, HTTP_VERSION::HTTP_10),
        std::make_pair("HTTP/1.1"sv, HTTP_VERSION::HTTP_11)
    };

    auto it = std::find_if(
        versionMap.begin()
        , versionMap.end()
        , [version](const auto &pair) {
            return version.size() == pair.first.size() && version.back() == pair.first.back();
        }
    );

    if (it == versionMap.end()) {
        return HTTP_VERSION::INVALID_VERSION;
    }

    return it->second;
}

void Server::invalidRequest(std::string &response) {
    const static std::string CONTENT_LENGTH_HEADER = "Content-Length: " + std::to_string(ResponseBody::INVALID_REQUEST.size());

    getResponseLine(response, HTTP_VERSION::HTTP_11, HTTP_RESPONSE_CODE::_400);

    response += "Content-Type: text/plain";
    response += ResponseBody::CRLF;

    response += CONTENT_LENGTH_HEADER;
    response += ResponseBody::CRLF;

    response += ResponseBody::CRLF;

    response += ResponseBody::INVALID_REQUEST;
}

void Server::pageNotFound(std::string &response) {
    const static std::string CONTENT_LENGTH_HEADER = "Content-Length: " + std::to_string(ResponseBody::NOT_FOUND.size());

    getResponseLine(response, HTTP_VERSION::HTTP_11, HTTP_RESPONSE_CODE::_404);

    response += "Content-Type: text/plain";
    response += ResponseBody::CRLF;

    response += CONTENT_LENGTH_HEADER;
    response += ResponseBody::CRLF;

    response += ResponseBody::CRLF;

    response += ResponseBody::NOT_FOUND;
}

void Server::getResponseLine(std::string &response, HTTP_VERSION version, HTTP_RESPONSE_CODE code) {
    response += versionToString(version);
    response += codeToString(code);
    response += ResponseBody::CRLF;
}

std::string_view Server::versionToString(HTTP_VERSION version) {
    using namespace std::string_view_literals;

    static constexpr std::array versionMap{
        std::make_pair(HTTP_VERSION::HTTP_10, "HTTP/1.0 "sv),
        std::make_pair(HTTP_VERSION::HTTP_11, "HTTP/1.1 "sv),
        std::make_pair(HTTP_VERSION::INVALID_VERSION, " "sv)
    };

    return versionMap[static_cast<int>(version)].second;
}

std::string_view Server::codeToString(HTTP_RESPONSE_CODE code) {
    using namespace std::string_view_literals;

    static constexpr std::array codeMap{
        std::make_pair(HTTP_RESPONSE_CODE::_200, "200 OK"sv),
        std::make_pair(HTTP_RESPONSE_CODE::_400, "400 Bad Request"sv),
        std::make_pair(HTTP_RESPONSE_CODE::_404, "404 Not Found"sv),
        std::make_pair(HTTP_RESPONSE_CODE::_500, "500 Internal Server Error"sv)
    };

    return codeMap[static_cast<int>(code)].second;
}

void Server::addHandler(HTTP_METHOD method, std::string_view path, Handler_t handler) {
    m_methodHandlers[static_cast<int>(method)][path] = handler;
}
