/* ====================================================================
 * twoeatone.c - 主程序文件
 * 包含 WinMain、主窗口过程、初始化函数
 * 使用 K&R C 语法，适配 MSC 4.0 + Windows 1.03 SDK
 * ==================================================================== */

#include "twoeat.h"
#include <string.h>

/* --------------------------------------------------------------------
 * 全局变量定义
 * -------------------------------------------------------------------- */

HANDLE hInst;                    /* 应用程序实例句柄 */
HWND hWndMain = NULL;            /* 主窗口句柄 */
HWND hModelessDlg = NULL;        /* 当前无模式对话框 */

/* MakeProcInstance thunk 指针
 * Win 1.03 DS 问题修复：直接传函数指针给 CreateDialog/DialogBox 会导致
 * 对话框过程中 DS 指向错误段，全局变量读写命中随机内存。
 * MakeProcInstance 创建的 thunk 在调用回调前设置正确的 DS。
 *
 * 无模式对话框的 thunk 不释放（见 twoeat.h 注释）。
 * 模态对话框在 DialogBox 返回后 FreeProcInstance（此时回调已返回，安全）。 */
FARPROC lpProcAbout = NULL;
FARPROC lpProcHelp = NULL;
FARPROC lpProcSettings = NULL;
FARPROC lpProcSave = NULL;
FARPROC lpProcLoad = NULL;
FARPROC lpProcDirtyRect = NULL;

GameState gameState;             /* 游戏状态 */
BoardLayout boardLayout;         /* 棋盘布局 */
HistoryEntry history[MAX_HISTORY]; /* 悔棋历史 */
int historyCount = 0;            /* 历史记录数 */
int historyCurrent = 0;          /* 当前历史指针 */
HBRUSH hbrBackground = NULL;     /* 背景画刷 */
HCURSOR hCursorArrow = NULL;     /* 标准箭头光标 */
char szAppName[] = "TwoEatOne";  /* 窗口类名 */

/* 持久化双缓冲位图（非 static，供 dirtyrect.c 访问） */
HDC hBackDC = NULL;
HBITMAP hBackBitmap = NULL;
HBITMAP hBackOldBitmap = NULL;

/* --------------------------------------------------------------------
 * WinMain - 程序入口点
 * 
 * Windows 程序的入口点，相当于 DOS 程序的 main()。
 * 负责注册窗口类、创建主窗口、运行消息循环。
 * 
 * 参数：
 *   hInstance     - 当前实例句柄
 *   hPrevInstance - 前一个实例句柄（Windows 1.03 中可能为 NULL）
 *   lpCmdLine     - 命令行参数
 *   nCmdShow      - 窗口初始显示方式
 * 
 * 返回值：消息循环的退出码
 * -------------------------------------------------------------------- */
int PASCAL WinMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow)
HANDLE hInstance;
HANDLE hPrevInstance;
LPSTR lpCmdLine;
int nCmdShow;
{
    MSG msg;

    /* 注册窗口类（仅在第一个实例时注册） */
    if (!InitApplication(hInstance))
        return FALSE;

    /* 初始化实例，创建主窗口 */
    if (!InitInstance(hInstance, nCmdShow))
        return FALSE;

    /* 检测存档目录，尝试载入最新存档；若无存档则开始新游戏 */
    EnsureSaveDir();
    if (!LoadLatestSave()) {
        StartNewGame(MODE_NORMAL);
    }

    /* StartNewGame 在 UpdateWindow 之后才执行，棋盘数据已就位但屏幕画的还是空棋盘
     * 必须手动触发一次重绘，否则看不到棋子 */
    InvalidateRect(hWndMain, NULL, FALSE);

    /* ----------------------------------------------------------------
     * 消息循环
     *
     * Windows 1.03 的消息循环。GetMessage 从队列取消息；
     * 如果有无模式对话框打开，用 IsDialogMessage 让它处理对话框消息
     * （如 Tab 键切换焦点等），否则正常翻译和分发消息。
     * ---------------------------------------------------------------- */
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (hModelessDlg != NULL && IsDialogMessage(hModelessDlg, &msg))
            continue;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return msg.wParam;
}

