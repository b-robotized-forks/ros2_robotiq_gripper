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

#pragma once

#include <robotiq_driver/serial.hpp>

namespace robotiq_driver
{
/** Raw Modbus RTU byte transport over TCP, e.g. UR tool forwarding on port 54321.
 * Implements Serial to preserve the existing driver API. Baud rate is configured
 * on the remote RS-485 interface, not through this socket.
 */
class TcpTransport : public Serial
{
public:
  TcpTransport(const std::string& host, uint16_t port);
  ~TcpTransport() override;
  TcpTransport(const TcpTransport&) = delete;
  TcpTransport& operator=(const TcpTransport&) = delete;

  void open() override;
  [[nodiscard]] bool is_open() const override;
  void close() override;
  [[nodiscard]] std::vector<uint8_t> read(size_t size) override;
  void write(const std::vector<uint8_t>& data) override;
  // The inherited port accessor denotes the numeric remote IP address.
  void set_port(const std::string& host) override;
  [[nodiscard]] std::string get_port() const override;
  void set_timeout(std::chrono::milliseconds timeout) override;
  [[nodiscard]] std::chrono::milliseconds get_timeout() const override;
  // Unsupported: configure baud rate on the robot instead.
  void set_baudrate(uint32_t baudrate) override;
  [[nodiscard]] uint32_t get_baudrate() const override;

private:
  using Deadline = std::chrono::steady_clock::time_point;
  void wait_ready(short events, Deadline deadline);
  [[noreturn]] void fail(const std::string& message);

  std::string host_;
  uint16_t port_;
  std::chrono::milliseconds timeout_{ 500 };
  int socket_{ -1 };
};
}  // namespace robotiq_driver
