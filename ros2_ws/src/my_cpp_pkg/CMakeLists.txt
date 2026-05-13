cmake_minimum_required(VERSION 3.8)
project(my_cpp_pkg)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 尋找必要的功能包
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(std_srvs REQUIRED)
find_package(rclcpp_action REQUIRED)
find_package(example_interfaces REQUIRED)

# 建立載具 A 發布者執行檔 (agv_A_node)
add_executable(agv_A_node src/agv_A_node.cpp)
ament_target_dependencies(
  agv_A_node 
  rclcpp 
  std_msgs 
  geometry_msgs 
  std_srvs
  rclcpp_action
  example_interfaces
)

# 建立載具 B 訂閱者執行檔 (agv_B_node)
add_executable(agv_B_node src/agv_B_node.cpp)
ament_target_dependencies(
  agv_B_node 
  rclcpp 
  std_msgs 
  geometry_msgs 
  std_srvs
)

# 安裝指令：將編譯好的執行檔放到 install 目錄，這樣 ros2 run 才能找到它
install(
  TARGETS
  agv_A_node
  agv_B_node
  DESTINATION lib/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # comment the line when a copyright and license is added to all source files
  set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # comment the line when this package is in a git repo and when
  # a copyright and license is added to all source files
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

# 封裝此功能包
ament_package()
