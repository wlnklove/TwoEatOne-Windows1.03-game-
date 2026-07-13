/* ====================================================================
 * twoeatone.h - 公共头文件
 * 《走四子（二吃一）》民间棋游戏
 * 适用于 Microsoft C 4.0 + Windows 1.03 SDK
 * 使用 K&R C 语法
 * ==================================================================== */

#include <windows.h>

/* --------------------------------------------------------------------
 * 菜单命令 ID 定义
 * 这些 ID 在 .rc 资源文件和 .c 源文件中共享
 * -------------------------------------------------------------------- */
#define IDM_NEWGAME       101   /* 菜单：开始新游戏 */
#define IDM_CHALLENGE     102   /* 菜单：挑战模式 */
#define IDM_UNDO          103   /* 菜单：悔棋 */
#define IDM_SAVE          104   /* 菜单：保存 */
#define IDM_LOAD          105   /* 菜单：载入 */
#define IDM_SETTINGS      106   /* 菜单：游戏设置 */
#define IDM_HELP          107   /* 菜单：游戏帮助 */
#define IDM_ABOUT         108   /* 菜单：关于 */
#define IDM_AI_MODE       109   /* 菜单：AI对弈模式 */
#define IDM_TWO_PLAYER    110   /* 菜单：双人对弈模式 */
#define IDM_DIRTYRECT     111   /* 菜单：脏矩形设置 */

/* 系统菜单中"关于"项的 ID（必须 < 0xF000 避免与系统命令冲突） */
#define IDM_SYSABOUT      0x0010

/* --------------------------------------------------------------------
 * 对话框控件 ID
 * -------------------------------------------------------------------- */
/* 设置对话框 */
#define IDC_GRP_FIRSTHAND  201  /* 分组框：先后手 */
#define IDC_SET_DEFAULT    202  /* 单选：默认 */
#define IDC_SET_FIRST      203  /* 单选：始终先手 */
#define IDC_SET_SECOND     204  /* 单选：始终后手 */
#define IDC_GRP_COLORMODE  205  /* 分组框：颜色模式 */
#define IDC_SET_EGA        206  /* 单选：暗绿 */
#define IDC_SET_VGA        207  /* 单选：深绿 */
#define IDC_SET_BRIGHT     208  /* 单选：亮绿 */

/* 脏矩形设置对话框 */
#define IDC_DR_ENABLE      209  /* 复选框：开启脏矩形 */
#define IDC_DR_DC          210  /* 单选：DC Direct 模式 */
#define IDC_DR_PARTIAL     211  /* 单选：Partial Update 模式 */
#define IDC_DR_CELL        212  /* 单选：Cell Buffer 模式 */

/* 存档对话框 */
#define IDC_SAVE_EDIT      301  /* 编辑框：存档名称 */
#define IDC_SAVE_LIST      302  /* 列表框：存档列表 */
#define IDC_SAVE_DELETE    303  /* 按钮：删除存档 */

/* 载入对话框 */
#define IDC_LOAD_LIST      401  /* 列表框：存档列表 */
#define IDC_LOAD_DELETE    402  /* 按钮：删除存档 */

/* --------------------------------------------------------------------
 * 定时器 ID
 * -------------------------------------------------------------------- */
#define IDT_AI_MOVE        1    /* AI 走棋延时定时器 */

/* --------------------------------------------------------------------
 * 游戏常量
 * -------------------------------------------------------------------- */
#define EMPTY   0               /* 空位 */
#define WHITE   1               /* 白棋（玩家，下方，灰白色） */
#define BLACK   2               /* 黑棋（AI，上方，黑色） */

#define MODE_NORMAL      0      /* 普通模式：3x3 网格，棋子在交叉点 */
#define MODE_CHALLENGE   1      /* 挑战模式：4x4 网格，棋子在格子里 */

#define FIRST_DEFAULT    0      /* 默认先后手：胜则交换，败则先手 */
#define FIRST_ALWAYS     1      /* 始终先手 */
#define SECOND_ALWAYS    2      /* 始终后手 */

#define BOARD_SIZE        4     /* 棋盘 4x4 */
#define MAX_HISTORY       50    /* 最多悔棋步数 */
#define MAX_SAVES         20    /* 最多存档数 */
#define SAVE_NAME_LEN     32    /* 存档名最大长度 */

