#include "HttpMessage.h"

bool HttpHeader::hasFieldValue(std::size_t key, std::string_view value) const {
    auto it = m_fields.find(key);

    if (it != m_fields.end()) {
        return it->second == value;
    }

    return false;
}

bool HttpHeader::hasFieldValue(std::string_view field, std::string_view value) const {
    const auto key = calcKey(field);
    return hasFieldValue(key, value);
}

void HttpHeader::insert(std::string_view field, std::string_view value) {
    const auto key = calcKey(field);
    m_fields[key] = value;
}

std::size_t HttpHeader::calcKey(std::string_view field) {
    return std::hash<std::string_view>{}(field);
}

HttpMessageBuilderBase::HttpMessageBuilderBase(HttpRequest &message)
    : m_message{ message } {

}

HttpRequestLineBuilder HttpMessageBuilderBase::line() {
    return { m_message };
}

HttpHeaderBuilder HttpMessageBuilderBase::header() {
    return { m_message };
}

HttpBodyBuilder HttpMessageBuilderBase::body() {
    return { m_message };
}

HttpMessageBuilderBase::operator HttpRequest() {
    return std::move(m_message);
}

HttpRequest & HttpMessageBuilderBase::getMessage() {
    return m_message;
}

HttpRequestLineBuilder& HttpRequestLineBuilder::setMethod(HTTP_METHOD method) {
    getMessage().m_line.m_method = method;
    return *this;
}

HttpRequestLineBuilder& HttpRequestLineBuilder::setPath(std::string_view path) {
    getMessage().m_line.m_path = path;
    return *this;
}

HttpRequestLineBuilder& HttpRequestLineBuilder::setHttpVersion(HTTP_VERSION version) {
    getMessage().m_line.m_httpVersion = version;
    return *this;
}

HttpHeaderBuilder& HttpHeaderBuilder::add(std::string_view field, std::string_view value) {
    auto &header = getMessage().m_header;
    header.insert(field, value);

    return *this;
}

HttpBodyBuilder& HttpBodyBuilder::set(std::string_view value) {
    auto &body = getMessage().m_body.m_data;
    body = value;

    return *this;
}

HttpRequest::Builder HttpRequest::create() {
    return {};
}

const HttpRequestLine& HttpRequest::line() const {
    return m_line;
}

const HttpHeader& HttpRequest::headers() const {
    return m_header;
}

const HttpBody& HttpRequest::body() const {
    return m_body;
}
