# 🌱 TwoEatOne · 二吃一 🌱

> *A wholesome pixel chess game for Windows 1.03*

---

## 🇨🇳 中文介绍

**运行在 Windows 1.03 上的治愈系像素棋局**

1985 年的某个下午，阳光透过 CRT 显示器的玻璃，照在那一抹经典的护眼绿上……

这就是 **《二吃一》** —— 一个只有 **4×4 格子** 的温柔小战场。你和对手各领四枚棋子，黑子沉稳，白子明亮，在底线排排站好。

### ✨ 规则超简单

轮流移动棋子（上下左右走一格），当两枚己方子棋**对着**对方一颗棋子时——

> **"啊呜~"** 一口吃掉！✨

把对方吃光光，或者让他无路可走，你就赢啦！

### 🎮 功能一览

| 功能 | 说明 |
|------|------|
| 🤖 **AI 模式** | 一个人无聊？打开 AI 模式，让电脑陪你玩~（它有点笨笨的，但很努力！） |
| 👥 **双人模式** | 和朋友面对面，轮流走棋，友谊的小船说翻就翻 |
| 💾 **存档/读档** | 下了一半要吃饭？存档！下次接着吃！ |
| 🎨 **换色护眼** | 觉得绿色看腻了？换个背景色，继续护眼（或者伤眼）！ |
| ↩️ **悔棋** | 悔棋是棋艺的一部分，按 **Q** 键就好啦~ |
| ⌨️ **快捷键** | N-新游戏 / Z-挑战模式 / S-存档 / L-载入 / Q-悔棋 |

### 🖥️ 系统要求

- **MS-DOS 3.33** + **Windows 1.03**
- 286 及以上 CPU
- 256KB 内存
- EGA/VGA 显示适配器

---

## 🇬🇧 English

A 4×4 grid, 4 pieces each. Move one step, sandwich an enemy to capture. Eat them all or leave them stuck to win!

Play vs AI, save games, undo (press **Q**), and pick your board color. Wholesome retro pixels for **Windows 1.03** — runs on a 286!

---

## 📸 游戏截图