/* 脏矩形模式（值从 0 开始，与对话框 IDC_DR_DC 偏移对应） */
#define DR_DC              0    /* DC Direct 模式 */
#define DR_PARTIAL         1    /* Partial Update 模式 */
#define DR_CELL            2    /* Cell Buffer 模式 */

/* --------------------------------------------------------------------
 * 棋盘布局结构体
 * 记录棋盘在窗口中的绘制坐标，用于绘制和点击检测
 * -------------------------------------------------------------------- */
typedef struct {
    int centerX;          /* 棋盘中心 X */
    int centerY;          /* 棋盘中心 Y */
    int cellSize;         /* 每格大小（像素） */
    int gridX;            /* 第一条线的 X 坐标（网格左上角） */
    int gridY;            /* 第一条线的 Y 坐标 */
    int gridSize;         /* 网格总尺寸（从第一条线到最后一条线） */
    int margin;           /* 底座边缘到网格线的间距 */
    int borderWidth;      /* 底座黑色边框宽度 */
    int shadowOffset;     /* 阴影偏移量 */
    RECT baseRect;        /* 底座矩形（含黑色边框） */
    RECT shadowRect;      /* 阴影矩形 */
    RECT innerRect;       /* 底座内部灰色面（边框内侧） */
} BoardLayout;

/* --------------------------------------------------------------------
 * 游戏状态结构体
 * 保存当前游戏的全部状态
 * -------------------------------------------------------------------- */
typedef struct {
    int board[BOARD_SIZE][BOARD_SIZE]; /* 棋盘数组 [行][列]，0=空 1=白 2=黑 */
    int mode;             /* 游戏模式：MODE_NORMAL 或 MODE_CHALLENGE */
    int currentPlayer;    /* 当前走棋方：WHITE 或 BLACK */
    int firstPlayer;      /* 本局先手方：WHITE 或 BLACK */
    int firstSetting;     /* 先后手设置：FIRST_DEFAULT / FIRST_ALWAYS / SECOND_ALWAYS */
    int twoPlayer;        /* 对弈模式：0=AI对弈 1=双人对弈 */
    int selRow;           /* 已选中的棋子行号（-1 表示未选中） */
    int selCol;           /* 已选中的棋子列号 */
    int whiteCount;       /* 白棋剩余数量 */
    int blackCount;       /* 黑棋剩余数量 */
    int gameOver;         /* 游戏是否结束：0=进行中 1=白胜 2=黑胜 */
    int egaMode;          /* 颜色模式：0=暗绿 1=深绿 2=亮绿 */
    int lastMoveRow;      /* 上一步走棋的目标行（用于高亮） */
    int lastMoveCol;      /* 上一步走棋的目标列 */
} GameState;

/* --------------------------------------------------------------------
 * 悔棋历史记录条目
 * -------------------------------------------------------------------- */
typedef struct {
    int board[BOARD_SIZE][BOARD_SIZE];
    int currentPlayer;
    int whiteCount;
    int blackCount;
    int lastMoveRow;
    int lastMoveCol;
} HistoryEntry;

/* --------------------------------------------------------------------
 * 存档数据结构
 * -------------------------------------------------------------------- */
typedef struct {
    char magic[4];        /* 魔数 "TEO1" */
    int version;          /* 版本号 */
    int mode;             /* 游戏模式 */
    int board[BOARD_SIZE][BOARD_SIZE];
    int currentPlayer;
    int whiteCount;
    int blackCount;
    int firstPlayer;
    int firstSetting;
    int twoPlayer;
    int egaMode;
    int gameOver;         /* 游戏是否结束 */
    char saveName[SAVE_NAME_LEN];
    long timestamp;       /* 存档时间戳 */
} SaveData;

/* --------------------------------------------------------------------
 * 全局变量声明（在 twoeatone.c 中定义）
 * -------------------------------------------------------------------- */
