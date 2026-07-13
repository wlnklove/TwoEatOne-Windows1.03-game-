/* ====================================================================
 * board.c - 棋盘绘制模块
 * 
 * 负责棋盘布局计算、背景绘制、底座与阴影绘制、
 * 网格线绘制、棋子绘制。
 * 
 * 绘制设计要点：
 * 1. 背景色为 EGA 标准绿（RGB(0,128,0)），不使用抖动
 * 2. 棋盘底座为深灰色，略大于网格，带黑色阴影产生立体浮起效果
 * 3. 网格线为黑色刻纹
 * 4. 棋子为灰白色（白方）和黑色（黑方）圆形
 * 5. 普通模式棋子在交叉点，挑战模式棋子在格子中心
 * ==================================================================== */

#include "twoeat.h"

/* --------------------------------------------------------------------
 * CalcBoardLayout - 计算棋盘布局
 * 
 * 根据窗口客户区大小，计算棋盘的居中位置和各部分尺寸。
 * 棋盘大小取窗口最小维度的 80%，确保舒适的人眼识别。
 * 
 * 参数：
 *   cx, cy - 客户区宽度和高度
 * 
 * 棋盘结构（剖面）：
 *   ┌─────────────────────────┐
 *   │  底座（深灰色）          │  ← 底座边缘
 *   │   ┌───────────────────┐  │  
 *   │   │ 网格线区域          │  │  ← margin 间距
 *   │   │  +--+--+--+        │  │
 *   │   │  |  |  |  |        │  │  ← 网格（3x3 或 4x4）
 *   │   │  +--+--+--+        │  │
 *   │   │  |  |  |  |        │  │
 *   │   │  +--+--+--+        │  │
 *   │   └───────────────────┘  │
 *   └─────────────────────────┘
 *          阴影（黑色，偏移）
 * -------------------------------------------------------------------- */
void CalcBoardLayout(cx, cy)
int cx, cy;
{
    int minDim;
    int numLines;
    int gridExtent;

    /* 取窗口最小维度的 80% 作为棋盘参考尺寸 */
    minDim = (cx < cy) ? cx : cy;
    minDim = (minDim * 8) / 10;

    /* 确定网格线数量 */
    if (gameState.mode == MODE_NORMAL)
        numLines = 4;   /* 普通模式：4 条线（3 格） */
    else
        numLines = 5;   /* 挑战模式：5 条线（4 格） */

    /* 计算每格大小 */
    boardLayout.cellSize = minDim / numLines;

    /* 网格总尺寸（从第一条线到最后一条线） */
    gridExtent = boardLayout.cellSize * (numLines - 1);

    /* 底座边缘留白：加大到 2/3 格，让底座明显大于网格 */
    boardLayout.margin = boardLayout.cellSize * 2 / 3;

    /* 黑色边框宽度：约 3 像素（不随格子缩放，保持视觉一致） */
    boardLayout.borderWidth = 3;

    /* 阴影偏移量 */
    boardLayout.shadowOffset = boardLayout.cellSize / 6;

    /* 棋盘中心坐标 */
    boardLayout.centerX = cx / 2;
    boardLayout.centerY = cy / 2;

    /* 网格左上角坐标（第一条线的交点） */
    boardLayout.gridX = boardLayout.centerX - gridExtent / 2;
    boardLayout.gridY = boardLayout.centerY - gridExtent / 2;

    /* 网格总尺寸 */
    boardLayout.gridSize = gridExtent;

    /* 底座矩形（略大于网格，留出 margin） */
    boardLayout.baseRect.left = boardLayout.gridX - boardLayout.margin;
    boardLayout.baseRect.top = boardLayout.gridY - boardLayout.margin;
    boardLayout.baseRect.right = boardLayout.gridX + gridExtent + boardLayout.margin;
    boardLayout.baseRect.bottom = boardLayout.gridY + gridExtent + boardLayout.margin;

    /* 底座内部灰色面（边框内侧，比 baseRect 小 borderWidth） */
    boardLayout.innerRect.left = boardLayout.baseRect.left + boardLayout.borderWidth;
    boardLayout.innerRect.top = boardLayout.baseRect.top + boardLayout.borderWidth;
    boardLayout.innerRect.right = boardLayout.baseRect.right - boardLayout.borderWidth;
    boardLayout.innerRect.bottom = boardLayout.baseRect.bottom - boardLayout.borderWidth;

    /* 阴影矩形（向右下偏移） */
    boardLayout.shadowRect.left = boardLayout.baseRect.left + boardLayout.shadowOffset;
    boardLayout.shadowRect.top = boardLayout.baseRect.top + boardLayout.shadowOffset;
    boardLayout.shadowRect.right = boardLayout.baseRect.right + boardLayout.shadowOffset;
    boardLayout.shadowRect.bottom = boardLayout.baseRect.bottom + boardLayout.shadowOffset;
}

