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

#include <gtest/gtest.h>

#include <robotiq_driver/default_serial.hpp>
#include <robotiq_driver/default_transport_factory.hpp>
#include <robotiq_driver/tcp_transport.hpp>

#include <hardware_interface/hardware_info.hpp>

namespace robotiq_driver::test
{
/**
 * Here we test the transport factory with default parameters.
 */
TEST(TestDefaultTransportFactory, create_with_default_parameters)
{
  hardware_interface::HardwareInfo info;

  auto transport = DefaultTransportFactory().create(info);
  auto* serial = dynamic_cast<DefaultSerial*>(transport.get());
  ASSERT_NE(serial, nullptr);
  EXPECT_EQ(serial->get_port(), "/dev/ttyUSB0");
  EXPECT_EQ(serial->get_baudrate(), 115200u);
  EXPECT_EQ(serial->get_timeout(), std::chrono::milliseconds{ 500 });
  EXPECT_FALSE(serial->is_open());
}

/**
 * Here we test the transport factory with given serial parameters.
 */
TEST(TestDefaultTransportFactory, create_with_given_parameters)
{
  hardware_interface::HardwareInfo info;
  info.hardware_parameters.emplace("COM_port", "/dev/ttyUSB1");
  info.hardware_parameters.emplace("baudrate", "9600");
  info.hardware_parameters.emplace("timeout", "0.1");

  auto transport = DefaultTransportFactory().create(info);
  auto* serial = dynamic_cast<DefaultSerial*>(transport.get());
  ASSERT_NE(serial, nullptr);
  EXPECT_EQ(serial->get_port(), "/dev/ttyUSB1");
  EXPECT_EQ(serial->get_baudrate(), 9600u);
  EXPECT_EQ(serial->get_timeout(), std::chrono::milliseconds(100));
}

TEST(TestDefaultTransportFactory, creates_tcp_transport)
{
  hardware_interface::HardwareInfo info;
  info.hardware_parameters = { { "transport", "tcp" }, { "tcp_host", "127.0.0.1" }, { "timeout", "0.1" } };

  auto transport = DefaultTransportFactory().create(info);
  auto* tcp = dynamic_cast<TcpTransport*>(transport.get());
  ASSERT_NE(tcp, nullptr);
  EXPECT_EQ(tcp->get_host(), "127.0.0.1");
  EXPECT_EQ(tcp->get_port(), 54321);
  EXPECT_EQ(tcp->get_timeout(), std::chrono::milliseconds(100));
  EXPECT_FALSE(tcp->is_open());
}

TEST(TestDefaultTransportFactory, rejects_invalid_tcp_configuration)
{
  hardware_interface::HardwareInfo info;
  info.hardware_parameters = { { "transport", "udp" } };
  EXPECT_THROW(DefaultTransportFactory().create(info), std::invalid_argument);
  info.hardware_parameters = { { "transport", "tcp" } };
  EXPECT_THROW(DefaultTransportFactory().create(info), std::invalid_argument);
  info.hardware_parameters["tcp_host"] = "127.0.0.1";
  for (const auto* port : { "0", "65536", "-1", "54321oops", "" })
  {
    info.hardware_parameters["tcp_port"] = port;
    EXPECT_THROW(DefaultTransportFactory().create(info), std::invalid_argument);
  }
  info.hardware_parameters["tcp_port"] = "54321";
  for (const auto* timeout : { "0", "-1", "nan", "inf" })
  {
    info.hardware_parameters["timeout"] = timeout;
    EXPECT_THROW(DefaultTransportFactory().create(info), std::invalid_argument);
  }
}
}  // namespace robotiq_driver::test