extern HANDLE hInst;              /* 应用程序实例句柄 */
extern HWND hWndMain;             /* 主窗口句柄 */
extern HWND hModelessDlg;         /* 当前打开的无模式对话框句柄 */
extern GameState gameState;       /* 游戏状态 */
extern BoardLayout boardLayout;   /* 棋盘布局 */
extern HistoryEntry history[];    /* 悔棋历史 */
extern int historyCount;          /* 历史记录总数 */
extern int historyCurrent;        /* 当前历史位置 */
extern HBRUSH hbrBackground;      /* 背景画刷 */
extern HCURSOR hCursorArrow;      /* 箭头光标 */

/* 持久化双缓冲 DC（dirtyrect.c 也需要访问） */
extern HDC hBackDC;
extern HBITMAP hBackBitmap;
extern HBITMAP hBackOldBitmap;

/* 脏矩形全局变量 */
extern int g_dirtyEnabled;
extern int g_dirtyMode;

/* --------------------------------------------------------------------
 * 函数原型声明（K&R 风格，无参数类型）
 * -------------------------------------------------------------------- */

/* twoeatone.c */
int PASCAL WinMain();
BOOL InitApplication();
BOOL InitInstance();
LONG FAR PASCAL MainWndProc();
void StartNewGame();
void EndGame();

/* board.c */
void CalcBoardLayout();
void DrawEntireBoard();
void DrawBackground();
void DrawBoardBase();
void DrawGrid();
void DrawPieces();
void DrawPiece();
BOOL BoardHitTest();
void GetPieceCenter();
void GetPieceRect();          /* 新增：获取棋子屏幕矩形 */

/* game.c */
void InitBoard();
BOOL IsValidMove();
void MakeMove();
int CheckCapture();
BOOL CanPlayerMove();
void GetValidMoves();
void SaveHistoryEntry();
BOOL UndoMove();
BOOL IsAdjacent();

/* ai.c */
void DoAIMove();
int MinimaxNormal();
int MinimaxChallenge();
int EvaluateNormal();
int EvaluateChallenge();
void CopyBoard();
int CountMoves();

/* dialogs.c */
BOOL FAR PASCAL AboutDlgProc();
BOOL FAR PASCAL HelpDlgProc();
BOOL FAR PASCAL SettingsDlgProc();
BOOL FAR PASCAL SaveDlgProc();
BOOL FAR PASCAL LoadDlgProc();
BOOL FAR PASCAL DirtyRectDlgProc();

/* dirtyrect.c */
void DirtyRectEnable();
void DirtyRectSetMode();
void DirtyRectInvalidatePiece();
void DirtyRectInvalidateSelect();
void DirtyRectInvalidateMove();
void DirtyRectInvalidateBoardDiff();
int  DirtyRectPaint();
void DirtyRectEnterMode();
void DirtyRectExitMode();
void DirtyRectCleanup();
void InitCellBuffers();
void FreeCellBuffers();
void RenderCellBitmap();
void RenderCleanBoard();

/* twoeat.c 中定义的 MakeProcInstance thunk 指针
 * Win 1.03 下直接传函数指针给 CreateDialog/DialogBox 会导致 DS 数据段
 * 指向错误的位置（Windows USER.EXE 的 DS），回调函数中读写全局变量
 * 会命中随机内存。MakeProcInstance 创建一个 thunk 在调用前设置正确的 DS。
 *
 * 重要：无模式对话框的 thunk 不在 WM_DESTROY 中 FreeProcInstance！
 * 因为 WM_DESTROY 是在 DestroyWindow() 内同步发送的，此时对话框过程
 * 仍在调用栈上（通过 thunk 调用）。如果 thunk 用 CALL 而非 JMP，
 * 释放后 return 会跳到已释放的内存，导致内存损坏/系统卡死。
 * SDK 文档说 MakeProcInstance 对同一函数+实例返回相同地址，所以 thunk
 * 会被复用，不需要释放。模态对话框在 DialogBox 返回后释放是安全的。 */
extern FARPROC lpProcAbout;
extern FARPROC lpProcHelp;
extern FARPROC lpProcSettings;
extern FARPROC lpProcSave;
extern FARPROC lpProcLoad;
extern FARPROC lpProcDirtyRect;

/* save.c */
void EnsureSaveDir();
BOOL SaveGame();
BOOL LoadGame();
int GetSaveList();
void GetSaveFileName();
BOOL LoadLatestSave();
BOOL DeleteSave();

/* 图标资源 ID */
#define IDI_APPICON 500
