/* ====================================================================
 * dirtyrect.c - 脏矩形刷新管理模块
 *
 * 三种模式：
 *   Mode 0 (DR_DC):      直接画屏幕 DC，依赖 BeginPaint 裁剪
 *   Mode 1 (DR_PARTIAL): 内存 DC 全量重绘，只 BitBlt 脏区域到屏幕
 *   Mode 2 (DR_CELL):    格子级位图缓存，脏矩形只 BitBlt 变化的格子
 *
 * 设计原则：
 *   - 不干扰原始双缓冲逻辑（禁用时走原 WM_PAINT 路径）
 *   - 模式切换时清理旧资源、建立新资源
 *   - 格子缓存复用 hBackDC 作为"干净棋盘"源
 * ==================================================================== */

#include "twoeat.h"
#include <string.h>

/* DR_DC / DR_PARTIAL / DR_CELL 常量在 twoeat.h 中定义 */

/* --------------------------------------------------------------------
 * 全局变量
 * -------------------------------------------------------------------- */
int g_dirtyEnabled = 0;    /* 脏矩形开关 */
int g_dirtyMode    = 0;    /* 当前模式：DR_DC / DR_PARTIAL / DR_CELL */

/* 模式 2：格子位图缓存 */
HDC     g_hCellDC    = NULL;
HBITMAP g_hbmCells[BOARD_SIZE][BOARD_SIZE];
HBITMAP g_hCellOldBmp = NULL;
int     g_cellRadius = 0;   /* 格子位图半径 */
int     g_cellBmpSize = 0;  /* 格子位图边长 */
int     g_cellsValid = 0;   /* 格子位图是否已创建 */

/* 脏格子标记（模式 2 用） */
int g_dirtyCells[BOARD_SIZE][BOARD_SIZE];

/* --------------------------------------------------------------------
 * GetPieceRect 在 board.c 中定义，这里声明 */
void GetPieceRect();

/* --------------------------------------------------------------------
 * DirtyRectInvalidatePiece - 标记单个棋子位置为脏
 * -------------------------------------------------------------------- */
void DirtyRectInvalidatePiece(hWnd, row, col)
HWND hWnd;
int row, col;
{
    RECT rc;
    GetPieceRect(row, col, &rc);
    InvalidateRect(hWnd, &rc, FALSE);
}

/* --------------------------------------------------------------------
 * DirtyRectInvalidateSelect - 选中状态变化的脏矩形
 *
 * 旧选中位置和新选中位置都需要重绘（一个去高亮，一个加高亮）。
 * -------------------------------------------------------------------- */
void DirtyRectInvalidateSelect(hWnd, oldRow, oldCol, newRow, newCol)
HWND hWnd;
int oldRow, oldCol, newRow, newCol;
{
    if (oldRow >= 0 && oldCol >= 0)
        DirtyRectInvalidatePiece(hWnd, oldRow, oldCol);
    if (newRow >= 0 && newCol >= 0)
        DirtyRectInvalidatePiece(hWnd, newRow, newCol);

    /* 模式 2：标记格子为脏 */
    if (g_dirtyEnabled && g_dirtyMode == DR_CELL && g_cellsValid) {
        if (oldRow >= 0 && oldCol >= 0)
            g_dirtyCells[oldRow][oldCol] = 1;
        if (newRow >= 0 && newCol >= 0)
            g_dirtyCells[newRow][newCol] = 1;
    }
}

/* --------------------------------------------------------------------
 * DirtyRectInvalidateMove - 走棋时的脏矩形
 *
 * 源位置（棋子移走）、目标位置（棋子落下）、被吃位置（如有）。
 * -------------------------------------------------------------------- */
void DirtyRectInvalidateMove(hWnd, fromRow, fromCol, toRow, toCol,
                             capRow, capCol)
