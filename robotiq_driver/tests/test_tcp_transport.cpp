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

#include <gtest/gtest.h>
#include <robotiq_driver/tcp_transport.hpp>
#include <robotiq_driver/driver_exception.hpp>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <memory>
#include <future>
#include <thread>
#include <stdexcept>

namespace robotiq_driver::test
{
using namespace std::chrono_literals;

class TcpTransportTest : public testing::Test
{
protected:
  void SetUp() override
  {
    listener_ = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_GE(listener_, 0);
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    ASSERT_EQ(bind(listener_, reinterpret_cast<sockaddr*>(&address), sizeof(address)), 0);
    ASSERT_EQ(listen(listener_, 1), 0);
    socklen_t length = sizeof(address);
    ASSERT_EQ(getsockname(listener_, reinterpret_cast<sockaddr*>(&address), &length), 0);
    connection_ = std::make_unique<TcpTransport>("127.0.0.1", ntohs(address.sin_port));
    connection_->open();
    peer_ = accept(listener_, nullptr, nullptr);
    ASSERT_GE(peer_, 0);
    const timeval timeout{ 3, 0 };
    ASSERT_EQ(setsockopt(peer_, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)), 0);
  }

  void TearDown() override
  {
    connection_.reset();
    if (peer_ >= 0)
    {
      ::close(peer_);
    }
    if (listener_ >= 0)
    {
      ::close(listener_);
    }
  }

  int listener_{ -1 };
  int peer_{ -1 };
  std::unique_ptr<TcpTransport> connection_;
};

TEST_F(TcpTransportTest, PreservesRawRtuBytes)
{
  const std::vector<uint8_t> request{ 0x09, 0x03, 0x07, 0xD0, 0x00, 0x01, 0x85, 0xCF };
  connection_->write(request);
  std::vector<uint8_t> received(request.size());
  ASSERT_EQ(recv(peer_, received.data(), received.size(), MSG_WAITALL), static_cast<ssize_t>(received.size()));
  EXPECT_EQ(received, request);
}

TEST_F(TcpTransportTest, CollectsFragmentedResponse)
{
  auto server = std::async(std::launch::async, [this] {
    const uint8_t first[]{ 9, 3 };
    const uint8_t second[]{ 2, 0, 255, 1, 2 };
    EXPECT_EQ(send(peer_, first, sizeof(first), MSG_NOSIGNAL), 2);
    std::this_thread::sleep_for(10ms);
    EXPECT_EQ(send(peer_, second, sizeof(second), MSG_NOSIGNAL), 5);
  });
  EXPECT_EQ(connection_->read(7), (std::vector<uint8_t>{ 9, 3, 2, 0, 255, 1, 2 }));
  server.get();
}

TEST_F(TcpTransportTest, DoesNotConsumeFollowingResponse)
{
  const uint8_t responses[]{ 1, 2, 3, 4, 5, 6 };
  ASSERT_EQ(send(peer_, responses, sizeof(responses), MSG_NOSIGNAL), 6);
  EXPECT_EQ(connection_->read(3), (std::vector<uint8_t>{ 1, 2, 3 }));
  EXPECT_EQ(connection_->read(3), (std::vector<uint8_t>{ 4, 5, 6 }));
}

TEST_F(TcpTransportTest, TimesOutAndClosesIncompleteStream)
{
  connection_->set_timeout(30ms);
  const uint8_t partial[]{ 9, 3 };
  ASSERT_EQ(send(peer_, partial, sizeof(partial), MSG_NOSIGNAL), 2);
  const auto start = std::chrono::steady_clock::now();
  EXPECT_THROW(connection_->read(7), DriverException);
  EXPECT_GE(std::chrono::steady_clock::now() - start, 20ms);
  EXPECT_LT(std::chrono::steady_clock::now() - start, 1s);
  EXPECT_FALSE(connection_->is_open());
  // A subsequent request must not silently retry against the stale connection.
  EXPECT_THROW(connection_->write({ 1, 2 }), DriverException);
}

TEST_F(TcpTransportTest, DetectsPeerDisconnect)
{
  ::close(peer_);
  peer_ = -1;
  EXPECT_THROW(connection_->read(1), DriverException);
  EXPECT_FALSE(connection_->is_open());
}

