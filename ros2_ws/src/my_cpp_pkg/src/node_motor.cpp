#include <chrono>
#include <memory>
#include <string>
#include <random>
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "std_srvs/srv/trigger.hpp"
#include "geometry_msgs/msg/point.hpp"

using namespace std::chrono_literals;

class AGV_Motor_Node : public rclcpp::Node {
public:
  AGV_Motor_Node() : Node("node_motor") {
    publisher_ = this->create_publisher<geometry_msgs::msg::Point>("agv/topic/motor_status", 10);
    timer_ = this->create_wall_timer(3000ms, std::bind(&AGV_Motor_Node::timer_callback, this));

    subscription_ = this->create_subscription<geometry_msgs::msg::Point>(
      "agv/topic/location", 10, std::bind(&AGV_Motor_Node::location_callback, this, std::placeholders::_1));
    
    brake_service_ = this->create_service<std_srvs::srv::Trigger>(
      "agv/service/brake",
      std::bind(&AGV_Motor_Node::handle_brake, this, std::placeholders::_1, std::placeholders::_2));

    RCLCPP_INFO(this->get_logger(), "馬達控制節點 (node_motor) 已啟動...");
  }

private:
  void location_callback(const geometry_msgs::msg::Point::SharedPtr msg) const {
    RCLCPP_INFO(this->get_logger(), "接收到位置資訊: [x: %.2f, y: %.2f]", msg->x, msg->y);
  }

  void handle_brake(const std::shared_ptr<std_srvs::srv::Trigger::Request> request,
                     std::shared_ptr<std_srvs::srv::Trigger::Response> response)
  {
    (void)request;
    RCLCPP_INFO(this->get_logger(), "收到急停指令！馬達轉速歸零...");
    current_left_rpm_ = 0.0;
    current_right_rpm_ = 0.0;
    response->success = true;
    response->message = "馬達已急停";
  }

  void timer_callback() {
    auto message = geometry_msgs::msg::Point();

    if (current_left_rpm_ == 0.0 && current_right_rpm_ == 0.0) {
        message.x = 0.0;
        message.y = 0.0;
    } else {
        current_left_rpm_ = dis_rpm_(gen_);
        current_right_rpm_ = dis_rpm_(gen_);
        message.x = current_left_rpm_;
        message.y = current_right_rpm_;
    }
    message.z = dis_temp_(gen_);

    RCLCPP_INFO(this->get_logger(), "目前馬達狀態: [左輪RPM: %.2f, 右輪RPM: %.2f, 溫度: %.2f]", message.x, message.y, message.z);
    publisher_->publish(message);
  }

  rclcpp::TimerBase::SharedPtr timer_;                                      
  rclcpp::Publisher<geometry_msgs::msg::Point>::SharedPtr publisher_;         
  rclcpp::Subscription<geometry_msgs::msg::Point>::SharedPtr subscription_;   
  rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr brake_service_;       

  std::random_device rd_{};                               
  std::mt19937 gen_{rd_()};                               
  std::uniform_real_distribution<> dis_rpm_{0.0, 100.0};  
  std::uniform_real_distribution<> dis_temp_{20.0, 80.0}; 
  double current_left_rpm_ = 10.0; // 預設初始轉速，使其模擬運行
  double current_right_rpm_ = 10.0;
};

int main(int argc, char * argv[]) {
  rclcpp::init(argc, argv);
  auto node = std::make_shared<AGV_Motor_Node>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}