HWND hWnd;
int fromRow, fromCol, toRow, toCol, capRow, capCol;
{
    DirtyRectInvalidatePiece(hWnd, fromRow, fromCol);
    DirtyRectInvalidatePiece(hWnd, toRow, toCol);
    if (capRow >= 0 && capCol >= 0)
        DirtyRectInvalidatePiece(hWnd, capRow, capCol);

    /* 模式 2：标记格子为脏 */
    if (g_dirtyEnabled && g_dirtyMode == DR_CELL && g_cellsValid) {
        g_dirtyCells[fromRow][fromCol] = 1;
        g_dirtyCells[toRow][toCol] = 1;
        if (capRow >= 0 && capCol >= 0)
            g_dirtyCells[capRow][capCol] = 1;
    }
}

/* --------------------------------------------------------------------
 * DirtyRectInvalidateBoardDiff - 比较新旧棋盘，标记变化的位置
 *
 * 用于 AI 走棋后（不知道具体移动了哪里）和悔棋后。
 * -------------------------------------------------------------------- */
void DirtyRectInvalidateBoardDiff(hWnd, oldBoard, newBoard)
HWND hWnd;
int oldBoard[][BOARD_SIZE];
int newBoard[][BOARD_SIZE];
{
    int row, col;

    for (row = 0; row < BOARD_SIZE; row++) {
        for (col = 0; col < BOARD_SIZE; col++) {
            if (oldBoard[row][col] != newBoard[row][col]) {
                DirtyRectInvalidatePiece(hWnd, row, col);

                /* 模式 2：标记格子为脏 */
                if (g_dirtyEnabled && g_dirtyMode == DR_CELL && g_cellsValid)
                    g_dirtyCells[row][col] = 1;
            }
        }
    }

    /* 选中状态变化也要检查 */
    /* 高亮环可能需要重绘——全量刷新选中位置 */
    if (gameState.selRow >= 0 && gameState.selCol >= 0)
        DirtyRectInvalidatePiece(hWnd, gameState.selRow, gameState.selCol);
}

/* --------------------------------------------------------------------
 * RenderCleanBoard - 在 hBackDC 上渲染干净棋盘（无棋子）
 *
 * 用于模式 2 的格子位图源：从 hBackDC BitBlt 到格子位图，
 * 自动获得正确的背景 + 网格线。
 * -------------------------------------------------------------------- */
void RenderCleanBoard()
{
    RECT rcMem;
    int boardW, boardH;
    DWORD oldOrg;

    if (hBackDC == NULL || hBackBitmap == NULL) return;

    boardW = (boardLayout.baseRect.right - boardLayout.baseRect.left)
           + boardLayout.shadowOffset;
    boardH = (boardLayout.baseRect.bottom - boardLayout.baseRect.top)
           + boardLayout.shadowOffset;

    /* 清除位图——填充背景色 */
    rcMem.left = 0; rcMem.top = 0;
    rcMem.right = boardW; rcMem.bottom = boardH;
    DrawBackground(hBackDC, &rcMem);

    /* 画棋盘底座 + 网格（不画棋子） */
    oldOrg = SetViewportOrg(hBackDC,
                            -boardLayout.baseRect.left,
                            -boardLayout.baseRect.top);
    DrawBoardBase(hBackDC);
    DrawGrid(hBackDC);
    SetViewportOrg(hBackDC, LOWORD(oldOrg), HIWORD(oldOrg));
}

/* --------------------------------------------------------------------
 * InitCellBuffers - 创建 16 个格子位图（模式 2）
 *
 * 每个位图大小 = cellRadius * 2，包含背景 + 网格 + 棋子。
 * 从 hBackDC（干净棋盘）BitBlt 获取背景。
 * -------------------------------------------------------------------- */
