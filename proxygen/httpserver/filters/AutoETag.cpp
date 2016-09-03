#include "AutoETag.h"

namespace proxygen {

AutoETag::AutoETag(RequestHandler *upstream):
    Filter(upstream),
    skip_(false),
    msg_(),
    body_()
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
}

void AutoETag::sendBody(std::unique_ptr<folly::IOBuf> body) noexcept {
    if (skip_) {
        downstream_->sendBody(std::move(body));
        return;
    }

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

    HTTPHeaders& headers = msg_.getHeaders();
    headers.set(HTTP_HEADER_ETAG, "\"Some placeholder\"");

    downstream_->sendHeaders(msg_);
    downstream_->sendBody(std::move(body_));
    downstream_->sendEOM();
}

}
