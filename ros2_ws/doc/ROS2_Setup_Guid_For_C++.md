# ROS 2 Humble 開發環境建構指南 (macOS + Docker + VS Code)

此文件僅紀錄如何在 macOS 環境下，透過 Docker 容器建構標準化的 ROS 2 Humble 開發環境，確保開發環境與宿主主機隔離且易於移轉。

---

## 第一階段：準備工作 (macOS 本機)

### 1. 安裝必要軟體

* **VS Code**: 主要編輯器。
* VS Code 擴充功能(外掛)：左側工具欄點擊  **Extensions** 搜尋安裝
  * **Dev Containers** (由 Microsoft 發行)
  * **Gemini Code Assist** (Google AI)
* **Docker Desktop**: 負責運行 Linux 虛擬環境。
  * 安裝後請確保 Docker Desktop 已啟動 (狀態欄顯示綠色鯨魚)。

### 2. 預載 ROS 2 映像檔 (本地優先原則)

在 Mac 終端機執行以下指令，預先下載 ROS 2 映像檔

```bash
docker pull ros:humble
```

這能避免 VS Code 在啟動容器時因網路問題導致建立失敗。如果沒有先下載，VS Code 在啟動時也會自動下載，但速度會慢很多。所以先下載好的動作，讓 VS Code 啟動環境的速度變成了「秒開」。

## 第二階段：建立工作空間 (Workspace)

### 1. 建立專案資料夾

在 Mac 上建立專案目錄：

```bash
mkdir -p ~/ros2Projects/ros2_ws
cd ~/ros2Projects/ros2_ws
```

### 2. 配置 Dev Container

在 `ros2_ws` 資料夾內建立 `.devcontainer` 目錄，並建立 `devcontainer.json` 檔案。主要是讓 VS Code 自動構建一個專屬的虛擬 Linux 開發環境：

- **環境一致性**
  - 無論在哪一台電腦，只要透過 Docker 打開這個資料夾，開發環境（作業系統版本、工具、編譯器）都會一樣
- **自動化環境建置**
  - 當透過 VS Code 開啟 devcontainer 時，會自動解析 `devcontainer.json` 並執行預設的 Linux 環境建置指令（如 `postCreateCommand` 中列出的套件安裝與初始化），確保開發環境「開箱即用」。
- **主機解耦**
  - 開發所需的工具全部留在容器內，不會弄髒本機系統。

**檔案內容範例：**

```json
{
    // Dev Container 的名稱，將顯示在 VS Code 遠端開發介面中。
    "name": "ROS 2 Humble Development",
    // 使用的 Docker 映像檔。這裡指定了 ROS 2 Humble 的官方映像檔。
    "image": "ros:humble",
    // 自訂 VS Code 設定，例如安裝擴充功能。
    "customizations": {
        "vscode": {
            "extensions": [
                // ROS 擴充功能，提供 ROS 開發工具和功能。
                "ms-iot.vscode-ros",
                // C/C++ 工具擴充功能包，包含 IntelliSense、偵錯等。
                "ms-vscode.cpptools-extension-pack",
                // Python 擴充功能，用於 Python 開發。
                "ms-python.python",
                // Gemini Code Assist 擴充功能。
                "Google.geminicodeassist"
            ]
        }
    },
    // 指定在容器中使用的使用者。這裡設定為 root。
    "remoteUser": "root",
    // 容器建立後執行的命令。這裡更新了 apt 套件列表，安裝了 Python pip 和 ROS Humble 範例介面，
    // 更新了 rosdep，並將 ROS 環境設定腳本加入到 root 使用者的 .bashrc 中。
    "postCreateCommand": "apt-get update && apt-get install -y python3-pip ros-humble-example-interfaces && rosdep update && echo \"source /opt/ros/humble/setup.bash\" >> /root/.bashrc"
}

```

**關鍵設定說明：**

* **`image`**: 直接指定 `ros:humble`，確保不同裝置時，系統環境保持一致。
* **`postCreateCommand`**: 
  * `apt-get update`
    * 更新 Ubuntu 系統的 apt 套件，確保後續安裝的是最新版本。
  * `python3-pip`
    * 安裝 Python 的套件管理工具 `pip`。在開發 ROS 2 時，經常會用到一些額外的 Python 函式庫，預設的映像檔可能沒有包含它。`-y` 表示自動同意安裝。
  * `ros-humble-example-interfaces`
    * ros2 Humble 內建的操作範例
  * `rosdep update`
    * 初始化 ROS 的依賴管理工具。這對於後續你要編譯（Build）別人的專案或自己的專案時，自動安裝所需的系統元件非常重要。
  * `echo \"source /opt/ros/humble/setup.bash\" >> /root/.bashrc`
    * 在 Linux 中，每次開啟新的終端機視窗時都會自動執行 `.bashrc`，故把「載入 ROS 2 環境」的指令寫進 `/root/.bashrc` 這個檔案，這樣以後打開終端機就能直接使用 `ros2` 指令。不用再每次開終端機都要手動輸入 source 指令
    * 另外也是**為了解決遇到 `ros2: command not found` 問題**。