void InitCellBuffers(hWnd)
HWND hWnd;
{
    HDC hdc;
    int row, col;

    if (g_hCellDC != NULL) return;  /* 已创建 */

    /* 确保 hBackDC 存在且有干净棋盘 */
    if (hBackDC == NULL || hBackBitmap == NULL) {
        hdc = GetDC(hWnd);
        if (hdc == NULL) return;

        hBackDC = CreateCompatibleDC(hdc);
        if (hBackDC != NULL) {
            int boardW, boardH;
            boardW = (boardLayout.baseRect.right - boardLayout.baseRect.left)
                   + boardLayout.shadowOffset;
            boardH = (boardLayout.baseRect.bottom - boardLayout.baseRect.top)
                   + boardLayout.shadowOffset;
            hBackBitmap = CreateCompatibleBitmap(hdc, boardW, boardH);
            if (hBackBitmap != NULL)
                hBackOldBitmap = SelectObject(hBackDC, hBackBitmap);
        }
        ReleaseDC(hWnd, hdc);
    }

    if (hBackDC == NULL || hBackBitmap == NULL) return;

    /* 渲染干净棋盘到 hBackDC */
    RenderCleanBoard();

    /* 创建格子 DC */
    hdc = GetDC(hWnd);
    if (hdc == NULL) return;

    g_hCellDC = CreateCompatibleDC(hdc);
    if (g_hCellDC == NULL) {
        ReleaseDC(hWnd, hdc);
        return;
    }

    /* 计算格子位图尺寸 */
    g_cellRadius = boardLayout.cellSize / 3 + 8;
    g_cellBmpSize = g_cellRadius * 2;

    /* 创建 16 个格子位图 */
    for (row = 0; row < BOARD_SIZE; row++) {
        for (col = 0; col < BOARD_SIZE; col++) {
            g_hbmCells[row][col] = CreateCompatibleBitmap(hdc,
                g_cellBmpSize, g_cellBmpSize);
            g_dirtyCells[row][col] = 1;  /* 标记全部为脏 */
        }
    }

    ReleaseDC(hWnd, hdc);
    g_cellsValid = 1;
}

/* --------------------------------------------------------------------
 * FreeCellBuffers - 销毁格子位图
 * -------------------------------------------------------------------- */
void FreeCellBuffers()
{
    int row, col;

    for (row = 0; row < BOARD_SIZE; row++) {
        for (col = 0; col < BOARD_SIZE; col++) {
            if (g_hbmCells[row][col] != NULL) {
                DeleteObject(g_hbmCells[row][col]);
                g_hbmCells[row][col] = NULL;
            }
            g_dirtyCells[row][col] = 0;
        }
    }

    if (g_hCellDC != NULL) {
        DeleteDC(g_hCellDC);
        g_hCellDC = NULL;
    }

    g_cellsValid = 0;
    g_cellRadius = 0;
    g_cellBmpSize = 0;
}

/* --------------------------------------------------------------------
 * RenderCellBitmap - 渲染单个格子位图
 *
 * 1. 从 hBackDC BitBlt 干净背景（含网格）
 * 2. 在格子位图上画棋子（如有）
 * 3. 画高亮环（如选中）
 * -------------------------------------------------------------------- */
void RenderCellBitmap(row, col)
int row, col;
{
    int px, py, r;
    HBITMAP oldBmp;
    DWORD oldOrg;
    int srcX, srcY;

    if (g_hCellDC == NULL || g_hbmCells[row][col] == NULL) return;
    if (hBackDC == NULL) return;

    GetPieceCenter(row, col, &px, &py);
    r = g_cellRadius;

    /* 选中格子位图 */
    oldBmp = SelectObject(g_hCellDC, g_hbmCells[row][col]);

    /* 1. 从 hBackDC BitBlt 干净背景 */
    /* hBackDC 坐标原点对应 screen 的 baseRect.left, baseRect.top */
    srcX = px - r - boardLayout.baseRect.left;
    srcY = py - r - boardLayout.baseRect.top;
    BitBlt(g_hCellDC, 0, 0, g_cellBmpSize, g_cellBmpSize,
           hBackDC, srcX, srcY, SRCCOPY);

    /* 2. 画棋子（如有）——用 SetViewportOrg 偏移坐标 */
    oldOrg = SetViewportOrg(g_hCellDC, r - px, r - py);

    if (gameState.board[row][col] != EMPTY) {
        DrawPiece(g_hCellDC, row, col, gameState.board[row][col]);
    }

    /* 3. 画高亮环（如选中） */
    if (gameState.selRow == row && gameState.selCol == col) {
        int radius = boardLayout.cellSize / 3;
        HPEN hPen, hOldPen;
        hPen = CreatePen(PS_SOLID, 2, RGB(255, 255, 0));
        hOldPen = SelectObject(g_hCellDC, hPen);
        SelectObject(g_hCellDC, GetStockObject(NULL_BRUSH));
        Ellipse(g_hCellDC, px - radius - 3, py - radius - 3,
                px + radius + 3, py + radius + 3);
        SelectObject(g_hCellDC, hOldPen);
        DeleteObject(hPen);
    }

    SetViewportOrg(g_hCellDC, LOWORD(oldOrg), HIWORD(oldOrg));
    SelectObject(g_hCellDC, oldBmp);

    g_dirtyCells[row][col] = 0;
}

