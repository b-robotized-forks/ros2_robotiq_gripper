// Copyright (c) 2026 b»robotized group
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//    * Redistributions of source code must retain the above copyright
//      notice, this list of conditions and the following disclaimer.
//
//    * Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//
//    * Neither the name of the copyright holder nor the names of its
//      contributors may be used to endorse or promote products derived from
//      this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include <robotiq_driver/tcp_transport.hpp>
#include <robotiq_driver/driver_exception.hpp>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace robotiq_driver
{
TcpTransport::TcpTransport(const std::string& host, uint16_t port) : host_(host), port_(port)
{
  if (port == 0)
  {
    throw std::invalid_argument("tcp_port must be between 1 and 65535");
  }
}

TcpTransport::~TcpTransport()
{
  close();
}

void TcpTransport::open()
{
  if (is_open())
  {
    return;
  }
  // Numeric addresses avoid an unbounded DNS lookup during hardware activation.
  sockaddr_storage address{};
  auto* ipv4 = reinterpret_cast<sockaddr_in*>(&address);
  auto* ipv6 = reinterpret_cast<sockaddr_in6*>(&address);
  socklen_t length;
  if (inet_pton(AF_INET, host_.c_str(), &ipv4->sin_addr) == 1)
  {
    ipv4->sin_family = AF_INET;
    ipv4->sin_port = htons(port_);
    length = sizeof(sockaddr_in);
  }
  else if (inet_pton(AF_INET6, host_.c_str(), &ipv6->sin6_addr) == 1)
  {
    ipv6->sin6_family = AF_INET6;
    ipv6->sin6_port = htons(port_);
    length = sizeof(sockaddr_in6);
  }
  else
  {
    throw std::invalid_argument("tcp_host must be a numeric IPv4 or IPv6 address");
  }

  socket_ = ::socket(address.ss_family, SOCK_STREAM, 0);
  if (socket_ < 0)
  {
    fail("socket creation failed");
  }
  const int flags = fcntl(socket_, F_GETFL, 0);
  const int enabled = 1;
  if (flags < 0 || fcntl(socket_, F_SETFL, flags | O_NONBLOCK) < 0 || fcntl(socket_, F_SETFD, FD_CLOEXEC) < 0 ||
      setsockopt(socket_, IPPROTO_TCP, TCP_NODELAY, &enabled, sizeof(enabled)) < 0)
  {
    fail("socket configuration failed");
  }
  const auto deadline = std::chrono::steady_clock::now() + timeout_;
  if (::connect(socket_, reinterpret_cast<sockaddr*>(&address), length) < 0)
  {
    if (errno != EINPROGRESS)
    {
      fail("connect failed");
    }
    wait_ready(POLLOUT, deadline);
    int error = 0;
    socklen_t error_size = sizeof(error);
    if (getsockopt(socket_, SOL_SOCKET, SO_ERROR, &error, &error_size) < 0)
    {
      fail("checking connect failed");
    }
    if (error != 0)
    {
      fail(std::string("connect failed: ") + std::strerror(error));
    }
  }
}

bool TcpTransport::is_open() const
{
  return socket_ >= 0;
}

void TcpTransport::close()
{
  if (socket_ >= 0)
  {
    ::close(socket_);
    socket_ = -1;
  }
}

void TcpTransport::wait_ready(short events, Deadline deadline)
{
  while (true)
  {
    if (!is_open())
    {
      throw TransportException("TCP transport is closed");
    }
    const auto remaining = deadline - std::chrono::steady_clock::now();
    if (remaining <= Deadline::duration::zero())
    {
      fail("operation timed out");
    }
    const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(remaining).count();
    const int wait_ms = static_cast<int>(std::min<int64_t>(milliseconds + 1, std::numeric_limits<int>::max()));
    pollfd descriptor{ socket_, events, 0 };
    const int result = poll(&descriptor, 1, wait_ms);
    if (result < 0)
    {
      if (errno == EINTR)
      {
        continue;
      }
      fail("poll failed");
    }
    if (result == 0)
    {
      continue;
    }
    // Read buffered bytes even when the peer has also closed its end.
    if (descriptor.revents & events)
    {
      return;
    }
    fail("connection closed or socket error");
  }
}

std::vector<uint8_t> TcpTransport::read(size_t size)
{
  std::vector<uint8_t> data(size);
  size_t offset = 0;
  const auto deadline = std::chrono::steady_clock::now() + timeout_;
  while (offset < size)
  {
    wait_ready(POLLIN, deadline);
    const auto count = recv(socket_, data.data() + offset, size - offset, 0);
    if (count > 0)
    {
      offset += static_cast<size_t>(count);
    }
    else if (count == 0)
    {
      fail("peer disconnected during read");
    }
    else if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK)
    {
      fail("read failed");
    }
  }
  return data;
}

void TcpTransport::write(const std::vector<uint8_t>& data)
{
  size_t offset = 0;
  const auto deadline = std::chrono::steady_clock::now() + timeout_;
  while (offset < data.size())
  {
    wait_ready(POLLOUT, deadline);
    const auto count = send(socket_, data.data() + offset, data.size() - offset, MSG_NOSIGNAL);
    if (count > 0)
    {
      offset += static_cast<size_t>(count);
    }
    else if (count == 0 || (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK))
    {
      fail("write failed");
    }
  }
}

void TcpTransport::set_timeout(std::chrono::milliseconds timeout)
{
  if (timeout.count() <= 0)
  {
    throw std::invalid_argument("TCP timeout must be positive");
  }
  timeout_ = timeout;
}

std::chrono::milliseconds TcpTransport::get_timeout() const
{
  return timeout_;
}

std::string TcpTransport::get_host() const
{
  return host_;
}

uint16_t TcpTransport::get_port() const
{
  return port_;
}

void TcpTransport::fail(const std::string& message)
{
  // Never retry on a stream with an incomplete response: delayed bytes could be
  // mistaken for the next reply. Reconnect through the hardware lifecycle instead.
  close();
  throw TransportException("TCP " + host_ + ":" + std::to_string(port_) + ": " + message);
}
}  // namespace robotiq_driver