![ai对弈](https://github.com/wlnklove/TwoEatOne-Windows1.03-game-/blob/main/TwoEatOne/img/1.gif)
![双人对弈](https://github.com/wlnklove/TwoEatOne-Windows1.03-game-/blob/main/TwoEatOne/img/4.gif)
![挑战模式](https://github.com/wlnklove/TwoEatOne-Windows1.03-game-/blob/main/TwoEatOne/img/2.gif)
![悔棋](https://github.com/wlnklove/TwoEatOne-Windows1.03-game-/blob/main/TwoEatOne/img/3.gif)
![EGA换色](https://github.com/wlnklove/TwoEatOne-Windows1.03-game-/blob/main/TwoEatOne/img/5.gif)
![256色](https://github.com/wlnklove/TwoEatOne-Windows1.03-game-/blob/main/TwoEatOne/img/000.gif)

---
### 构建好的二进制

| 文件 | 说明 |
|------|------|
|[v1.2](https://github.com/wlnklove/TwoEatOne-Windows1.03-game-/releases/download/TwoEatOne/TWOEAT.EXE) 、[V2.0](https://github.com/wlnklove/TwoEatOne-Windows1.03-game-/releases/download/v2.0/TWOEAT.EXE)| Windows 1.03 可执行文件（Medium 内存模型）|
---

## 🛠️ 从源码编译

### 开发环境

- **MSC 4.0** (Microsoft C Compiler)
- **Windows 1.03 SDK**
- **LINK 5.00.12**
- **MAKE** (MSC 4.0 自带)

### 编译步骤

```batch
REM 1. 编译各源文件
cl -AM -G2 -Gsw -c -Zpe twoeat.c
cl -AM -G2 -Gsw -c -Zpe board.c
cl -AM -G2 -Gsw -c -Zpe game.c
cl -AM -G2 -Gsw -c -Zpe ai.c
cl -AM -G2 -Gsw -c -Zpe dialogs.c
cl -AM -G2 -Gsw -c -Zpe save.c

REM 2. 编译资源
rc twoeat.rc

REM 3. 链接
link @twoeat.lnk
```

或使用 `build.bat` 一键编译。

### 项目文件

```
twoeat/
├── twoeat.h          # 公共头文件
├── twoeat.c          # 主程序（WinMain、窗口过程）
├── board.c           # 棋盘绘制与布局
├── game.c            # 游戏逻辑（走棋、吃子、悔棋）
├── ai.c              # AI 逻辑（Minimax + Alpha-Beta）
├── dialogs.c         # 对话框（设置、存档、载入）
├── save.c            # 存档/读档（文件 I/O）
├── twoeat.rc         # 资源脚本（菜单、对话框模板）
├── twoeat.def        # 模块定义文件
├── twoeat.lnk        # 链接响应文件
├── build.bat         # 构建脚本
└── makefile          # make 构建文件
```

---

## 📝 版本历史
### v2.0（2026-07-11

- **增加脏矩形模式**： 三个模式gdi绘制、小位图、脏更新！若出现闪烁可以尝试此功能


### v1.2 (2026-07-05)

- **棋盘闪烁根治**：棋盘区域双缓冲 + 边矩挖洞技术
- **新增亮绿颜色模式**：EGA 绿 / 深绿 / 亮绿 三色可选
- **悔棋逻辑**：用户手动修复，记录存档语义匹配的教训

### v1.1 (2026-07-04)

- **删除存档确认**：防止误删
- **持久化双缓冲**：位图只创建一次，复用到窗口大小变化
- **悔棋逻辑重写**：AI 模式悔 2 步，双人模式悔 1 步
- **防 AI 偷步**：悔棋前 KillTimer

### v1.0 (2026-07-03)

- 首个正式版
- AI 对战 / 双人对弈
- 存档 / 读档 / 删除
- 先后手设置、颜色模式
- 键盘快捷键

---

## 🐛 Bug 修复史

| Bug | 现象 | 根因 | 修复 |
|-----|------|------|------|
| 闪烁 | 走棋时棋盘闪一下 | `FillRect` 全屏刷绿 + `BitBlt` 两步之间屏幕刷新 | 边矩挖洞：背景只画边矩，棋盘区域不动 |
| DS 错误 | 弹窗只能开一次、设置不生效、偶发卡死 | 对话框过程 DS 指向错误段 | `MakeProcInstance` 创建 thunk |
| 整数溢出 | 点击底部棋子选中顶部棋子 | `dx*dx` 16 位 int 溢出 | 改用 `long`（32 位）|
| 悔棋 | AI 重走同一手棋 | 悔棋后 currentPlayer = BLACK | 强制设为 WHITE（玩家回合）|

---

## 🙏 致谢

- **glm**：技术协作、Bug 分析、代码审查
- **ima**：审核api函数是否符合Windows1.03标准
- **Microsoft Windows 1.03 SDK**：1985 年的经典平台
- **DOSBox**：让 2026 年的我们还能玩到 1985 年的系统

---

## 🐛关于平台的说明与当下问题

- 本软件专门给Windows1.x编写，Windows2.x也可以运行！Windows3以上请使用兼容模式运行~
- 若要使用Windows10/11平台运行可以使用otvdm兼容运行时！
- **Windows1 256色** ：在im1024显示卡下游戏是闪烁的可以使用脏矩形模式减轻但不能消除闪烁！！！
- **winddows1 16色 800*600 VESA驱动**：在VESA驱动下会发声棋盘闪烁请减小棋盘视口大小可消除！！！
- **Windows2 256色** ：在8514/a 256色模式下全屏或者大视口的情况下点击棋子、走棋会很慢，可以减小棋盘视口消除该现象！！！
- **EGA VGA** ：官方驱动下运行正常，有问题可以反馈！！！

## English Translation (Accurate, Retro Windows Tech Terminology)
- This software is developed exclusively for Windows 1.x, and it is also compatible with Windows 2.x! For Windows 3 and later versions, please run it in compatibility mode.
- To run this program on Windows 10 / 11, use the OTVDM compatibility runtime.
- **Windows 1 256-color mode**: The game exhibits screen flickering when using an IM1024 graphics card. Dirty rect rendering can reduce flickering but cannot eliminate it entirely.
- **Windows 1 16-color 800×600 VESA driver**: Severe board flickering occurs under VESA drivers. Resizing the game viewport to a smaller dimension will resolve this issue.
- **Windows 2 256-color mode**: Large or full-screen viewports with the 8514/a 256-color driver cause lag when clicking pieces or executing moves. Reducing the board viewport size fixes this slowdown.
- **EGA / VGA**: The program runs normally with official display drivers. Feel free to submit feedback if you encounter any issues.

## 📜 许可证

本项目为学习研究目的开发，遵循 MIT 许可证。

> *"在 Windows 1.03 上玩到了二吃一，这件事确实做到了。"*
---

*最后更新：2026-07-05*
*从 2026 年 6 月开始开发，历时约一个月，历经多轮 Debug。*