/* --------------------------------------------------------------------
 * DrawEntireBoard - 绘制完整棋盘
 * 
 * 按顺序绘制：背景 → 阴影 → 底座 → 网格线 → 棋子
 * 在内存 DC 上绘制（双缓冲），由 WM_PAINT 调用。
 * -------------------------------------------------------------------- */
void DrawEntireBoard(hdc, prc)
HDC hdc;
RECT *prc;
{
    /* 1. 绘制背景 */
    DrawBackground(hdc, prc);

    /* 2. 绘制棋盘阴影 + 底座 + 网格 + 棋子 */
    DrawBoardBase(hdc);
    DrawGrid(hdc);
    DrawPieces(hdc);
}

/* --------------------------------------------------------------------
 * DrawBackground - 绘制背景
 *
 * 用绿色画刷填充整个客户区。
 * 三种颜色模式：
 *   0 = 暗绿 RGB(0,128,0)（标准 EGA 绿）
 *   1 = 深绿 RGB(0,64,0)（更深的绿）
 *   2 = 亮绿 RGB(0,255,0)（明亮的绿）
 * 纯色填充，不使用抖动。
 * -------------------------------------------------------------------- */
void DrawBackground(hdc, prc)
HDC hdc;
RECT *prc;
{
    HBRUSH hBrush, hOldBrush;

    if (gameState.egaMode == 0)
        hBrush = CreateSolidBrush(RGB(0, 128, 0));   /* 暗绿 */
    else if (gameState.egaMode == 1)
        hBrush = CreateSolidBrush(RGB(0, 64, 0));    /* 深绿 */
    else
        hBrush = CreateSolidBrush(RGB(0, 255, 0));   /* 亮绿 */

    FillRect(hdc, prc, hBrush);

    DeleteObject(hBrush);
}

/* --------------------------------------------------------------------
 * DrawBoardBase - 绘制棋盘底座和阴影
 *
 * 三层结构营造立体感：
 *   1. 黑色阴影矩形（向右下偏移）—— 棋盘"浮起"于桌面的投影
 *   2. 黑色边框矩形（baseRect）—— 底座四边的黑色框
 *   3. 灰色内部矩形（innerRect）—— 边框内侧的灰色面板
 *
 * 阴影 + 黑色边框 + 灰色面板 = 类似系统 Reversi 的 3D 浮起效果
 * -------------------------------------------------------------------- */
void DrawBoardBase(hdc)
HDC hdc;
{
    HBRUSH hBrush;

    /* 1. 绘制阴影（纯黑色矩形，向右下偏移） */
    hBrush = CreateSolidBrush(RGB(0, 0, 0));
    FillRect(hdc, &boardLayout.shadowRect, hBrush);
    DeleteObject(hBrush);

    /* 2. 绘制底座黑色边框（填满 baseRect，作为外框） */
    hBrush = CreateSolidBrush(RGB(0, 0, 0));
    FillRect(hdc, &boardLayout.baseRect, hBrush);
    DeleteObject(hBrush);

    /* 3. 绘制底座灰色面板（innerRect 比 baseRect 小 borderWidth，
     *    露出周围的黑色边框） */
    hBrush = CreateSolidBrush(RGB(128, 128, 128));
    FillRect(hdc, &boardLayout.innerRect, hBrush);
    DeleteObject(hBrush);
}