/* --------------------------------------------------------------------
 * InitApplication - 注册窗口类
 * 
 * 在 Windows 1.03 中，窗口类需要在第一个实例时注册。
 * 后续实例共享已注册的类。
 * 
 * WNDCLASS 结构各字段说明：
 *   style        - 窗口类风格，CS_HREDRAW|CS_VREDRAW 表示窗口大小
 *                  变化时重绘整个客户区（棋盘需要随窗口缩放）
 *   lpfnWndProc  - 指向窗口过程函数的指针
 *   hInstance    - 所属实例
 *   hCursor      - 默认光标（箭头）
 *   hbrBackground - 背景画刷（NULL 表示自行处理 WM_ERASEBKGND）
 *   lpszMenuName - 菜单资源名
 *   lpszClassName- 窗口类名
 * -------------------------------------------------------------------- */
BOOL InitApplication(hInstance)
HANDLE hInstance;
{
    WNDCLASS wc;

    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = MainWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance, (LPSTR)"AppIcon");             /* Windows 1.03 对自定义图标支持有限 */
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;      /* 自行处理背景擦除 */
    wc.lpszMenuName = "MainMenu"; /* 对应 .rc 中的菜单资源名 */
    wc.lpszClassName = szAppName;

    return RegisterClass(&wc);
}

/* --------------------------------------------------------------------
 * InitInstance - 创建主窗口
 * 
 * 创建并显示主窗口。
 * 
 * CreateWindow 参数说明：
 *   lpszClassName - 已注册的窗口类名
 *   lpszWindowName- 窗口标题栏文字
 *   dwStyle       - 窗口风格
 *                   WS_TILEDWINDOW 包含标题栏、系统菜单、
 *                   最大化/最小化按钮、可调边框
 *   x, y          - 窗口初始位置（0,0）
 *   nWidth, nHeight- 窗口初始尺寸
 *   hwndParent    - 父窗口（无）
 *   hmenu         - 菜单（NULL 表示使用类菜单）
 *   hInstance     - 实例句柄
 *   lpvParam      - 额外参数（无）
 * -------------------------------------------------------------------- */
BOOL InitInstance(hInstance, nCmdShow)
HANDLE hInstance;
int nCmdShow;
{
    RECT rc;

    hInst = hInstance;

    /* 创建主窗口 */
    hWndMain = CreateWindow(
        szAppName,
        "Two Eat One",           /* 窗口标题 */
        WS_TILEDWINDOW,
        0, 0,
        480, 520,                /* 初始窗口大小 */
        NULL, NULL, hInstance, NULL);

    if (hWndMain == NULL)
        return FALSE;

    /* 创建初始背景画刷（默认暗绿，用户可在设置中切换） */
    hbrBackground = CreateSolidBrush(RGB(0, 128, 0));

    /* 显示并更新窗口 */
    ShowWindow(hWndMain, nCmdShow);
    UpdateWindow(hWndMain);

    return TRUE;
}

/* --------------------------------------------------------------------
 * MainWndProc - 主窗口过程
 * 
 * 处理所有发送给主窗口的消息。这是 Windows 程序的核心。
 * 每个消息通过 switch-case 分支处理。
 * 
 * 参数：
 *   hWnd    - 窗口句柄
 *   message - 消息类型
 *   wParam  - 消息参数1（16位）
 *   lParam  - 消息参数2（32位）
 * -------------------------------------------------------------------- */
