#include <cinttypes>

#include <folly/String.h>

#include "AutoETag.h"

namespace proxygen {

AutoETag::AutoETag(RequestHandler *upstream):
    Filter(upstream),
    skip_(false),
    msg_(),
    body_(),
    if_none_match_(),
    hasher_()
{
}

void AutoETag::onRequest(std::unique_ptr<HTTPMessage> headers) noexcept {
    const auto& h = headers->getHeaders();

    if_none_match_.reserve(h.getNumberOfValues(HTTP_HEADER_IF_NONE_MATCH));

    h.forEachValueOfHeader(HTTP_HEADER_IF_NONE_MATCH,
            [&if_none_match_ = this->if_none_match_](const std::string& value) -> bool {
                if (value.find(',') == std::string::npos) {
                    if_none_match_.push_back(value);
                } else {
                    std::vector<std::string> tags;
                    folly::split(std::string(","), value, tags, true);
                    for (const std::string& tag: tags) {
                        folly::StringPiece trimmed = folly::trimWhitespace(folly::StringPiece(tag));
                        if_none_match_.emplace_back(trimmed.begin(), trimmed.end());
                    }
                }
                return false;
            });

    upstream_->onRequest(std::move(headers));
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

    bool sendBody = true;

    if (etagMatchesIfNoneMatch(etag)) {
        // Send 304 Not Modified
        msg_.setStatusCode(304);
        sendBody = false;
    }

    headers.set(HTTP_HEADER_ETAG, etag);

    downstream_->sendHeaders(msg_);

    if (sendBody)
        downstream_->sendBody(std::move(body_));

    downstream_->sendEOM();
}

bool AutoETag::etagMatchesIfNoneMatch(const std::string& etag) const noexcept {
    for (const auto& header: if_none_match_) {
        if (header == etag || header == "*")
            return true;
    }

    return false;
}

}
