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

class AGV_A_Node : public rclcpp::Node {
public:
  using Fibonacci = example_interfaces::action::Fibonacci;
  using GoalHandleFibonacci = rclcpp_action::ServerGoalHandle<Fibonacci>;

  AGV_A_Node() : Node("agv_A_node") {
    // 初始化 topic
    publisher_ = this->create_publisher<geometry_msgs::msg::Point>("agv_A_coord", 10);

    // 定義多久發佈一次 topic
    timer_ = this->create_wall_timer(2000ms, std::bind(&AGV_A_Node::timer_callback, this));

    // 訂閱載具 B 的 topic
    subscription_ = this->create_subscription<geometry_msgs::msg::Point>(
      "agv_B_coord", 10, std::bind(&AGV_A_Node::topic_callback, this, std::placeholders::_1));

    // 新增 Service：重啟
    reboot_service_ = this->create_service<std_srvs::srv::Trigger>(
      "agv_A/reboot",
      std::bind(&AGV_A_Node::handle_reboot, this, std::placeholders::_1, std::placeholders::_2));

    // 初始化 Action Server：費波那契數列計算
    this->action_server_ = rclcpp_action::create_server<Fibonacci>(
      this,
      "fibonacci",
      std::bind(&AGV_A_Node::handle_goal, this, std::placeholders::_1, std::placeholders::_2),
      std::bind(&AGV_A_Node::handle_cancel, this, std::placeholders::_1),
      std::bind(&AGV_A_Node::handle_accepted, this, std::placeholders::_1)
    );

    RCLCPP_INFO(this->get_logger(), "載具 A (agv_A_node) 已啟動，正於 /agv_A_coord 發布位置數據...");
  }

  ~AGV_A_Node() {
    RCLCPP_INFO(this->get_logger(), "載具 A (agv_A_node) 已收到關閉訊號，程式即將結束...");
  }

private:
  void topic_callback(const geometry_msgs::msg::Point::SharedPtr msg) const {
    RCLCPP_INFO(this->get_logger(), "接收到載具 B 座標: [x: %.2f, y: %.2f]", msg->x, msg->y);
  }

  void handle_reboot(const std::shared_ptr<std_srvs::srv::Trigger::Request> request,
                     std::shared_ptr<std_srvs::srv::Trigger::Response> response)
  {
    (void)request;
    RCLCPP_INFO(this->get_logger(), "收到重啟指令！正在將發布計數器歸零...");
    count_ = 0;
    response->success = true;
    response->message = "載具 A 已成功重置（計數器歸零）";
  }

  void timer_callback() {
    // Humble 版本的替代方案：檢查訂閱者數量是否與上次檢查時不同
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
    message.z = 0.0; // 平面移動，z 設為 0

    RCLCPP_INFO(this->get_logger(), "我是載具 A，我的座標: [x: %.2f, y: %.2f], 計數: %zu", message.x, message.y, count_++);
    publisher_->publish(message);
  }

  // Action: 處理接收到的目標請求
  rclcpp_action::GoalResponse handle_goal(
    const rclcpp_action::GoalUUID & uuid,
    std::shared_ptr<const Fibonacci::Goal> goal)
  {
    RCLCPP_INFO(this->get_logger(), "收到 Action 目標請求: 計算階數為 %d 的數列", goal->order);
    (void)uuid;
    return rclcpp_action::GoalResponse::ACCEPT_AND_EXECUTE;
  }

  // Action: 處理取消請求
  rclcpp_action::CancelResponse handle_cancel(
    const std::shared_ptr<GoalHandleFibonacci> goal_handle)
  {
    RCLCPP_INFO(this->get_logger(), "收到取消 Action 的請求");
    (void)goal_handle;
    return rclcpp_action::CancelResponse::ACCEPT;
  }

  // Action: 接受目標後開始執行
  void handle_accepted(const std::shared_ptr<GoalHandleFibonacci> goal_handle)
  {
    // 必須在獨立執行緒執行，避免阻塞 Executor
    std::thread{std::bind(&AGV_A_Node::execute_action, this, std::placeholders::_1), goal_handle}.detach();
  }

  // Action: 實際執行邏輯
  void execute_action(const std::shared_ptr<GoalHandleFibonacci> goal_handle)
  {
    RCLCPP_INFO(this->get_logger(), "開始執行 Fibonacci 計算...");
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
        RCLCPP_INFO(this->get_logger(), "Action 已取消");
        return;
      }
      sequence.push_back(sequence[i] + sequence[i - 1]);
      goal_handle->publish_feedback(feedback);
      RCLCPP_INFO(this->get_logger(), "發送 Feedback: 目前數列長度為 %zu", sequence.size());
      std::this_thread::sleep_for(1s);
    }

    result->sequence = sequence;
    goal_handle->succeed(result);
    RCLCPP_INFO(this->get_logger(), "Action 執行成功！");
  }

  rclcpp::TimerBase::SharedPtr timer_;                                      // 定期發佈 agv_A_coord topic
  rclcpp::Publisher<geometry_msgs::msg::Point>::SharedPtr publisher_;         // (發佈) agv_A_coord topic
  rclcpp::Subscription<geometry_msgs::msg::Point>::SharedPtr subscription_;   // (訂閱) agv_B_coord topic
  rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr reboot_service_;       // reboot service
  rclcpp_action::Server<Fibonacci>::SharedPtr action_server_;               // Fibonacci action

  size_t count_ = 0;           // 累計發布次數計數器
  size_t last_sub_count_ = 0;  // 用於追蹤與比較訂閱者數量的變化，以偵測連線狀態

  // 隨機數生成相關組件，用於模擬載具移動座標
  std::random_device rd_{};                         // 隨機數種子源
  std::mt19937 gen_{rd_()};                         // 亂數生成引擎 (Mersenne Twister)
  std::uniform_real_distribution<> dis_{0.0, 100.0}; // 均勻分佈，定義座標產生的範圍 (0.0 到 100.0)
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  
  auto node = std::make_shared<AGV_A_Node>();
  rclcpp::spin(node);

  rclcpp::shutdown();
  return 0;
}