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

#include <robotiq_driver/default_transport_factory.hpp>
#include <robotiq_driver/default_serial.hpp>
#include <robotiq_driver/tcp_transport.hpp>

#include <cmath>
#include <stdexcept>

#include <rclcpp/logging.hpp>

namespace robotiq_driver
{

const auto kLogger = rclcpp::get_logger("DefaultTransportFactory");

constexpr auto kUsbPortParamName = "COM_port";
constexpr auto kUsbPortParamDefault = "/dev/ttyUSB0";

constexpr auto kBaudrateParamName = "baudrate";
constexpr auto kBaudrateAddressParamDefault = 115200;

constexpr auto kTimeoutParamName = "timeout";
constexpr auto kTimeoutParamDefault = 0.5;

constexpr auto kTransportParamName = "transport";
constexpr auto kTransportParamDefault = "serial";

constexpr auto kTcpHostParamName = "tcp_host";

constexpr auto kTcpPortParamName = "tcp_port";
constexpr auto kTcpPortParamDefault = "54321";

std::unique_ptr<Transport> DefaultTransportFactory::create(const hardware_interface::HardwareInfo& info) const
{
  RCLCPP_INFO(kLogger, "Reading %s...", kTimeoutParamName);
  double timeout = info.hardware_parameters.count(kTimeoutParamName) ?
                       std::stod(info.hardware_parameters.at(kTimeoutParamName)) :
                       kTimeoutParamDefault;
  RCLCPP_INFO(kLogger, "%s: %fs", kTimeoutParamName, timeout);
  if (!std::isfinite(timeout) || timeout <= 0.0)
  {
    throw std::invalid_argument("timeout must be a positive number of seconds");
  }
  const auto timeout_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::duration<double>(timeout));

  const std::string transport = info.hardware_parameters.count(kTransportParamName) ?
                                    info.hardware_parameters.at(kTransportParamName) :
                                    kTransportParamDefault;
  RCLCPP_INFO(kLogger, "%s: %s", kTransportParamName, transport.c_str());

  if (transport == "tcp")
  {
    const std::string host =
        info.hardware_parameters.count(kTcpHostParamName) ? info.hardware_parameters.at(kTcpHostParamName) : "";
    if (host.empty())
    {
      throw std::invalid_argument("tcp_host is required for TCP transport");
    }
    const std::string port_text = info.hardware_parameters.count(kTcpPortParamName) ?
                                      info.hardware_parameters.at(kTcpPortParamName) :
                                      kTcpPortParamDefault;
    if (port_text.empty() || port_text.size() > 5 || port_text.find_first_not_of("0123456789") != std::string::npos)
    {
      throw std::invalid_argument("tcp_port must be between 1 and 65535");
    }
    const auto port = std::stoul(port_text);
    if (port == 0 || port > 65535)
    {
      throw std::invalid_argument("tcp_port must be between 1 and 65535");
    }
    RCLCPP_INFO(kLogger, "Using Modbus RTU over TCP at %s:%lu", host.c_str(), port);

    auto connection = std::make_unique<TcpTransport>(host, static_cast<uint16_t>(port));
    connection->set_timeout(timeout_ms);
    return connection;
  }
  if (transport != "serial")
  {
    throw std::invalid_argument("transport must be 'serial' or 'tcp'");
  }

  RCLCPP_INFO(kLogger, "Reading %s...", kUsbPortParamName);
  std::string usb_port = info.hardware_parameters.count(kUsbPortParamName) ?
                             info.hardware_parameters.at(kUsbPortParamName) :
                             kUsbPortParamDefault;
  RCLCPP_INFO(kLogger, "%s: %s", kUsbPortParamName, usb_port.c_str());

  RCLCPP_INFO(kLogger, "Reading %s...", kBaudrateParamName);
  uint32_t baudrate = info.hardware_parameters.count(kBaudrateParamName) ?
                          static_cast<uint32_t>(std::stoul(info.hardware_parameters.at(kBaudrateParamName))) :
                          kBaudrateAddressParamDefault;
  RCLCPP_INFO(kLogger, "%s: %dbps", kBaudrateParamName, baudrate);

  auto serial = std::make_unique<DefaultSerial>();
  serial->set_port(usb_port);
  serial->set_baudrate(baudrate);
  serial->set_timeout(timeout_ms);
  return serial;
}

}  // namespace robotiq_driver