/* --------------------------------------------------------------------
 * DirtyRectEnterMode - 进入脏矩形模式
 *
 * 根据当前模式初始化资源。
 * -------------------------------------------------------------------- */
void DirtyRectEnterMode(hWnd)
HWND hWnd;
{
    if (g_dirtyMode == DR_CELL) {
        /* 模式 2：创建格子位图 */
        InitCellBuffers(hWnd);
    }
    /* 模式 0 和 1 不需要额外资源 */
}

/* --------------------------------------------------------------------
 * DirtyRectExitMode - 退出脏矩形模式
 *
 * 清理当前模式的资源。
 * -------------------------------------------------------------------- */
void DirtyRectExitMode()
{
    if (g_dirtyMode == DR_CELL) {
        FreeCellBuffers();
    }
}

/* --------------------------------------------------------------------
 * DirtyRectEnable - 开启/关闭脏矩形
 *
 * 开启时进入当前模式，关闭时退出模式。
 * -------------------------------------------------------------------- */
void DirtyRectEnable(hWnd, enable)
HWND hWnd;
int enable;
{
    if (enable) {
        if (!g_dirtyEnabled) {
            g_dirtyEnabled = 1;
            DirtyRectEnterMode(hWnd);
        }
    } else {
        if (g_dirtyEnabled) {
            DirtyRectExitMode();
            g_dirtyEnabled = 0;
        }
    }
}

/* --------------------------------------------------------------------
 * DirtyRectSetMode - 切换脏矩形模式
 *
 * 先退出旧模式，再进入新模式。
 * -------------------------------------------------------------------- */
void DirtyRectSetMode(hWnd, newMode)
HWND hWnd;
int newMode;
{
    if (g_dirtyMode == newMode) return;

    if (g_dirtyEnabled)
        DirtyRectExitMode();

    g_dirtyMode = newMode;

    if (g_dirtyEnabled)
        DirtyRectEnterMode(hWnd);
}

/* --------------------------------------------------------------------
 * DirtyRectCleanup - WM_DESTROY 时清理所有资源
 * -------------------------------------------------------------------- */
void DirtyRectCleanup()
{
    FreeCellBuffers();
    g_dirtyEnabled = 0;
}

/* --------------------------------------------------------------------
 * DirtyRectPaint - 脏矩形模式的 WM_PAINT 处理
 *
 * 由 WM_PAINT 调用，替代原始的全量双缓冲绘制。
 * 返回 1 表示已处理，0 表示未处理（应走原始路径）。
 * -------------------------------------------------------------------- */
