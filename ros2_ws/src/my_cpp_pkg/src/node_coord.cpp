#include <cstdio>
#include <chrono>
#include <string>
#include <random>
#include <thread>
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "std_msgs/msg/string.hpp"
#include "std_srvs/srv/trigger.hpp"
#include "geometry_msgs/msg/point.hpp"
#include "example_interfaces/action/fibonacci.hpp"

using namespace std::chrono_literals;

class AGV_Coord_Node : public rclcpp::Node {
public:
  using Fibonacci = example_interfaces::action::Fibonacci;
  using GoalHandleFibonacci = rclcpp_action::ServerGoalHandle<Fibonacci>;

  AGV_Coord_Node() : Node("node_coord") {
    // 初始化位置資訊 Topic (發布載具目前座標)
    publisher_ = this->create_publisher<geometry_msgs::msg::Point>("agv/topic/location", 10);

    // 定義多久發佈一次 topic
    timer_ = this->create_wall_timer(3000ms, std::bind(&AGV_Coord_Node::timer_callback, this));

    // 訂閱馬達狀態 (從 node_motor 接收轉速或狀態)
    motor_sub_ = this->create_subscription<geometry_msgs::msg::Point>(
      "agv/topic/motor_status", 10, std::bind(&AGV_Coord_Node::motor_callback, this, std::placeholders::_1));

    // 新增 Service：重置座標點 (Odometry Reset)
    reset_service_ = this->create_service<std_srvs::srv::Trigger>(
      "agv/service/reset_pose",
      std::bind(&AGV_Coord_Node::handle_reset, this, std::placeholders::_1, std::placeholders::_2));

    // 初始化 Action Server：模擬路徑規劃計算
    this->action_server_ = rclcpp_action::create_server<Fibonacci>(
      this,
      "action/plan_path",
      std::bind(&AGV_Coord_Node::handle_goal, this, std::placeholders::_1, std::placeholders::_2),
      std::bind(&AGV_Coord_Node::handle_cancel, this, std::placeholders::_1),
      std::bind(&AGV_Coord_Node::handle_accepted, this, std::placeholders::_1)
    );

    RCLCPP_INFO(this->get_logger(), "定位節點 (node_coord) 已啟動...");
  }

private:
  void motor_callback(const geometry_msgs::msg::Point::SharedPtr msg) const {
    RCLCPP_INFO(this->get_logger(), "[topic/motor_status] 接收馬達狀態: [左輪轉速: %.2f, 右輪轉速: %.2f]", msg->x, msg->y);
  }

  void handle_reset(const std::shared_ptr<std_srvs::srv::Trigger::Request> request,
                     std::shared_ptr<std_srvs::srv::Trigger::Response> response)
  {
    (void)request;
    RCLCPP_INFO(this->get_logger(), "[service/reset_pose] 執行座標重置程序...");
    count_ = 0;
    response->success = true;
    response->message = "座標與計數器已歸零";
  }

  void timer_callback() {
    auto message = geometry_msgs::msg::Point();
    message.x = dis_(gen_);
    message.y = dis_(gen_);
    message.z = 0.0;

    RCLCPP_INFO(this->get_logger(), "[topic/location] 發佈當前座標: [x: %.2f, y: %.2f], 累積里程計數: %zu", message.x, message.y, count_++);
    publisher_->publish(message);
  }

  rclcpp_action::GoalResponse handle_goal(
    const rclcpp_action::GoalUUID & uuid,
    std::shared_ptr<const Fibonacci::Goal> goal)
  {
    RCLCPP_INFO(this->get_logger(), "[action/plan_path] 收到路徑規劃請求: 預計產生 %d 個路點", goal->order);
    (void)uuid;
    return rclcpp_action::GoalResponse::ACCEPT_AND_EXECUTE;
  }

  rclcpp_action::CancelResponse handle_cancel(
    const std::shared_ptr<GoalHandleFibonacci> goal_handle)
  {
    RCLCPP_INFO(this->get_logger(), "[action/plan_path] 收到取消 Action 的請求");
    (void)goal_handle;
    return rclcpp_action::CancelResponse::ACCEPT;
  }

  void handle_accepted(const std::shared_ptr<GoalHandleFibonacci> goal_handle)
  {
    std::thread{std::bind(&AGV_Coord_Node::execute_action, this, std::placeholders::_1), goal_handle}.detach();
  }

  void execute_action(const std::shared_ptr<GoalHandleFibonacci> goal_handle)
  {
    RCLCPP_INFO(this->get_logger(), "[action/plan_path] 開始計算路徑點...");
    const auto goal = goal_handle->get_goal();
    auto feedback = std::make_shared<Fibonacci::Feedback>();
    auto & sequence = feedback->sequence;
    sequence.push_back(0);
    sequence.push_back(1);
    auto result = std::make_shared<Fibonacci::Result>();

    for (int i = 1; (i < goal->order) && rclcpp::ok(); ++i) {
      if (goal_handle->is_canceling()) {
        result->sequence = sequence;
        goal_handle->canceled(result);
        return;
      }
      sequence.push_back(sequence[i] + sequence[i - 1]);
      goal_handle->publish_feedback(feedback);
      std::this_thread::sleep_for(1s);
    }

    result->sequence = sequence;
    goal_handle->succeed(result);
    RCLCPP_INFO(this->get_logger(), "[action/plan_path] 路徑計算完成");
  }

  rclcpp::TimerBase::SharedPtr timer_;                                      
  rclcpp::Publisher<geometry_msgs::msg::Point>::SharedPtr publisher_;         
  rclcpp::Subscription<geometry_msgs::msg::Point>::SharedPtr motor_sub_;   
  rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr reset_service_;       
  rclcpp_action::Server<Fibonacci>::SharedPtr action_server_;               

  size_t count_ = 0;
  std::random_device rd_{};
  std::mt19937 gen_{rd_()};
  std::uniform_real_distribution<> dis_{0.0, 100.0};
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<AGV_Coord_Node>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}