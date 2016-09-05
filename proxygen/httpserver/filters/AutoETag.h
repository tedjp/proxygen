#pragma once

#include <folly/SpookyHashV2.h>

#include <proxygen/httpserver/Filters.h>

namespace proxygen {

/**
 * Possible bugs:
 * - ETag doesn't vary depending on whether response is gzipped. [?]
 *   - Does that even matter?
 */

class AutoETag : public Filter {
public:
    AutoETag(RequestHandler *upstream);

    void onRequest(std::unique_ptr<HTTPMessage> headers) noexcept override;

    void sendHeaders(HTTPMessage& msg) noexcept override;
    void sendBody(std::unique_ptr<folly::IOBuf> body) noexcept override;
    void sendEOM() noexcept override;

    static bool etagMatches(const std::string& etag, const std::vector<std::string>& etags) noexcept;

private:
    // Request
    std::vector<std::string> if_none_match_;

    // Response
    HTTPMessage msg_;
    std::unique_ptr<folly::IOBuf> body_;

    folly::hash::SpookyHashV2 hasher_;
};

}