int DirtyRectPaint(hWnd, hdc, ps)
HWND hWnd;
HDC hdc;
PAINTSTRUCT *ps;
{
    RECT rcBoard, rcMargin, rcInter;
    int boardW, boardH;
    int row, col;

    if (!g_dirtyEnabled) return 0;

    /* === 模式 0：DC 直接绘制 === */
    if (g_dirtyMode == DR_DC) {
        /* 1. 填充脏区域的背景 */
        DrawBackground(hdc, &ps->rcPaint);

        /* 2. 画棋盘底座 + 网格 + 棋子——全部裁剪到 BeginPaint 的区域 */
        DrawBoardBase(hdc);
        DrawGrid(hdc);
        DrawPieces(hdc);
        return 1;
    }

    /* === 模式 1：局部更新 === */
    if (g_dirtyMode == DR_PARTIAL) {
        /* 确保 hBackDC 存在 */
        if (hBackDC == NULL || hBackBitmap == NULL) {
            /* 回退到直接画屏 */
            DrawBackground(hdc, &ps->rcPaint);
            DrawBoardBase(hdc);
            DrawGrid(hdc);
            DrawPieces(hdc);
            return 1;
        }

        /* 在 hBackDC 上全量重绘棋盘（内存操作，速度快） */
        {
            DWORD oldOrg;
            RECT rcMem;

            boardW = (boardLayout.baseRect.right - boardLayout.baseRect.left)
                   + boardLayout.shadowOffset;
            boardH = (boardLayout.baseRect.bottom - boardLayout.baseRect.top)
                   + boardLayout.shadowOffset;

            rcMem.left = 0; rcMem.top = 0;
            rcMem.right = boardW; rcMem.bottom = boardH;
            DrawBackground(hBackDC, &rcMem);

            oldOrg = SetViewportOrg(hBackDC,
                                    -boardLayout.baseRect.left,
                                    -boardLayout.baseRect.top);
            DrawBoardBase(hBackDC);
            DrawGrid(hBackDC);
            DrawPieces(hBackDC);
            SetViewportOrg(hBackDC, LOWORD(oldOrg), HIWORD(oldOrg));
        }

        /* 填充脏区域中的边距背景 */
        rcBoard.left   = boardLayout.baseRect.left;
        rcBoard.top    = boardLayout.baseRect.top;
        rcBoard.right  = boardLayout.baseRect.right + boardLayout.shadowOffset;
        rcBoard.bottom = boardLayout.baseRect.bottom + boardLayout.shadowOffset;

        /* 上边距 */
        if (ps->rcPaint.top < rcBoard.top) {
            rcMargin.left = ps->rcPaint.left;
            rcMargin.top = ps->rcPaint.top;
            rcMargin.right = ps->rcPaint.right;
            rcMargin.bottom = rcBoard.top;
            if (rcMargin.bottom > rcMargin.top)
                DrawBackground(hdc, &rcMargin);
        }
        /* 下边距 */
        if (ps->rcPaint.bottom > rcBoard.bottom) {
            rcMargin.left = ps->rcPaint.left;
            rcMargin.top = rcBoard.bottom;
            rcMargin.right = ps->rcPaint.right;
            rcMargin.bottom = ps->rcPaint.bottom;
            if (rcMargin.bottom > rcMargin.top)
                DrawBackground(hdc, &rcMargin);
        }
        /* 左边距 */
        if (ps->rcPaint.left < rcBoard.left) {
            rcMargin.left = ps->rcPaint.left;
            rcMargin.top = rcBoard.top;
            rcMargin.right = rcBoard.left;
            rcMargin.bottom = rcBoard.bottom;
            if (rcMargin.right > rcMargin.left)
                DrawBackground(hdc, &rcMargin);
        }
        /* 右边距 */
        if (ps->rcPaint.right > rcBoard.right) {
            rcMargin.left = rcBoard.right;
            rcMargin.top = rcBoard.top;
            rcMargin.right = ps->rcPaint.right;
            rcMargin.bottom = rcBoard.bottom;
            if (rcMargin.right > rcMargin.left)
                DrawBackground(hdc, &rcMargin);
        }

        /* BitBlt 脏区域与棋盘区域的交集 */
        if (IntersectRect(&rcInter, &ps->rcPaint, &rcBoard)) {
            int srcX, srcY, dstX, dstY, w, h;
            dstX = rcInter.left;
            dstY = rcInter.top;
            w = rcInter.right - rcInter.left;
            h = rcInter.bottom - rcInter.top;
            srcX = rcInter.left - boardLayout.baseRect.left;
            srcY = rcInter.top - boardLayout.baseRect.top;
            BitBlt(hdc, dstX, dstY, w, h,
                   hBackDC, srcX, srcY, SRCCOPY);
        }
        return 1;
    }

    /* === 模式 2：格子级双缓冲 === */
    if (g_dirtyMode == DR_CELL) {
        if (!g_cellsValid || g_hCellDC == NULL) {
            /* 格子位图未创建，回退 */
            DrawBackground(hdc, &ps->rcPaint);
            DrawBoardBase(hdc);
            DrawGrid(hdc);
            DrawPieces(hdc);
            return 1;
        }

        /* 先画棋盘底座 + 网格（直接画屏，纯色/线条不闪） */
        rcBoard.left   = boardLayout.baseRect.left;
        rcBoard.top    = boardLayout.baseRect.top;
        rcBoard.right  = boardLayout.baseRect.right + boardLayout.shadowOffset;
        rcBoard.bottom = boardLayout.baseRect.bottom + boardLayout.shadowOffset;

        /* 如果脏区域与棋盘区域相交，重画底座 + 网格 */
        if (IntersectRect(&rcInter, &ps->rcPaint, &rcBoard)) {
            /* 底座 + 网格直接画到屏幕 */
            DrawBoardBase(hdc);
            DrawGrid(hdc);
        }

        /* 填充边距背景 */
        /* 上边距 */
        if (ps->rcPaint.top < boardLayout.baseRect.top) {
            rcMargin.left = ps->rcPaint.left;
            rcMargin.top = ps->rcPaint.top;
            rcMargin.right = ps->rcPaint.right;
            rcMargin.bottom = boardLayout.baseRect.top;
            if (rcMargin.bottom > rcMargin.top)
                DrawBackground(hdc, &rcMargin);
        }
        /* 下边距 */
        if (ps->rcPaint.bottom > rcBoard.bottom) {
            rcMargin.left = ps->rcPaint.left;
            rcMargin.top = rcBoard.bottom;
            rcMargin.right = ps->rcPaint.right;
            rcMargin.bottom = ps->rcPaint.bottom;
            if (rcMargin.bottom > rcMargin.top)
                DrawBackground(hdc, &rcMargin);
        }
        /* 左边距 */
        if (ps->rcPaint.left < boardLayout.baseRect.left) {
            rcMargin.left = ps->rcPaint.left;
            rcMargin.top = boardLayout.baseRect.top;
            rcMargin.right = boardLayout.baseRect.left;
            rcMargin.bottom = rcBoard.bottom;
            if (rcMargin.right > rcMargin.left)
                DrawBackground(hdc, &rcMargin);
        }
        /* 右边距 */
        if (ps->rcPaint.right > rcBoard.right) {
            rcMargin.left = rcBoard.right;
            rcMargin.top = boardLayout.baseRect.top;
            rcMargin.right = ps->rcPaint.right;
            rcMargin.bottom = rcBoard.bottom;
            if (rcMargin.right > rcMargin.left)
                DrawBackground(hdc, &rcMargin);
        }

        /* 渲染并 BitBlt 脏格子 */
        for (row = 0; row < BOARD_SIZE; row++) {
            for (col = 0; col < BOARD_SIZE; col++) {
                RECT rcPiece;
                int px, py;

                GetPieceRect(row, col, &rcPiece);

                /* 检查格子是否在脏区域内 */
                if (!IntersectRect(&rcInter, &rcPiece, &ps->rcPaint))
                    continue;

                /* 重新渲染格子位图 */
                RenderCellBitmap(row, col);

                /* BitBlt 格子位图到屏幕 */
                GetPieceCenter(row, col, &px, &py);
                BitBlt(hdc,
                       px - g_cellRadius, py - g_cellRadius,
                       g_cellBmpSize, g_cellBmpSize,
                       g_hCellDC, 0, 0, SRCCOPY);
            }
        }
        return 1;
    }

    return 0;
}