TEST_F(TcpTransportTest, ReadsBufferedResponseBeforeEndOfStream)
{
  const uint8_t response[]{ 1, 2, 3 };
  ASSERT_EQ(send(peer_, response, sizeof(response), MSG_NOSIGNAL), 3);
  ::close(peer_);
  peer_ = -1;
  EXPECT_EQ(connection_->read(3), (std::vector<uint8_t>{ 1, 2, 3 }));
  EXPECT_THROW(connection_->read(1), DriverException);
}

TEST_F(TcpTransportTest, HandlesLargeWrites)
{
  connection_->set_timeout(2s);
  std::vector<uint8_t> data(8 * 1024 * 1024);
  for (size_t i = 0; i < data.size(); ++i)
  {
    data[i] = static_cast<uint8_t>(i);
  }
  auto server = std::async(std::launch::async, [this, &data] {
    std::vector<uint8_t> received(data.size());
    size_t offset = 0;
    while (offset < received.size())
    {
      const auto count = recv(peer_, received.data() + offset, received.size() - offset, 0);
      if (count <= 0)
      {
        break;
      }
      offset += static_cast<size_t>(count);
    }
    EXPECT_EQ(offset, data.size());
    EXPECT_EQ(received, data);
  });
  connection_->write(data);
  server.get();
}

TEST_F(TcpTransportTest, BoundsWriteWhenPeerDoesNotRead)
{
  connection_->set_timeout(30ms);
  const std::vector<uint8_t> data(16 * 1024 * 1024, 0xAB);
  const auto start = std::chrono::steady_clock::now();
  EXPECT_THROW(connection_->write(data), DriverException);
  EXPECT_LT(std::chrono::steady_clock::now() - start, 1s);
  EXPECT_FALSE(connection_->is_open());
}

TEST_F(TcpTransportTest, CloseIsIdempotentAndCanExplicitlyReopen)
{
  connection_->close();
  connection_->close();
  EXPECT_FALSE(connection_->is_open());
  EXPECT_THROW(connection_->read(1), DriverException);
  EXPECT_NO_THROW(connection_->open());
  EXPECT_TRUE(connection_->is_open());
}

TEST_F(TcpTransportTest, ReconnectsAfterConnectionLoss)
{
  ::close(peer_);
  peer_ = -1;
  EXPECT_THROW(connection_->read(1), DriverException);
  EXPECT_FALSE(connection_->is_open());

  connection_->open();
  peer_ = accept(listener_, nullptr, nullptr);
  ASSERT_GE(peer_, 0);
  const uint8_t response[]{ 1, 2, 3 };
  ASSERT_EQ(send(peer_, response, sizeof(response), MSG_NOSIGNAL), 3);
  EXPECT_EQ(connection_->read(3), (std::vector<uint8_t>{ 1, 2, 3 }));
}

TEST_F(TcpTransportTest, ValidatesSettings)
{
  EXPECT_THROW(connection_->set_timeout(0ms), std::invalid_argument);
  EXPECT_THROW(connection_->set_timeout(-1ms), std::invalid_argument);
  EXPECT_EQ(connection_->get_host(), "127.0.0.1");
}

TEST(TcpTransport, RejectsInvalidEndpoint)
{
  EXPECT_THROW((TcpTransport{ "127.0.0.1", 0 }), std::invalid_argument);
  TcpTransport connection("invalid-host", 54321);
  EXPECT_THROW(connection.open(), std::invalid_argument);
  EXPECT_FALSE(connection.is_open());
}

TEST(TcpTransport, RefusedConnectionClosesSocket)
{
  const int listener = socket(AF_INET, SOCK_STREAM, 0);
  ASSERT_GE(listener, 0);
  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  ASSERT_EQ(bind(listener, reinterpret_cast<sockaddr*>(&address), sizeof(address)), 0);
  socklen_t length = sizeof(address);
  ASSERT_EQ(getsockname(listener, reinterpret_cast<sockaddr*>(&address), &length), 0);
  // A bound but non-listening socket refuses incoming connections.
  TcpTransport connection("127.0.0.1", ntohs(address.sin_port));
  EXPECT_THROW(connection.open(), DriverException);
  EXPECT_FALSE(connection.is_open());
  ::close(listener);
}
}  // namespace robotiq_driver::test
