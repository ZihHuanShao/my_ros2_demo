#include <chrono>
#include <memory>
#include <string>
#include <random>
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "std_srvs/srv/trigger.hpp"
#include "geometry_msgs/msg/point.hpp"

using namespace std::chrono_literals;

class AGV_B_Node : public rclcpp::Node {
public:
  AGV_B_Node() : Node("agv_B_node") {
    // 初始化 topic
    publisher_ = this->create_publisher<geometry_msgs::msg::Point>("agv_B_coord", 10);

    // 定義多久發佈一次 topic
    timer_ = this->create_wall_timer(5000ms, std::bind(&AGV_B_Node::timer_callback, this));

    // 訂閱載具 A 的 topic
    subscription_ = this->create_subscription<geometry_msgs::msg::Point>(
      "agv_A_coord", 10, std::bind(&AGV_B_Node::topic_callback, this, std::placeholders::_1));
    
    // 新增 Service：重啟
    reboot_service_ = this->create_service<std_srvs::srv::Trigger>(
      "agv_B/reboot",
      std::bind(&AGV_B_Node::handle_reboot, this, std::placeholders::_1, std::placeholders::_2));

    RCLCPP_INFO(this->get_logger(), "載具 B (agv_B_node) 已啟動，正於 /agv_B_coord 發布位置數據...");
  }

  ~AGV_B_Node() {
    RCLCPP_INFO(this->get_logger(), "載具 B (agv_B_node) 已收到關閉訊號，程式即將結束...");
  }

private:
  void topic_callback(const geometry_msgs::msg::Point::SharedPtr msg) const {
    RCLCPP_INFO(this->get_logger(), "接收到載具 A 座標: [x: %.2f, y: %.2f]", msg->x, msg->y);
  }

  void handle_reboot(const std::shared_ptr<std_srvs::srv::Trigger::Request> request,
                     std::shared_ptr<std_srvs::srv::Trigger::Response> response)
  {
    (void)request;
    RCLCPP_INFO(this->get_logger(), "收到重啟指令！正在將發布計數器歸零...");
    count_ = 0;
    response->success = true;
    response->message = "載具 B 已成功重置（計數器歸零）";
  }

  void timer_callback() {
    // 監控訂閱者狀態
    size_t current_sub_count = publisher_->get_subscription_count();
    if (current_sub_count != last_sub_count_) {
      if (current_sub_count > last_sub_count_) {
        RCLCPP_INFO(this->get_logger(), "偵測到新的訂閱者！目前連線數: %zu", current_sub_count);
      } else if (current_sub_count == 0) {
        RCLCPP_WARN(this->get_logger(), "訂閱者已斷開，目前無人監聽。");
      } else {
        RCLCPP_INFO(this->get_logger(), "訂閱者數量變更。目前連線數: %zu", current_sub_count);
      }
      last_sub_count_ = current_sub_count;
    }

    auto message = geometry_msgs::msg::Point();
    message.x = dis_(gen_);
    message.y = dis_(gen_);
    message.z = 0.0;

    RCLCPP_INFO(this->get_logger(), "我是載具 B，我的座標: [x: %.2f, y: %.2f], 計數: %zu", message.x, message.y, count_++);
    publisher_->publish(message);
  }

  rclcpp::TimerBase::SharedPtr timer_;                                      // 定期發佈 agv_B_coord topic
  rclcpp::Publisher<geometry_msgs::msg::Point>::SharedPtr publisher_;         // (發佈) agv_B_coord topic
  rclcpp::Subscription<geometry_msgs::msg::Point>::SharedPtr subscription_;   // (訂閱) agv_A_coord topic
  rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr reboot_service_;       // reboot service

  size_t count_ = 0;           // 累計發布次數計數器
  size_t last_sub_count_ = 0;  // 用於追蹤與比較訂閱者數量的變化，以偵測連線狀態

  // 隨機數生成相關組件，用於模擬載具移動座標
  std::random_device rd_{};                         // 隨機數種子源
  std::mt19937 gen_{rd_()};                         // 亂數生成引擎 (Mersenne Twister)
  std::uniform_real_distribution<> dis_{0.0, 100.0}; // 均勻分佈，定義座標產生的範圍 (0.0 到 100.0)
};

int main(int argc, char * argv[]) {
  rclcpp::init(argc, argv);
  
  auto node = std::make_shared<AGV_B_Node>();
  rclcpp::spin(node);
  
  rclcpp::shutdown();
  return 0;
}