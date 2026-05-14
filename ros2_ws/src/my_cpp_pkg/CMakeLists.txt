cmake_minimum_required(VERSION 3.8)
project(my_cpp_pkg)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 尋找必要的功能包
find_package(ament_cmake REQUIRED)  # ament_cmake: ROS 2 的建置系統核心
find_package(rclcpp REQUIRED) # rclcpp: ROS 2 的 C++ 客戶端函式庫，提供 Node, Topic, Service 等核心功能
find_package(geometry_msgs REQUIRED)  # geometry_msgs: 提供幾何原始型別訊息（如 Point, Pose），用於座標傳輸
find_package(std_srvs REQUIRED) # std_srvs: 提供標準服務型別（如 Trigger），用於簡單的開關或觸發指令
find_package(rclcpp_action REQUIRED)  # rclcpp_action: 提供 Action 客戶端與伺服器端的 C++ 支援
find_package(example_interfaces REQUIRED) # example_interfaces: 包含範例用的 Action 定義（如專案中使用的 Fibonacci）

# 建立定位管理執行檔 (node_coord)
# 負責處理 AGV 的全域位置資訊發布，並提供路徑規劃的 Action 服務
add_executable(node_coord src/node_coord.cpp)
ament_target_dependencies(
  node_coord            # 目標執行檔
  rclcpp                # 核心通訊
  geometry_msgs         # 座標訊息
  std_srvs              # 重置服務
  rclcpp_action         # Action 框架
  example_interfaces    # Fibonacci 介面
)

# 建立馬達控制執行檔 (node_motor)
# 負責直接對接馬達硬體狀態（RPM, 溫度），並提供急停（Brake）服務
add_executable(node_motor src/node_motor.cpp)
ament_target_dependencies(
  node_motor            # 目標執行檔
  rclcpp                # 核心通訊
  geometry_msgs         # 馬達狀態訊息
  std_srvs              # 急停服務
)

# 安裝指令：將編譯好的執行檔放到 install 目錄，這樣 ros2 run 才能找到它
install(
  TARGETS
  node_coord
  node_motor
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
