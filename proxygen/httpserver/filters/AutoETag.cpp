#include <cinttypes>

#include <folly/String.h>

#include "AutoETag.h"

namespace proxygen {

AutoETag::AutoETag(RequestHandler *upstream):
    Filter(upstream),
    skip_(false),
    msg_(),
    body_(),
    hasher_()
{
}

void AutoETag::sendHeaders(HTTPMessage& msg) noexcept {
    HTTPHeaders& headers = msg.getHeaders();

    if (msg.getIsChunked() || headers.exists(HTTP_HEADER_ETAG)) {
        skip_ = true;
        downstream_->sendHeaders(msg);
        return;
    }

    msg_ = msg;

    hasher_.Init(0, 0);
}

void AutoETag::sendBody(std::unique_ptr<folly::IOBuf> body) noexcept {
    if (skip_) {
        downstream_->sendBody(std::move(body));
        return;
    }

    hasher_.Update(body->data(), body->length());

    if (body_)
        body_->appendChain(std::move(body));
    else
        body_ = std::move(body);
}

void AutoETag::sendEOM() noexcept {
    if (skip_) {
        downstream_->sendEOM();
        return;
    }

    uint64_t hash1, hash2;
    hasher_.Final(&hash1, &hash2);

    // ETags are always quoted-strings
    std::string etag = folly::stringPrintf("\"%016" PRIx64, hash1);
    folly::stringAppendf(&etag, "%016" PRIx64 "\"", hash2);

    HTTPHeaders& headers = msg_.getHeaders();
    headers.set(HTTP_HEADER_ETAG, etag);

    downstream_->sendHeaders(msg_);
    downstream_->sendBody(std::move(body_));
    downstream_->sendEOM();
}

}