/* --------------------------------------------------------------------
 * DrawGrid - 绘制网格线
 * 
 * 用黑色线条在底座上绘制网格。
 * 
 * 普通模式：4 条横线 × 4 条竖线（棋子在交叉点）
 * 挑战模式：5 条横线 × 5 条竖线（棋子在格子中）
 * 
 * GDI 绘线方法：
 *   MoveTo  - 设置画笔起点
 *   LineTo  - 画线到终点
 *   SelectObject - 选择画笔（黑色实线）
 * -------------------------------------------------------------------- */
void DrawGrid(hdc)
HDC hdc;
{
    HPEN hPen, hOldPen;
    int i;
    int numLines;
    int linePos;

    if (gameState.mode == MODE_NORMAL)
        numLines = 4;
    else
        numLines = 5;

    /* 创建黑色画笔 */
    hPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
    hOldPen = SelectObject(hdc, hPen);

    /* 绘制横线 */
    for (i = 0; i < numLines; i++) {
        linePos = boardLayout.gridY + i * boardLayout.cellSize;
        MoveTo(hdc, boardLayout.gridX, linePos);
        LineTo(hdc, boardLayout.gridX + boardLayout.gridSize, linePos);
    }

    /* 绘制竖线 */
    for (i = 0; i < numLines; i++) {
        linePos = boardLayout.gridX + i * boardLayout.cellSize;
        MoveTo(hdc, linePos, boardLayout.gridY);
        LineTo(hdc, linePos, boardLayout.gridY + boardLayout.gridSize);
    }

    /* 恢复原有画笔并清理 */
    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
}

/* --------------------------------------------------------------------
 * DrawPieces - 绘制所有棋子
 * 
 * 遍历棋盘数组，在对应位置绘制棋子。
 * 同时绘制选中棋子的高亮标记和上一步走棋标记。
 * -------------------------------------------------------------------- */
void DrawPieces(hdc)
HDC hdc;
{
    int row, col;

    for (row = 0; row < BOARD_SIZE; row++) {
        for (col = 0; col < BOARD_SIZE; col++) {
            if (gameState.board[row][col] != EMPTY) {
                DrawPiece(hdc, row, col, gameState.board[row][col]);
            }
        }
    }

    /* 绘制选中棋子的高亮环 */
    if (gameState.selRow >= 0 && gameState.selCol >= 0) {
        int px, py;
        int radius;
        HPEN hPen, hOldPen;

        GetPieceCenter(gameState.selRow, gameState.selCol, &px, &py);
        radius = boardLayout.cellSize / 3;

        /* 用亮黄色画环表示选中 */
        hPen = CreatePen(PS_SOLID, 2, RGB(255, 255, 0));
        hOldPen = SelectObject(hdc, hPen);
        SelectObject(hdc, GetStockObject(NULL_BRUSH));

        Ellipse(hdc, px - radius - 3, py - radius - 3,
                px + radius + 3, py + radius + 3);

        SelectObject(hdc, hOldPen);
        DeleteObject(hPen);
    }
}

/* --------------------------------------------------------------------
 * DrawPiece - 绘制单个棋子
 * 
 * 在指定行列位置绘制棋子（圆形）。
 * 
 * 普通模式：棋子中心在网格线交叉点
 * 挑战模式：棋子中心在格子中心
 * 
 * 白棋用灰白色填充 + 深灰色边框
 * 黑棋用黑色填充 + 深灰色边框
 * 
 * Ellipse - GDI 绘制椭圆/圆函数
 * -------------------------------------------------------------------- */
