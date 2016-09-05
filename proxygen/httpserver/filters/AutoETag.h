#pragma once

#include <folly/SpookyHashV2.h>

#include <proxygen/httpserver/Filters.h>
#include <proxygen/httpserver/RequestHandlerFactory.h>

namespace proxygen {

/**
 * AutoETag generates ETags for responses and sends back 304 Not Modified
 * responses to requestors who already have an up-to-date copy of the response.
 * This can save bandwidth and reduce round-trip-time when reloading an object
 * that is unchanged.
 *
 * It operates by caching the entire response in memory and hashing it before
 * sending the response. If your objects are too big for that to be reasonable,
 * it's best to calculate your own ETag for the object and send that. If you
 * have already set an ETag header by the time the AutoETag filter receives the
 * response headers, it will patch itself out of the response chain and have no
 * effect.  FIXME: Maybe callers still want the If-None-Match behavior!!!
 * - Bring back the skip variable I suppose.
 *
 * Usage: There are two ways to use the AutoETag filter. Either on a
 * per-handler basis, or globally for all handlers.
 *
 * TODO: Examples.
 *
 * BUGS:
 * - Unknown interaction with the ZlibServerFilter. The ZlibServerFilter
 *   should probably be run first.
 * - Except that's probably not doable with the global self-registering one.
 * - Maybe that doesn't matter.
 * - Maybe fixable with weak ETags or a known format for gzipped equivalents.
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

class AutoETagFilterFactory : public RequestHandlerFactory {
public:
    void onServerStart(folly::EventBase* evb) noexcept override {}
    void onServerStop() noexcept override {}

    RequestHandler* onRequest(RequestHandler* h, HTTPMessage *msg) noexcept override {
        return new AutoETag(h);
    }
};

}
