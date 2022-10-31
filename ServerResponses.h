#ifndef _SERVER_RESPONSES_H_
#define _SERVER_RESPONSES_H_

#include <string_view>

namespace ResponseBody {

inline constexpr std::string_view CRLF = "\r\n";

inline constexpr std::string_view INVALID_REQUEST = "Invalid request";

inline constexpr std::string_view NOT_FOUND = "Not Found";

inline constexpr std::string_view HELLO_MESSAGE = "Hello World";

inline constexpr std::string_view MAIN_TEXT_PAGE = "Hello World!";

inline constexpr std::string_view MAIN_HTML_PAGE = R"(
    <!DOCTYPE html>
    <html>
        <head>
            <title>My Server</title>
        </head>
        <body>
            <h1>Hello world!</h1>
            <p>This is a simple paragraph...</p>
        </body>
    </html>
)";

}

#endif // !_SERVER_RESPONSES_H_
