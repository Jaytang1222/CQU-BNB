# 炸弹人游戏

一个使用Qt开发的炸弹人游戏。

## 游戏特性

- 玩家和建筑方块体积为 4x4
- 玩家每次移动一个单位，移动间隔 75ms
- 炸弹爆炸持续 400ms
- 爆炸范围有特效显示
- 初始爆炸范围：上下左右各 1 个方块

## 操作说明

- **WASD** 或 **方向键**：移动玩家
- **空格键**：放置炸弹

## 如何运行

### 方法一：使用 Qt Creator

1. 打开 Qt Creator
2. 选择 **文件 → 打开文件或项目**
3. 选择项目根目录的 `CMakeLists.txt`
4. 配置构建套件（Kit）
5. 点击左下角的 **运行** 按钮（绿色三角形）或按 `Ctrl+R`

### 方法二：使用命令行

#### Windows (MinGW)

```bash
# 进入项目目录
cd QtGames

# 创建构建目录（如果不存在）
mkdir build
cd build

# 配置项目（假设Qt安装在默认位置）
cmake .. -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="C:/Qt/6.10.1/mingw_64"

# 编译
cmake --build .

# 运行
./QtGames.exe
```

#### Windows (MSVC)

```bash
cd QtGames
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -DCMAKE_PREFIX_PATH="C:/Qt/6.10.1/msvc2019_64"
cmake --build . --config Release
Release\QtGames.exe
```

## 项目结构

- `player.h/cpp` - 玩家类
- `block.h/cpp` - 方块类（墙和砖块）
- `bomb.h/cpp` - 炸弹类
- `gameengine.h/cpp` - 游戏引擎（游戏逻辑）
- `gamescene.h/cpp` - 游戏场景（QGraphicsScene）
- `mainwindow.h/cpp` - 主窗口
- `main.cpp` - 程序入口

## 系统要求

- Qt 5.15+ 或 Qt 6.x
- CMake 3.16+
- C++17 编译器
