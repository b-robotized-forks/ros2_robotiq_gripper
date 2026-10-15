// Copyright (c) 2023 PickNik, Inc.
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
//    * Neither the name of the {copyright_holder} nor the names of its
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

#include <chrono>
#include <cstdint>
#include <vector>

namespace robotiq_driver
{
/**
 * The driver sends and receives bytes through an implementation of the
 * transport interface, e.g. a serial port or a TCP socket.
 * We can mock the transport to check that a high level command
 * (e.g activate) is converted into the correct sequence of bytes
 * including CRC.
 */
class Transport
{
public:
  virtual ~Transport() = default;

  /**
   * Opens the connection, if it is not already open.
   */
  virtual void open() = 0;

  /**
   * Gets the open status of the connection.
   * @return Returns true if the connection is open, false otherwise.
   */
  [[nodiscard]] virtual bool is_open() const = 0;

  /** Closes the connection. */
  virtual void close() = 0;

  /**
   * Read a given amount of bytes.
   * @param size The number of bytes to read.
   * @return The sequence of bytes.
   */
  [[nodiscard]] virtual std::vector<uint8_t> read(size_t size) = 0;

  /**
   * Write a sequence of bytes.
   * @param data A vector containing data to be written.
   */
  virtual void write(const std::vector<uint8_t>& data) = 0;

  /**
   * Set read/write timeout in milliseconds.
   * @param timeout Timeout in milliseconds.
   */
  virtual void set_timeout(const std::chrono::milliseconds timeout) = 0;

  /**
   * Get read/write timeout in milliseconds.
   * @return Timeout in milliseconds.
   */
  [[nodiscard]] virtual std::chrono::milliseconds get_timeout() const = 0;
};
}  // namespace robotiq_driver