LONG FAR PASCAL MainWndProc(hWnd, message, wParam, lParam)
HWND hWnd;
unsigned message;
WORD wParam;
LONG lParam;
{
    HDC hdc;
    PAINTSTRUCT ps;
    RECT rc;

    switch (message) {

    /* ----------------------------------------------------------------
     * WM_CREATE - 窗口创建时
     * 
     * 在系统菜单中添加分隔符和"关于"选项。
     * GetSystemMenu 获取系统菜单句柄，ChangeMenu 追加菜单项。
     * Win 1.03 没有 AppendMenu，用 ChangeMenu + MF_APPEND 代替。
     * MF_SEPARATOR 表示分隔线，MF_STRING 表示文字菜单项。
     * ---------------------------------------------------------------- */
    case WM_CREATE:
    {
        HMENU hSysMenu;
        HMENU hMainMenu;

        hSysMenu = GetSystemMenu(hWnd, FALSE);
        ChangeMenu(hSysMenu, 0, NULL, 0, MF_APPEND | MF_SEPARATOR);
        ChangeMenu(hSysMenu, 0, "About", IDM_SYSABOUT, MF_APPEND | MF_STRING);

        /* 初始化对弈模式菜单对勾：默认 AI 对弈 */
        hMainMenu = GetMenu(hWnd);
        CheckMenuItem(hMainMenu, IDM_AI_MODE, MF_CHECKED);
        CheckMenuItem(hMainMenu, IDM_TWO_PLAYER, MF_UNCHECKED);

        return 0L;
    }

    /* ----------------------------------------------------------------
     * WM_SIZE - 窗口大小变化时
     * 
     * 重新计算棋盘布局，使棋盘居中且随窗口缩放。
     * wParam = 尺寸变化类型
     * lParam = 新的客户区尺寸（LOWORD=宽, HIWORD=高）
     * ---------------------------------------------------------------- */
    case WM_SIZE:
        {
                CalcBoardLayout(LOWORD(lParam), HIWORD(lParam));

                /* 窗口大小变了，销毁旧位图，WM_PAINT 会按新棋盘尺寸重建 */
                if (hBackDC != NULL && hBackBitmap != NULL) {
                        SelectObject(hBackDC, hBackOldBitmap);
                        DeleteObject(hBackBitmap);
                        hBackBitmap = NULL;
                        hBackOldBitmap = NULL;
                }

                /* 脏矩形模式下窗口大小变了，重建资源 */
                if (g_dirtyEnabled) {
                    FreeCellBuffers();
                    InitCellBuffers(hWnd);
                }

                InvalidateRect(hWnd, NULL, FALSE);
                return 0L;
        }

    /* ----------------------------------------------------------------
     * WM_ERASEBKGND - 擦除背景
     *
     * 返回 1（TRUE）但不做任何绘制——告诉 Windows 背景已处理。
     * 实际背景由 WM_PAINT 中的双缓冲完整绘制。
     *
     * 关键：如果在这里 FillRect，Windows 会先用背景色刷一遍屏幕，
     * 然后 WM_PAINT 再 BitBlt——两次绘制之间屏幕短暂显示背景色，
     * 这就是闪烁的根源。返回 TRUE 跳过系统擦除，只靠 BitBlt 一次性
     * 更新屏幕，实现无闪烁绘制。
     * ---------------------------------------------------------------- */
    case WM_ERASEBKGND:
        return 1L;

    /* ----------------------------------------------------------------
     * WM_PAINT - 绘制客户区
     *
     * 棋盘区域双缓冲技术（解决最大化闪烁）：
     *
     * 旧方案的问题：
     *   CreateCompatibleBitmap(hdc, rc.right, rc.bottom) 创建整个客户区
     *   大小的位图。最大化时 640×350 4bpp ≈ 56KB，286 GDI 堆仅 64KB，
     *   创建失败 → 回退直接画屏 → 闪烁。
     *
     * 新方案（只缓冲棋盘区域）：
     *   1. 背景直接画屏幕——纯色 FillRect，单次 GDI 调用，不闪烁
     *   2. 创建棋盘大小的位图（baseRect + shadowOffset ≈ 300×300 ≈ 22KB）
     *   3. SetViewportOrg 偏移坐标，让 DrawBoardBase/DrawGrid/DrawPieces
     *      的绝对坐标在内存 DC 中从 (0,0) 开始正确映射
     *   4. 在内存 DC 上画棋盘元素
     *   5. BitBlt 只拷贝棋盘区域到屏幕
     *
     * 为什么不动 DrawEntireBoard / CalcBoardLayout / board.c？
     *   这些函数都使用 boardLayout 的绝对坐标。SetViewportOrg 让内存
     *   DC 的坐标原点偏移到 baseRect.left/baseRect.top，所以绝对坐标
     *   在内存 DC 中自动映射到正确的相对位置。不需要改任何绘制函数。
     * ---------------------------------------------------------------- */
    case WM_PAINT:
    {
        DWORD oldOrg;
        int boardW, boardH;
        RECT rcMem, rcBoard, rcMargin;

        hdc = BeginPaint(hWnd, &ps);
        GetClientRect(hWnd, &rc);
        CalcBoardLayout(rc.right, rc.bottom);

        /* === 脏矩形模式：交给 dirtyrect.c 处理 === */
        if (g_dirtyEnabled) {
            if (DirtyRectPaint(hWnd, hdc, &ps)) {
                EndPaint(hWnd, &ps);
                return 0L;
            }
        }

        /* === 原始双缓冲路径（脏矩形禁用时） === */

        /* 棋盘区域（含阴影） */
        rcBoard.left   = boardLayout.baseRect.left;
        rcBoard.top    = boardLayout.baseRect.top;
        rcBoard.right  = boardLayout.baseRect.right + boardLayout.shadowOffset;
        rcBoard.bottom = boardLayout.baseRect.bottom + boardLayout.shadowOffset;

        /* 只画边矩背景，棋盘区域挖洞——屏幕刷新时棋盘区域仍是旧帧完整画面 */
        if (rcBoard.top > 0) {
            rcMargin.left = 0; rcMargin.top = 0;
            rcMargin.right = rc.right; rcMargin.bottom = rcBoard.top;
            FillRect(hdc, &rcMargin, hbrBackground);
        }
        if (rcBoard.bottom < rc.bottom) {
            rcMargin.left = 0; rcMargin.top = rcBoard.bottom;
            rcMargin.right = rc.right; rcMargin.bottom = rc.bottom;
            FillRect(hdc, &rcMargin, hbrBackground);
        }
        if (rcBoard.left > 0) {
            rcMargin.left = 0; rcMargin.top = rcBoard.top;
            rcMargin.right = rcBoard.left; rcMargin.bottom = rcBoard.bottom;
            FillRect(hdc, &rcMargin, hbrBackground);
        }
        if (rcBoard.right < rc.right) {
            rcMargin.left = rcBoard.right; rcMargin.top = rcBoard.top;
            rcMargin.right = rc.right; rcMargin.bottom = rcBoard.bottom;
            FillRect(hdc, &rcMargin, hbrBackground);
        }

        /* 计算棋盘位图大小 */
        boardW = (boardLayout.baseRect.right - boardLayout.baseRect.left)
               + boardLayout.shadowOffset;
        boardH = (boardLayout.baseRect.bottom - boardLayout.baseRect.top)
               + boardLayout.shadowOffset;

        /* 复用持久化内存 DC（只创建一次） */
        if (hBackDC == NULL) {
            hBackDC = CreateCompatibleDC(hdc);
        }

        /* 创建位图（首次或 WM_SIZE 删掉后重建） */
        if (hBackDC != NULL && hBackBitmap == NULL) {
            hBackBitmap = CreateCompatibleBitmap(hdc, boardW, boardH);
            if (hBackBitmap != NULL) {
                hBackOldBitmap = SelectObject(hBackDC, hBackBitmap);
            }
        }

        /* 双缓冲绘制 */
        if (hBackDC != NULL && hBackBitmap != NULL) {
            /* 先清空整个位图，消除上一帧残影和创建时的垃圾像素 */
            rcMem.left = 0;
            rcMem.top = 0;
            rcMem.right = boardW;
            rcMem.bottom = boardH;
            DrawBackground(hBackDC, &rcMem);

            /* 偏移视口：让绝对坐标映射到内存 DC 的 (0,0) */
            oldOrg = SetViewportOrg(hBackDC,
                                    -boardLayout.baseRect.left,
                                    -boardLayout.baseRect.top);

            /* 画棋盘元素 */
            DrawBoardBase(hBackDC);
            DrawGrid(hBackDC);
            DrawPieces(hBackDC);

            /* 恢复视口 */
            SetViewportOrg(hBackDC, LOWORD(oldOrg), HIWORD(oldOrg));

            /* 整块棋盘区域（含背景）原子贴到屏幕 */
            BitBlt(hdc,
                   boardLayout.baseRect.left,
                   boardLayout.baseRect.top,
                   boardW, boardH,
                   hBackDC, 0, 0, SRCCOPY);
        } else {
            /* 回退：补全背景再直接画 */
            FillRect(hdc, &rc, hbrBackground);
            DrawBoardBase(hdc);
            DrawGrid(hdc);
            DrawPieces(hdc);
        }

        EndPaint(hWnd, &ps);
        return 0L;
    }

    /* ----------------------------------------------------------------
     * WM_KEYDOWN - 键盘快捷键
     *
     * 功能键映射：
     *   N - 开始新游戏（普通模式）
     *   Z - 开始挑战模式
     *   S - 打开存档对话框
     *   L - 打开载入对话框
     *
     * 不在界面上显示，仅在帮助文档中提及。
     * ---------------------------------------------------------------- */
    case WM_KEYDOWN:
    {
        switch (wParam) {
        case 0x4E:  /* 'N' - 新游戏 */
            StartNewGame(MODE_NORMAL);
            InvalidateRect(hWnd, NULL, FALSE);
            break;
        case 0x5A:  /* 'Z' - 挑战模式 */
            StartNewGame(MODE_CHALLENGE);
            InvalidateRect(hWnd, NULL, FALSE);
            break;
        case 0x53:  /* 'S' - 存档 */
            SendMessage(hWnd, WM_COMMAND, IDM_SAVE, 0L);
            break;
        case 0x4C:  /* 'L' - 载入 */
            SendMessage(hWnd, WM_COMMAND, IDM_LOAD, 0L);
            break;
                case 0x51:  /* 'Q' - 悔棋 */
        SendMessage(hWnd, WM_COMMAND, IDM_UNDO, 0L);
        break;
        }
        return 0L;
    }

    /* ----------------------------------------------------------------
     * WM_LBUTTONDOWN - 鼠标左键按下
     * 
     * 处理棋子选择和走棋。
     * wParam = 按键状态标志
     * lParam = 鼠标坐标（LOWORD=X, HIWORD=Y）
     * ---------------------------------------------------------------- */
    case WM_LBUTTONDOWN:
    {
        int mouseX, mouseY;
        int row, col;

        /* 游戏结束时忽略点击 */
        if (gameState.gameOver)
            break;

        /* AI 模式下，只有玩家回合（白棋）可以操作
         * 双人模式下，双方都可以操作 */
        if (!gameState.twoPlayer && gameState.currentPlayer != WHITE)
            break;

        mouseX = LOWORD(lParam);
        mouseY = HIWORD(lParam);

        /* 检测点击位置对应的棋盘格 */
        if (!BoardHitTest(mouseX, mouseY, &row, &col))
            break;

        if (gameState.selRow < 0) {
            /* ---- 未选中棋子状态：尝试选中 ---- */
            if (gameState.board[row][col] == gameState.currentPlayer) {
                /* 选中当前走棋方的棋子 */
                int oldR = gameState.selRow;
                int oldC = gameState.selCol;
                gameState.selRow = row;
                gameState.selCol = col;
                if (g_dirtyEnabled)
                    DirtyRectInvalidateSelect(hWnd, oldR, oldC, row, col);
                else
                    InvalidateRect(hWnd, NULL, FALSE);
            } else {
                /* 点击了空位或对方棋子，蜂鸣提示 */
                MessageBeep(0);
            }
        } else {
            /* ---- 已选中棋子状态：尝试走棋 ---- */
            if (row == gameState.selRow && col == gameState.selCol) {
                /* 点击同一棋子：取消选中 */
                int oldR = gameState.selRow;
                int oldC = gameState.selCol;
                gameState.selRow = -1;
                gameState.selCol = -1;
                if (g_dirtyEnabled)
                    DirtyRectInvalidateSelect(hWnd, oldR, oldC, -1, -1);
                else
                    InvalidateRect(hWnd, NULL, FALSE);
            } else if (gameState.board[row][col] == gameState.currentPlayer) {
                /* 点击另一颗己方棋子：切换选中 */
                int oldR = gameState.selRow;
                int oldC = gameState.selCol;
                gameState.selRow = row;
                gameState.selCol = col;
                if (g_dirtyEnabled)
                    DirtyRectInvalidateSelect(hWnd, oldR, oldC, row, col);
                else
                    InvalidateRect(hWnd, NULL, FALSE);
            } else if (gameState.board[row][col] == EMPTY) {
                /* 点击空位：尝试走棋 */
                if (IsValidMove(gameState.selRow, gameState.selCol, row, col)) {
                    /* 执行走棋 */
                    int oldB[BOARD_SIZE][BOARD_SIZE];
                    int fr = gameState.selRow;
                    int fc = gameState.selCol;
                    /* 保存旧棋盘用于脏矩形对比 */
                    if (g_dirtyEnabled)
                        memcpy(oldB, gameState.board, sizeof(oldB));
                    MakeMove(gameState.selRow, gameState.selCol, row, col);
                    gameState.selRow = -1;
                    gameState.selCol = -1;
                    if (g_dirtyEnabled)
                        DirtyRectInvalidateBoardDiff(hWnd, oldB, gameState.board);
                    else
                        InvalidateRect(hWnd, NULL, FALSE);

                    /* 检查游戏是否结束 */
                    if (gameState.gameOver) {
                        EndGame(gameState.gameOver);
                    } else if (gameState.currentPlayer == BLACK && !gameState.twoPlayer) {
                        /* 轮到 AI，设置延时定时器（仅 AI 模式） */
                        SetTimer(hWnd, IDT_AI_MOVE, 400, NULL);
                    }
                } else {
                    /* 不是有效走法，蜂鸣提示 */
                    MessageBeep(0);
                }
            } else {
                /* 点击对方棋子：蜂鸣提示 */
                MessageBeep(0);
            }
        }
        return 0L;
    }

    /* ----------------------------------------------------------------
     * WM_TIMER - 定时器触发
     * 
     * 用于 AI 延时走棋。走完后销毁定时器。
     * ---------------------------------------------------------------- */
    case WM_TIMER:
    {
        if (wParam == IDT_AI_MOVE) {
            KillTimer(hWnd, IDT_AI_MOVE);
            /* 对话框打开时跳过 AI 走棋，重新设置定时器等下次 */
            if (hModelessDlg != NULL) {
                SetTimer(hWnd, IDT_AI_MOVE, 400, NULL);
                break;
            }
            /* 仅 AI 模式下且轮到黑棋时执行 AI 走棋 */
            if (!gameState.gameOver && gameState.currentPlayer == BLACK && !gameState.twoPlayer) {
                if (g_dirtyEnabled) {
                    int oldB[BOARD_SIZE][BOARD_SIZE];
                    memcpy(oldB, gameState.board, sizeof(oldB));
                    DoAIMove();
                    DirtyRectInvalidateBoardDiff(hWnd, oldB, gameState.board);
                } else {
                    DoAIMove();
                    InvalidateRect(hWnd, NULL, FALSE);
                }
                if (gameState.gameOver) {
                    EndGame(gameState.gameOver);
                }
            }
        }
        return 0L;
    }

    /* ----------------------------------------------------------------
     * WM_COMMAND - 菜单命令
     * 
     * wParam = 菜单项 ID
     * ---------------------------------------------------------------- */
    case WM_COMMAND:
    {
        switch (wParam) {

        case IDM_NEWGAME:
            /* 开始新游戏（普通模式） */
            StartNewGame(MODE_NORMAL);
            InvalidateRect(hWnd, NULL, FALSE);
            break;

        case IDM_CHALLENGE:
            /* 开始挑战模式（4x4 格子） */
            StartNewGame(MODE_CHALLENGE);
            InvalidateRect(hWnd, NULL, FALSE);
            break;

        case IDM_AI_MODE:
            /* 切换到 AI 对弈模式 */
            gameState.twoPlayer = 0;
            CheckMenuItem(GetMenu(hWnd), IDM_AI_MODE, MF_CHECKED);
            CheckMenuItem(GetMenu(hWnd), IDM_TWO_PLAYER, MF_UNCHECKED);
            /* 清除选中状态（可能选中了黑棋） */
            gameState.selRow = -1;
            gameState.selCol = -1;
            InvalidateRect(hWnd, NULL, FALSE);
            /* 如果当前轮到黑棋，启动 AI */
            if (gameState.currentPlayer == BLACK && !gameState.gameOver) {
                SetTimer(hWnd, IDT_AI_MOVE, 400, NULL);
            }
            break;

        case IDM_TWO_PLAYER:
            /* 切换到双人对弈模式 */
            gameState.twoPlayer = 1;
            CheckMenuItem(GetMenu(hWnd), IDM_AI_MODE, MF_UNCHECKED);
            CheckMenuItem(GetMenu(hWnd), IDM_TWO_PLAYER, MF_CHECKED);
            /* 取消可能正在等待的 AI 定时器 */
            KillTimer(hWnd, IDT_AI_MOVE);
            /* 清除选中状态 */
            gameState.selRow = -1;
            gameState.selCol = -1;
            InvalidateRect(hWnd, NULL, FALSE);
            break;

        case IDM_UNDO:
            /* 悔棋 */
            /* 先杀掉 AI 定时器，防止悔棋后 AI 偷走一步 */
            KillTimer(hWnd, IDT_AI_MOVE);
            if (UndoMove()) {
                InvalidateRect(hWnd, NULL, FALSE);
                /* 如果恢复到了 AI 的回合，让 AI 重新走 */
                                 if (!gameState.twoPlayer && gameState.currentPlayer != WHITE) {
                                        SetTimer(hWnd, IDT_AI_MOVE, 400, NULL);
                                }
            } else {
                MessageBeep(0);
            }
            break;

        case IDM_SAVE:
            /* 打开存档对话框（无模式） */
            if (hModelessDlg != NULL)
                BringWindowToTop(hModelessDlg);
            else {
                lpProcSave = MakeProcInstance((FARPROC)SaveDlgProc, hInst);
                if (lpProcSave == NULL) break;
                hModelessDlg = CreateDialog(hInst, "SaveDlg", hWnd, lpProcSave);
                /* CreateDialog 不会自动显示窗口，必须手动 ShowWindow */
                if (hModelessDlg != NULL) {
                    ShowWindow(hModelessDlg, 1);
                    BringWindowToTop(hModelessDlg);
                }
            }
            break;

        case IDM_LOAD:
            /* 打开载入对话框（无模式） */
            if (hModelessDlg != NULL)
                BringWindowToTop(hModelessDlg);
            else {
                lpProcLoad = MakeProcInstance((FARPROC)LoadDlgProc, hInst);
                if (lpProcLoad == NULL) break;
                hModelessDlg = CreateDialog(hInst, "LoadDlg", hWnd, lpProcLoad);
                if (hModelessDlg != NULL) {
                    ShowWindow(hModelessDlg, 1);
                    BringWindowToTop(hModelessDlg);
                }
            }
            break;

        case IDM_SETTINGS:
            /* 打开设置对话框（无模式） */
            if (hModelessDlg != NULL)
                BringWindowToTop(hModelessDlg);
            else {
                lpProcSettings = MakeProcInstance((FARPROC)SettingsDlgProc, hInst);
                if (lpProcSettings == NULL) break;
                hModelessDlg = CreateDialog(hInst, "SettingsDlg", hWnd, lpProcSettings);
                if (hModelessDlg != NULL) {
                    ShowWindow(hModelessDlg, 1);
                    BringWindowToTop(hModelessDlg);
                }
            }
            break;

        case IDM_DIRTYRECT:
            /* 打开脏矩形设置对话框（无模式） */
            if (hModelessDlg != NULL)
                BringWindowToTop(hModelessDlg);
            else {
                lpProcDirtyRect = MakeProcInstance((FARPROC)DirtyRectDlgProc, hInst);
                if (lpProcDirtyRect == NULL) break;
                hModelessDlg = CreateDialog(hInst, "DirtyRectDlg", hWnd, lpProcDirtyRect);
                if (hModelessDlg != NULL) {
                    ShowWindow(hModelessDlg, 1);
                    BringWindowToTop(hModelessDlg);
                }
            }
            break;

        case IDM_HELP:
            /* 打开帮助对话框（模态） */
            lpProcHelp = MakeProcInstance((FARPROC)HelpDlgProc, hInst);
            if (lpProcHelp != NULL) {
                DialogBox(hInst, "HelpDlg", hWnd, lpProcHelp);
                FreeProcInstance(lpProcHelp);
                lpProcHelp = NULL;
            }
            break;

        case IDM_ABOUT:
            /* 打开关于对话框（模态） */
            lpProcAbout = MakeProcInstance((FARPROC)AboutDlgProc, hInst);
            if (lpProcAbout != NULL) {
                DialogBox(hInst, "AboutDlg", hWnd, lpProcAbout);
                FreeProcInstance(lpProcAbout);
                lpProcAbout = NULL;
            }
            break;
        }
        return 0L;
    }

    /* ----------------------------------------------------------------
     * WM_SYSCOMMAND - 系统菜单命令
     * 
     * 检查是否点击了系统菜单中的"关于"项。
     * ---------------------------------------------------------------- */
    case WM_SYSCOMMAND:
    {
        if (wParam == IDM_SYSABOUT) {
            lpProcAbout = MakeProcInstance((FARPROC)AboutDlgProc, hInst);
            if (lpProcAbout != NULL) {
                DialogBox(hInst, "AboutDlg", hWnd, lpProcAbout);
                FreeProcInstance(lpProcAbout);
                lpProcAbout = NULL;
            }
            return 0L;
        }
        break;
    }

    /* ----------------------------------------------------------------
     * WM_DESTROY - 窗口销毁
     * 
     * 清理资源，退出消息循环。
     * PostQuitMessage 发送 WM_QUIT 消息，使 GetMessage 返回 FALSE。
     * ---------------------------------------------------------------- */
    case WM_DESTROY:
    {
        /* 清理脏矩形资源 */
        DirtyRectCleanup();

        /* 清理持久化双缓冲 */
        if (hBackDC != NULL) {
            if (hBackBitmap != NULL) {
                SelectObject(hBackDC, hBackOldBitmap);
                DeleteObject(hBackBitmap);
                hBackBitmap = NULL;
            }
            DeleteDC(hBackDC);
            hBackDC = NULL;
        }
        if (hbrBackground != NULL)
            DeleteObject(hbrBackground);
        PostQuitMessage(0);
        return 0L;
    }

    default:
        break;
    }

    /* 未处理的消息交给默认窗口过程 */
    return DefWindowProc(hWnd, message, wParam, lParam);
}