## 第三階段：進入 ROS 2 開發環境與驗證

1. 用 VS Code 開啟 `ros2_ws` 資料夾。

2. 點擊左下角藍色圖標 (><)，選擇 **「Reopen in Container」**。

3. **驗證**：打開 VS Code 內建終端機，輸入：
   
   > **注意**：進入容器後，終端機運行的是 Linux 系統。Mac 專案資料夾會被自動掛載到容器內的 `/workspaces/ros2_ws` 路徑下。
   > 之後所有的指令都應在此路徑（或其子目錄）執行。
   
   ```bash
   ros2 pkg list
   ```
   
   若能看到套件清單，代表環境已就緒。

## 第四階段：開發流程

### 1. 建立原始碼目錄

在 ROS 2 的規範中，所有的原始碼 Package 都必須放在 **`src`** 資料夾內。

*此時終端機路徑應為 `/workspaces/ros2_ws`*

```bash
mkdir src
```

### 2. 建立專案：功能套件 (Package)

可將「Package」視為一個邏輯單元的小專案，如：控制輪子轉動、控製導航路徑都可以分別是一個 Package 。使用 ros2 提供的「Package 模板指令」 **`ros2 pkg create`**，建立對應的程式碼 Package 架構

#### C++ 範例

```bash
cd /workspaces/ros2_ws/src
ros2 pkg create --build-type ament_cmake --license Apache-2.0 my_cpp_pkg
```

- **`ros2 pkg create`**：ROS 2 的 Package 產生器。
- **`--build-type ament_cmake`**：指定這是一個 **C++** Package（使用 CMake 工具編譯）。
- **`--license Apache-2.0`**：設定授權協議（這是開源專案的標準作法）。
- **`my_cpp_pkg`**：這是 Package 名稱。

*提示：建議在建立時就加上 `--dependencies rclcpp`，這樣系統會自動幫你配置好依賴關係，省去後續手動修改 `package.xml` 的麻煩（詳見第四階段）。*

##### 指令產出的檔案結構

* **`CMakeLists.txt`**：編譯規則腳本，定義如何編譯程式與安裝路徑。
* **`package.xml`**：套件的「身分證」，定義套件名稱、版本、作者及最重要的「依賴關係」。
* **`src/`**：存放 C++ 原始碼 (.cpp) 的地方。
* **`include/`**：存放標頭檔 (.hpp) 的地方。

#### Python 範例

```bash
cd /workspaces/ros2_ws/src
ros2 pkg create --build-type ament_python --license Apache-2.0 my_py_pkg
```

- **`--build-type ament_python`**：指定這是一個 **Python** Package。
- **`my_py_pkg`**：這是 Package 名稱。

##### 指令產出的檔案結構

* **`package.xml`**：套件資訊與依賴宣告。
* **`setup.py`**：Python 套件的安裝與編譯設定檔。
* **`setup.cfg`**：定義安裝時的相關參數。
* **`my_py_pkg/`**：存放 Python 腳本的目錄（內含 `__init__.py`）。
* **`resource/`**：環境標記檔案存放地。
* **`test/`**：存放測試腳本的目錄。

### 3. 撰寫 C++ 範例程式碼

在功能套件的 `src` 目錄下建立 `hello_node.cpp`，內容如下：

```cpp
#include "rclcpp/rclcpp.hpp"

int main(int argc, char ** argv) {
  rclcpp::init(argc, argv);
  auto node = rclcpp::Node::make_shared("hello_node");
  RCLCPP_INFO(node->get_logger(), "Hello ROS 2 from C++!");
  rclcpp::shutdown();
  return 0;
}
```

### 4. 修改編譯配置 (C++ 專用)

為了讓系統知道如何編譯並執行剛剛寫好的程式碼，我們需要修改專案產出的兩個關鍵設定檔：

#### A. 修改 package.xml (宣告依賴)

我們需要在 `package.xml` 內加入 `rclcpp` 的依賴宣告，這樣系統編譯時才會去找對應的函式庫。