void DrawPiece(hdc, row, col, color)
HDC hdc;
int row, col, color;
{
    int px, py;
    int radius;
    HBRUSH hBrush, hOldBrush;
    HPEN hPen, hOldPen;

    /* 获取棋子中心坐标 */
    GetPieceCenter(row, col, &px, &py);

    /* 棋子半径约为格子的 1/3 */
    radius = boardLayout.cellSize / 3;

    /* 深灰色边框（所有棋子统一） */
    hPen = CreatePen(PS_SOLID, 1, RGB(64, 64, 64));
    hOldPen = SelectObject(hdc, hPen);

    /* 根据颜色选择填充画刷 */
    if (color == WHITE) {
        /* 灰白色棋子 */
        hBrush = CreateSolidBrush(RGB(200, 200, 200));
    } else {
        /* 黑色棋子 */
        hBrush = CreateSolidBrush(RGB(0, 0, 0));
    }
    hOldBrush = SelectObject(hdc, hBrush);

    /* 绘制圆形棋子 */
    Ellipse(hdc, px - radius, py - radius, px + radius, py + radius);

    /* 恢复并清理 GDI 对象 */
    SelectObject(hdc, hOldPen);
    SelectObject(hdc, hOldBrush);
    DeleteObject(hPen);
    DeleteObject(hBrush);
}

/* --------------------------------------------------------------------
 * GetPieceCenter - 获取棋子中心坐标
 * 
 * 根据棋盘模式和行列号，计算棋子在屏幕上的中心坐标。
 * 
 * 普通模式：棋子在交叉点
 *   中心 = (gridX + col * cellSize, gridY + row * cellSize)
 * 
 * 挑战模式：棋子在格子中心
 *   中心 = (gridX + col * cellSize + cellSize/2, gridY + row * cellSize + cellSize/2)
 * -------------------------------------------------------------------- */
void GetPieceCenter(row, col, pX, pY)
int row, col;
int *pX, *pY;
{
    if (gameState.mode == MODE_NORMAL) {
        /* 交叉点位置 */
        *pX = boardLayout.gridX + col * boardLayout.cellSize;
        *pY = boardLayout.gridY + row * boardLayout.cellSize;
    } else {
        /* 格子中心位置 */
        *pX = boardLayout.gridX + col * boardLayout.cellSize + boardLayout.cellSize / 2;
        *pY = boardLayout.gridY + row * boardLayout.cellSize + boardLayout.cellSize / 2;
    }
}

/* --------------------------------------------------------------------
 * GetPieceRect - 获取棋子所在的屏幕矩形区域
 *
 * 返回包含棋子 + 高亮环的矩形，用于脏矩形刷新。
 * 半径 = cellSize/3 + 6（棋子半径 cellSize/3 + 高亮余量 6）。
 * -------------------------------------------------------------------- */
void GetPieceRect(row, col, prc)
int row, col;
RECT *prc;
{
    int px, py;
    int r;

    GetPieceCenter(row, col, &px, &py);
    r = boardLayout.cellSize / 3 + 6;

    prc->left = px - r;
    prc->top = py - r;
    prc->right = px + r;
    prc->bottom = py + r;
}

/* --------------------------------------------------------------------
 * BoardHitTest - 鼠标点击检测
 *
 * 将鼠标坐标转换为棋盘行列号。
 * 如果点击在某个棋子/格子的有效范围内，返回 TRUE 并输出行列。
 *
 * 判定半径为半个格子大小，确保点击容差合理。
 *
 * 注意：必须用 long（32位）做距离平方运算！
 * MSC 4.0 中 int 是 16 位，dx*dx 在 dx>181 时就溢出（32767/181≈181），
 * 棋盘上 dx 轻松超过 300 像素，溢出后变负数，远处棋子反而"命中"。
 * -------------------------------------------------------------------- */
BOOL BoardHitTest(mouseX, mouseY, pRow, pCol)
int mouseX, mouseY;
int *pRow, *pCol;
{
    int row, col;
    int px, py;
    long dx, dy;
    long radius2;

    radius2 = (long)boardLayout.cellSize * boardLayout.cellSize / 4;

    for (row = 0; row < BOARD_SIZE; row++) {
        for (col = 0; col < BOARD_SIZE; col++) {
            GetPieceCenter(row, col, &px, &py);

            dx = (long)(mouseX - px);
            dy = (long)(mouseY - py);

            /* 圆形判定区域（32位运算，不会溢出） */
            if (dx * dx + dy * dy <= radius2) {
                *pRow = row;
                *pCol = col;
                return TRUE;
            }
        }
    }
    return FALSE;
}