/* --------------------------------------------------------------------
 * StartNewGame - 开始新游戏
 * 
 * 重置棋盘到初始状态，根据先手设置决定谁先走棋。
 * 
 * 普通模式：4x4 交叉点，各 4 子在底线
 * 挑战模式：4x4 格子，各 4 子在底线行
 * 
 * 先后手规则（默认模式）：
 *   - 胜则交换先后手
 *   - 败则玩家先手
 *   - 可在设置中改为始终先手/后手
 * -------------------------------------------------------------------- */
void StartNewGame(mode)
int mode;
{
    int row, col;

    /* 重置游戏状态 */
    gameState.mode = mode;
    gameState.selRow = -1;
    gameState.selCol = -1;
    gameState.gameOver = 0;
    gameState.whiteCount = 4;
    gameState.blackCount = 4;
    gameState.lastMoveRow = -1;
    gameState.lastMoveCol = -1;

    /* 清空棋盘 */
    for (row = 0; row < BOARD_SIZE; row++) {
        for (col = 0; col < BOARD_SIZE; col++) {
            gameState.board[row][col] = EMPTY;
        }
    }

    /* 摆放初始棋子 */
    /* 第 0 行（顶部）放黑棋（AI） */
    for (col = 0; col < BOARD_SIZE; col++)
        gameState.board[0][col] = BLACK;

    /* 第 3 行（底部）放白棋（玩家） */
    for (col = 0; col < BOARD_SIZE; col++)
        gameState.board[3][col] = WHITE;

    /* 确定先手方 */
    if (gameState.firstSetting == FIRST_ALWAYS) {
        gameState.firstPlayer = WHITE;   /* 玩家先手 */
    } else if (gameState.firstSetting == SECOND_ALWAYS) {
        gameState.firstPlayer = BLACK;   /* AI 先手 */
    } else {
        /* FIRST_DEFAULT 模式：保持上次更新值
         * 但首次游戏时 firstPlayer 为 0（全局零初始化），需要默认为玩家先手 */
        if (gameState.firstPlayer != WHITE && gameState.firstPlayer != BLACK)
            gameState.firstPlayer = WHITE;
    }

    gameState.currentPlayer = gameState.firstPlayer;

    /* 清空悔棋历史并保存初始状态 */
    historyCount = 0;
    historyCurrent = 0;
    SaveHistoryEntry();

    /* 如果 AI 先手，设置定时器延时走棋 */
    if (gameState.currentPlayer == BLACK && !gameState.gameOver && !gameState.twoPlayer) {
        SetTimer(hWndMain, IDT_AI_MOVE, 400, NULL);
    }
}

