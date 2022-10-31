#ifndef _HTTP_MESSAGE_H_
#define _HTTP_MESSAGE_H_

#include <vector>
#include <string_view>
#include <unordered_map>

enum class HTTP_RESPONSE_CODE {
    _200,
    _400,
    _404,
    _500
};

enum class HTTP_METHOD {
    GET,
    HEAD,
    POST,
    PUT,
    DELETE,
    CONNECT,
    OPTIONS,
    TRACE,
    PATCH,
    INVALID_METHOD
};

enum class HTTP_VERSION {
    HTTP_10,
    HTTP_11,
    INVALID_VERSION
};

struct HttpRequestLine {
    HTTP_METHOD m_method;
    HTTP_VERSION m_httpVersion;
    std::string_view m_path;
};

struct HttpHeader {
    using HeaderFields_t = std::unordered_map<std::size_t, std::string_view>; // (field hash -> value)

    bool hasFieldValue(std::size_t key, std::string_view value) const;
    bool hasFieldValue(std::string_view field, std::string_view value) const;
    void insert(std::string_view field, std::string_view value);

    static std::size_t calcKey(std::string_view field);

private:
    HeaderFields_t m_fields;
};

struct HttpBody {
    std::string_view m_data;
};

class HttpRequest;
class HttpRequestLineBuilder;
class HttpHeaderBuilder;
class HttpBodyBuilder;

class HttpMessageBuilderBase {
public:
    HttpRequestLineBuilder line();
    HttpHeaderBuilder header();
    HttpBodyBuilder body();

    operator HttpRequest();

protected:
    HttpMessageBuilderBase(HttpRequest &message);

    HttpRequest& getMessage();

private:
    HttpRequest &m_message;
};

class HttpRequestLineBuilder : public HttpMessageBuilderBase {
public:
    HttpRequestLineBuilder(HttpRequest &message) : HttpMessageBuilderBase{ message } {}
    HttpRequestLineBuilder& setMethod(HTTP_METHOD method);
    HttpRequestLineBuilder& setPath(std::string_view path);
    HttpRequestLineBuilder& setHttpVersion(HTTP_VERSION version);
};

class HttpHeaderBuilder : public HttpMessageBuilderBase {
public:
    HttpHeaderBuilder(HttpRequest &message) : HttpMessageBuilderBase{ message } {}
    HttpHeaderBuilder& add(std::string_view field, std::string_view value);
};

class HttpBodyBuilder : public HttpMessageBuilderBase {
public:
    HttpBodyBuilder(HttpRequest &message) : HttpMessageBuilderBase{ message } {}
    HttpBodyBuilder& set(std::string_view value);
};

template <typename Message>
class HttpMessageBuilder : public HttpMessageBuilderBase {
public:
    HttpMessageBuilder() : HttpMessageBuilderBase{ m_message } {}
    HttpMessageBuilder(const HttpMessageBuilder&) = delete;
    HttpMessageBuilder& operator=(const HttpMessageBuilder&) = delete;

private:
    Message m_message;
};

class HttpRequest {
    friend class HttpRequestLineBuilder;
    friend class HttpHeaderBuilder;
    friend class HttpBodyBuilder;
    friend class HttpMessageBuilder<HttpRequest>;

public:
    using Builder = HttpMessageBuilder<HttpRequest>;

    HttpRequest() = default;
    HttpRequest(const HttpRequest &) = delete;
    HttpRequest& operator=(const HttpRequest&) = delete;
    HttpRequest(HttpRequest&&) = default;
    HttpRequest& operator=(HttpRequest&&) = default;

    static Builder create();

    const HttpRequestLine &line() const;
    const HttpHeader &headers() const;
    const HttpBody &body() const;

private:
    HttpRequestLine m_line;
    HttpHeader m_header;
    HttpBody m_body;
};

#endif // !_HTTP_MESSAGE_H_
