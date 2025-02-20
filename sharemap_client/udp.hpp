#pragma once
#include <chrono>
#include <netdb.h>
#include <string>

namespace anysignal
{

class udp_sock
{
  public:
    udp_sock(void) = default;
    ~udp_sock(void);

    void bind(const std::string &url);
    void connect(const std::string &url);

    bool send_ready(const std::chrono::milliseconds timeout = {});
    bool recv_ready(const std::chrono::milliseconds timeout = {});

    int recv(void *const buff, const size_t length);
    int send(const void *const buff, const size_t length);

  private:
    int _sock{-1};
    static ::addrinfo get_addr_info(const std::string &url);
};

} // namespace anysignal

////////////////////////////////////////////////////////////////////////
// implementation details
////////////////////////////////////////////////////////////////////////

#include <cstring> //memset
#include <stdexcept>
#include <sys/select.h>
#include <sys/socket.h>
#include <tuple>
#include <unistd.h> //close

inline anysignal::udp_sock::~udp_sock(void)
{
    if (_sock != -1)
    {
        ::close(_sock);
    }
}

inline void anysignal::udp_sock::bind(const std::string &url)
{
    auto addrinfo = get_addr_info(url);
    _sock = ::socket(addrinfo.ai_family, addrinfo.ai_socktype, addrinfo.ai_protocol);
    if (_sock == -1)
    {
        throw std::runtime_error("failed to create socket for " + url);
    }

    if (::bind(_sock, addrinfo.ai_addr, addrinfo.ai_addrlen) != 0)
    {
        ::close(_sock);
        _sock = -1;
        throw std::runtime_error("failed to bind socket for " + url);
    }
}

inline void anysignal::udp_sock::connect(const std::string &url)
{
    auto addrinfo = get_addr_info(url);
    _sock = ::socket(addrinfo.ai_family, addrinfo.ai_socktype, addrinfo.ai_protocol);
    if (_sock == -1)
    {
        throw std::runtime_error("failed to create socket for " + url);
    }

    if (::connect(_sock, addrinfo.ai_addr, addrinfo.ai_addrlen) != 0)
    {
        ::close(_sock);
        _sock = -1;
        throw std::runtime_error("failed to connect socket for " + url);
    }
}

inline bool anysignal::udp_sock::send_ready(const std::chrono::milliseconds timeout)
{
    fd_set wfds;
    FD_ZERO(&wfds);
    FD_SET(_sock, &wfds);

    timeval tv{};
    tv.tv_sec = timeout.count() / 1000;
    tv.tv_usec = (timeout.count() % 1000) * 1000;

    int result = ::select(_sock + 1, nullptr, &wfds, nullptr, &tv);
    if (result == -1)
    {
        throw std::runtime_error("select failed on send_ready");
    }

    return result > 0;
}

inline bool anysignal::udp_sock::recv_ready(const std::chrono::milliseconds timeout)
{
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(_sock, &rfds);

    timeval tv{};
    tv.tv_sec = timeout.count() / 1000;
    tv.tv_usec = (timeout.count() % 1000) * 1000;

    int result = ::select(_sock + 1, &rfds, nullptr, nullptr, &tv);
    if (result == -1)
    {
        throw std::runtime_error("select failed on recv_ready");
    }

    return result > 0;
}

inline int anysignal::udp_sock::recv(void *const buff, const size_t length)
{
    if (_sock == -1)
    {
        throw std::runtime_error("recv failed: socket is not initialized");
    }

    return ::recv(_sock, buff, length, MSG_DONTWAIT);
}

inline int anysignal::udp_sock::send(const void *const buff, const size_t length)
{
    if (_sock == -1)
    {
        throw std::runtime_error("send failed: socket is not initialized");
    }

    return ::send(_sock, buff, length, MSG_DONTWAIT);
}

static inline std::tuple<std::string, std::string, std::string> anysignal_url_parse(const std::string &url_with_scheme)
{
    if (url_with_scheme.empty())
    {
        return {};
    }

    std::string scheme, url = url_with_scheme;
    auto scheme_end_pos = url_with_scheme.find("://");
    if (scheme_end_pos != std::string::npos)
    {
        scheme = url_with_scheme.substr(0, scheme_end_pos);
        url = url_with_scheme.substr(scheme_end_pos + 3);
    }

    auto open_bracket_pos = url.find("[");
    auto close_bracket_pos = url.find("]:");
    if (open_bracket_pos != std::string::npos and close_bracket_pos != std::string::npos and
        open_bracket_pos < close_bracket_pos)
    {
        auto host = url.substr(open_bracket_pos + 1, close_bracket_pos - open_bracket_pos - 1);
        return {scheme, host, url.substr(close_bracket_pos + 2)};
    }
    if (open_bracket_pos == std::string::npos and close_bracket_pos == std::string::npos)
    {
        auto colon_pos = url.find(":");
        if (colon_pos == std::string::npos)
            return {};
        return {scheme, url.substr(0, colon_pos), url.substr(colon_pos + 1)};
    }
    return {};
}

static inline std::tuple<::addrinfo, ::sockaddr_storage, ::socklen_t> anysignal_getaddrinfo(const std::string &node,
                                                                                            const std::string &service,
                                                                                            const addrinfo &hints)
{
    ::addrinfo *res{nullptr};
    ::addrinfo out_info{};
    ::sockaddr_storage out_addr{};
    ::socklen_t out_len{0};
    auto s = ::getaddrinfo(node.c_str(), service.c_str(), &hints, &res);
    if (s != 0)
    {
        return {out_info, out_addr, out_len};
    }
    for (auto i = res; i != nullptr; i = i->ai_next)
    {
        std::memcpy(&out_info, i, sizeof(out_info));
        std::memcpy(&out_addr, i->ai_addr, i->ai_addrlen);
        out_len = i->ai_addrlen;
    }
    ::freeaddrinfo(res);
    return {out_info, out_addr, out_len};
}

inline ::addrinfo anysignal::udp_sock::get_addr_info(const std::string &url)
{
    const auto [scheme, host, port] = anysignal_url_parse(url);
    ::addrinfo hint{};
    hint.ai_socktype = SOCK_DGRAM;
    const auto [addrinfo, sockaddr_data, addrlen] = anysignal_getaddrinfo(host, port, hint);
    if (addrlen == 0)
    {
        throw std::runtime_error("failed to getaddrinfo for " + url);
    }
    return addrinfo;
}