請在 `<export>` 標籤上方加入以下內容：

```xml
<depend>rclcpp</depend>
```

*提示：如果建立 Package 時直接使用 `--dependencies rclcpp`，系統會自動幫你寫好這行。*

#### B. 修改 CMakeLists.txt (設定編譯規則)

在 `CMakeLists.txt` 設定編譯規則與安裝路徑。

```cmake
# 在 CMakeLists.txt 的 find_package 下方加入
add_executable(hello_exe src/hello_node.cpp)
ament_target_dependencies(hello_exe rclcpp)

install(TARGETS
  hello_exe
  DESTINATION lib/${PROJECT_NAME}
)
```

### 5. 編譯程式碼工作空間 (Build)

在 ROS 2 裡，需要透過編譯工具將所有設定檔、路徑與程式進行「整合安裝」。

回到工作空間根目錄執行：

```bash
cd /workspaces/ros2_ws
colcon build

# 開發建議模式 (Python 專案強烈建議，修改免重新編譯)
colcon build --symlink-install
```

編譯完成後會產生 `build/`, `install/`, `log/` 三個資料夾。

- colcon build (標準模式)：
  - 行為：將編譯後的執行檔與資源檔案「複製」到 install 資料夾。
  - 缺點：如果你修改了非編譯檔案（如 Python 腳本或 Launch 檔案），必須重新執行 build 才能讓修改生效。
- colcon build --symlink-install (開發推薦模式)：
  - 行為：使用「符號連結 (Symbolic Link)」指向原始碼路徑，而不是複製檔案。
  - 優點：修改 Python 程式、Launch 檔、YAML 設定檔後，不需要重新編譯即可直接運行。
  - 注意：修改 C++ 程式原始碼 (.cpp) 後，不論哪種模式都必須重新編譯。

### 6. 載入當前工作空間 (Source)

這是最重要的一步，每當編譯完新的 Package，必須執行：

```bash
source install/setup.bash
```

*提示：如果不想每次手動 source，也可以手動將這行加進 `/root/.bashrc`。*

### 7. 執行節點 (Run)

使用 **`ros2 run`** 指令來啟動你編譯好的程式

```bash
cd /workspaces/ros2_ws
ros2 run my_cpp_pkg hello_exe
```

## 修正建議與注意事項

- **路徑一致性**
  
  - 在容器內部，你的預設路徑會是 `/workspaces/ros2_ws`。請確保所有 `colcon` 指令都在此路徑下執行。

- **rosdep 延伸**
  
  - 未來若專案變大，建議在 `postCreateCommand` 加入 `rosdep install --from-paths src --ignore-src -y` 來自動安裝 Package 所需的依賴。

- **GPU 加速**
  
  - macOS 的 Docker 目前不支援 NVIDIA GPU (CUDA)，若需運行複雜模擬 (如 Gazebo) 效能會受限。

## 常見問題

- 自動安裝依賴
  
  - 若下載了他人的 Package，執行以下指令自動安裝缺少的系統套件，在 `postCreateCommand` 加入：
  
  ```bash
  rosdep install --from-paths src --ignore-src -y
  ```

- **編譯特定 Package**
  
  - 若專案很大，只想編譯其中一個：
  
  ```bash
  colcon build --packages-select my_cpp_pkg
  ```

- **檔案同步**
  
  - 在容器內對 `/workspaces/ros2_ws` 的任何修改都會同步回 Mac 本機磁碟，不會因關閉容器而消失。

- 什麼時候需要執行工作環境的 colcon build？
  
  - **必須執行**：新增 Package、修改 C++ 程式碼 (.cpp/.hpp)、修改 CMakeLists.txt 或 package.xml、新增任何檔案。
  - **不需執行 (若用 symlink)**：僅修改 Python 腳本、Launch 檔案或參數檔 (YAML) 時。

- source install/setup.bash 會執行幾次？
  
  - 每次開啟新的終端機視窗都必須執行一次。
  
  - 當你新增了新的功能套件 (Package)，也需要重新執行一次以重新掃描路徑。

- 需要下指令關閉 Docker 裡面的作業系統嗎？
  
  - 不需要。直接關閉 VS Code 視窗即可，容器會自動停止。

- 執行 **`ros2 run`** 時若出現 No executable found？
  
  - 檢查 CMakeLists.txt 內是否有寫 install(TARGETS ...) 指令
  
  - 檢查編譯後是否有執行 source install/setup.bash
  
  - 輸入 `ros2 pkg executables my_cpp_pkg` 檢查是否有列出執行檔