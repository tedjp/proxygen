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

    bool etagMatchesIfNoneMatch(const std::string& etag) const noexcept;

private:
    // XXX: Is it possible to patch ourself out of the response chain
    // for this particular response?!
    bool skip_;
    HTTPMessage msg_;
    std::unique_ptr<folly::IOBuf> body_;
    std::vector<std::string> if_none_match_;

    folly::hash::SpookyHashV2 hasher_;
};

}
