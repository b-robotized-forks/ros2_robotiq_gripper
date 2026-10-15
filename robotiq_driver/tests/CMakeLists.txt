# Add support for GTest.
find_package(ament_cmake_gtest REQUIRED)
find_package(ament_cmake_gmock REQUIRED)
find_package(ament_lint_auto REQUIRED)

ament_lint_auto_find_test_dependencies()

# GMock throws an error if we don't switch off this option in tests.
if(CMAKE_CXX_COMPILER_ID MATCHES "(GNU|Clang)")
    add_compile_options(-Wno-sign-conversion)
endif()

###############################################################################
# test_robotiq_gripper_hardware_interface

ament_add_gmock(test_robotiq_gripper_hardware_interface
    test_robotiq_gripper_hardware_interface.cpp
)
target_include_directories(test_robotiq_gripper_hardware_interface
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(
    test_robotiq_gripper_hardware_interface
    robotiq_driver
    hardware_interface::hardware_interface
)

###############################################################################
# test_default_transport_factory

ament_add_gmock(test_default_transport_factory
    test_default_transport_factory.cpp
)
target_link_libraries(test_default_transport_factory robotiq_driver)

###############################################################################
# test_default_driver_factory

ament_add_gmock(test_default_driver_factory
    test_default_driver_factory.cpp
    mock/mock_driver.hpp
)
target_include_directories(test_default_driver_factory
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(test_default_driver_factory robotiq_driver)

###############################################################################
# test_crc_utils

ament_add_gmock(test_crc_utils
    test_crc_utils.cpp
)
target_include_directories(test_crc_utils
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(test_crc_utils robotiq_driver)

###############################################################################
# test_data_utils

ament_add_gmock(test_data_utils
    test_data_utils.cpp
)
target_include_directories(test_data_utils
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(test_data_utils robotiq_driver)

###############################################################################
# Raw TCP transport tests use a local loopback server; no robot required.
find_package(Threads REQUIRED)
ament_add_gtest(test_tcp_transport test_tcp_transport.cpp)
target_link_libraries(test_tcp_transport robotiq_driver Threads::Threads)