/* --------------------------------------------------------------------
 * EndGame - 游戏结束处理
 * 
 * 显示胜负信息，并根据默认先后手规则更新下一局先手。
 * 
 * 默认规则：
 *   - 玩家胜 → 交换先后手（先→后，后→先）
 *   - 玩家败 → 玩家始终先手
 * -------------------------------------------------------------------- */
void EndGame(winner)
int winner;
{
    char msg[80];

    if (gameState.twoPlayer) {
        /* 双人模式：直接报告胜方 */
        if (winner == WHITE)
            strcpy(msg, "White wins! Press N for a new game.");
        else
            strcpy(msg, "Black wins! Press N for a new game.");
    } else {
        /* AI 模式：报告玩家胜负 */
        if (winner == WHITE) {
            strcpy(msg, "You win! Press N for a new game.");
        } else {
            strcpy(msg, "You lose! Press N for a new game.");
        }
    }

    /* 仅在默认模式下更新先后手 */
    if (gameState.firstSetting == FIRST_DEFAULT) {
        if (winner == WHITE) {
            /* 玩家胜：交换先后手 */
            if (gameState.firstPlayer == WHITE)
                gameState.firstPlayer = BLACK;
            else
                gameState.firstPlayer = WHITE;
        } else {
            /* 玩家败：玩家先手 */
            gameState.firstPlayer = WHITE;
        }
    }

    MessageBox(hWndMain, msg, "Game Over", MB_OK | MB_ICONASTERISK);
}
