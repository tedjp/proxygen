#pragma once

#include <folly/SpookyHashV2.h>

#include <proxygen/httpserver/Filters.h>

namespace proxygen {

class AutoETag : public Filter {
public:
    AutoETag(RequestHandler *upstream);
    void sendHeaders(HTTPMessage& msg) noexcept override;
    void sendBody(std::unique_ptr<folly::IOBuf> body) noexcept override;
    void sendEOM() noexcept override;

private:
    // XXX: Is it possible to patch ourself out of the response chain
    // for this particular response?!
    bool skip_;
    HTTPMessage msg_;
    std::unique_ptr<folly::IOBuf> body_;

    folly::hash::SpookyHashV2 hasher_;
};

}
