# ros2_robotiq_gripper

This repository contains the ROS 2 driver, controller and description packages for working with a Robotiq Gripper.

The goal is to support multiple Robotiq Grippers.

Initially this repo supported only the 2f-85 however we want to also support the e-pick and pull requests are welcome for other grippers.
- https://github.com/PickNikRobotics/ros2_epick_gripper



## Serial and TCP communication

Serial communication is the default:

```bash
ros2 launch robotiq_description robotiq_control.launch.py com_port:=/dev/ttyUSB0
```

A gripper on the UR tool RS-485 interface can be used over TCP through the
[UR Tool Communication Forwarder](https://github.com/UniversalRobots/Universal_Robots_ToolComm_Forwarder_URCap).
Configure tool voltage, baud rate, parity and stop bits on the robot. Then connect:

```bash
ros2 launch robotiq_description robotiq_control.launch.py \
  transport:=tcp tcp_host:=192.168.1.100 tcp_port:=54321
```

The data is raw Modbus RTU (with CRC) over TCP, not Modbus TCP.
No `socat` or `tool_communication.py` is needed on the PC.
Use only one client of the forwarder: disable `use_tool_communication` in the UR ROS driver
and the Robotiq Grippers URCap.

| Xacro / launch parameter | Default | Description |
| --- | --- | --- |
| `transport` | `serial` | `serial` or `tcp` |
| `tcp_host` | empty | Numeric IPv4 or IPv6 address of the robot (required for TCP) |
| `tcp_port` | `54321` | Port of the forwarder |
| `timeout` | `0.5` | Communication timeout in seconds |
| `com_port` | `/dev/ttyUSB0` | Serial device (ignored for TCP) |

The hardware component opens the connection in `on_configure`.
When the TCP connection is lost, `read()` returns an error and the component goes to `unconfigured`.
To reconnect, activate the component again:

```bash
ros2 control set_hardware_component_state <gripper_component_name> active
```

## Build status

Currently the `main` branch is used for all current releases: Humble, Iron and Rolling.
As this is not a core ROS 2 package API/ABI breakage is not guaranteed, it is done as best effort and takes into account maintenance costs.
This is not sponsored or maintained by Robotiq we try to keep everything on main to reduce maintenance overhead.


ROS2 Distro | Branch | Build status | Documentation | Released packages
:---------: | :----: | :----------: | :-----------: | :---------------:
**Rolling** | [`main`](https://github.com/PickNikRobotics/ros2_robotiq_gripper/tree/main) | [![Rolling Binary Build](https://github.com/PickNikRobotics/ros2_robotiq_gripper/actions/workflows/rolling-binary-build-main.yml/badge.svg?branch=main)](https://github.com/PickNikRobotics/ros2_robotiq_gripper/actions/workflows/rolling-binary-build-main.yml?branch=main) <br /> [![Rolling Semi-Binary Build](https://github.com/PickNikRobotics/ros2_robotiq_gripper/actions/workflows/rolling-semi-binary-build-main.yml/badge.svg?branch=main)](https://github.com/PickNikRobotics/ros2_robotiq_gripper/actions/workflows/rolling-semi-binary-build-main.yml?branch=main) | | [ros2_robotiq_gripper](https://index.ros.org/p/ros2_robotiq_gripper/github-PickNikRobotics-ros2_robotiq_grippper/#rolling)


ROS2 Distro | Branch | Build status | Documentation | Released packages
:---------: | :----: | :----------: | :-----------: | :---------------:
**Humble** | [`main`](https://github.com/PickNikRobotics/ros2_robotiq_gripper/tree/main) | [![Humble Binary Build](https://github.com/PickNikRobotics/ros2_robotiq_gripper/actions/workflows/humble-binary-build-main.yml/badge.svg?branch=main)](https://github.com/PickNikRobotics/ros2_robotiq_gripper/actions/workflows/humble-binary-build-main.yml?branch=main) <br /> [![Humble Semi-Binary Build](https://github.com/PickNikRobotics/ros2_robotiq_gripper/actions/workflows/humble-semi-binary-build-main.yml/badge.svg?branch=main)](https://github.com/PickNikRobotics/ros2_robotiq_gripper/actions/workflows/humble-semi-binary-build-main.yml?branch=main) | | [ros2_robotiq_gripper](https://index.ros.org/p/ros2_robotiq_gripper/github-PickNikRobotics-ros2_robotiq_grippper/#humble)


ROS2 Distro | Branch | Build status | Documentation | Released packages
:---------: | :----: | :----------: | :-----------: | :---------------:
**Iron** | [`main`](https://github.com/PickNikRobotics/ros2_robotiq_gripper/tree/main) | [![Iron Binary Build](https://github.com/PickNikRobotics/ros2_robotiq_gripper/actions/workflows/iron-binary-build-main.yml/badge.svg?branch=main)](https://github.com/PickNikRobotics/ros2_robotiq_gripper/actions/workflows/iron-binary-build-main.yml?branch=main) <br /> [![Iron Semi-Binary Build](https://github.com/PickNikRobotics/ros2_robotiq_gripper/actions/workflows/iron-semi-binary-build-main.yml/badge.svg?branch=main)](https://github.com/PickNikRobotics/ros2_robotiq_gripper/actions/workflows/iron-semi-binary-build-main.yml?branch=main) | | [ros2_robotiq_gripper](https://index.ros.org/p/ros2_robotiq_gripper/github-PickNikRobotics-ros2_robotiq_grippper/#iron)

### Explanation of different build types

**NOTE**: There are three build stages checking current and future compatibility of the package.

[Detailed build status](.github/workflows/README.md)

1. Binary builds - against released packages (main and testing) in ROS distributions. Shows that direct local build is possible.

   Uses repos file: `$NAME$-not-released.<ros-distro>.repos`

1. Semi-binary builds - against released core ROS packages (main and testing), but the immediate dependencies are pulled from source.
   Shows that local build with dependencies is possible and if fails there we can expect that after the next package sync we will not be able to build.

   Uses repos file: `$NAME$.repos`

1. Source build - also core ROS packages are build from source. It shows potential issues in the mid future.
