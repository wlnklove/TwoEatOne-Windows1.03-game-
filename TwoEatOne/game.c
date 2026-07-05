/* ====================================================================
 * game.c - 游戏逻辑模块
 * 
 * 包含棋盘初始化、走棋验证、执行走棋、吃子判定、
 * 胜负判定、有效走法生成、悔棋等功能。
 * 
 * 吃子规则详解：
 * 
 * 【普通模式 - 二吃一】
 *   一条直线（横/竖）上刚好 3 颗棋子，己方两颗相邻，
 *   对方一颗紧挨，即可吃掉对方那颗。
 *   - 白(0)走棋后：001 或 100 → 黑被吃
 *   - 黑(1)走棋后：110 或 011 → 白被吃
 *   - 4 颗同线不触发吃子
 *   - 对方先连两子，我走入构成 3 颗，不被吃（仅走棋方可吃）
 * 
 * 【挑战模式 - 夹吃】
 *   两条己方棋子夹住一颗对方棋子（101 或 010），中间那颗被吃。
 *   同行四子不触发吃子。
 * ==================================================================== */

#include "twoeat.h"

/* 内部函数前向声明（K&R C 要求先声明后使用） */
static int CheckCaptureNormal();
static int CheckCaptureChallenge();

/* --------------------------------------------------------------------
 * InitBoard - 初始化棋盘
 * 
 * 清空棋盘并摆放初始棋子。
 * -------------------------------------------------------------------- */
void InitBoard()
{
    int row, col;

    for (row = 0; row < BOARD_SIZE; row++) {
        for (col = 0; col < BOARD_SIZE; col++) {
            gameState.board[row][col] = EMPTY;
        }
    }

    /* 黑棋（AI）在顶部第 0 行 */
    for (col = 0; col < BOARD_SIZE; col++)
        gameState.board[0][col] = BLACK;

    /* 白棋（玩家）在底部第 3 行 */
    for (col = 0; col < BOARD_SIZE; col++)
        gameState.board[3][col] = WHITE;
}

/* --------------------------------------------------------------------
 * IsAdjacent - 判断两个位置是否正交相邻
 * 
 * 只允许上下左右一步移动，不允许斜走。
 * -------------------------------------------------------------------- */
BOOL IsAdjacent(r1, c1, r2, c2)
int r1, c1, r2, c2;
{
    int dr, dc;
    dr = r1 - r2;
    dc = c1 - c2;

    /* 正交相邻：行差为±1且列差为0，或列差为±1且行差为0 */
    if ((dr == 1 || dr == -1) && dc == 0)
        return TRUE;
    if ((dc == 1 || dc == -1) && dr == 0)
        return TRUE;

    return FALSE;
}

/* --------------------------------------------------------------------
 * IsValidMove - 验证走法是否合法
 * 
 * 条件：
 *   1. 起点和终点在棋盘范围内
 *   2. 起点有当前玩家的棋子
 *   3. 终点为空
 *   4. 起点和终点正交相邻（一步）
 * -------------------------------------------------------------------- */
BOOL IsValidMove(fromRow, fromCol, toRow, toCol)
int fromRow, fromCol, toRow, toCol;
{
    /* 边界检查 */
    if (fromRow < 0 || fromRow >= BOARD_SIZE) return FALSE;
    if (fromCol < 0 || fromCol >= BOARD_SIZE) return FALSE;
    if (toRow < 0 || toRow >= BOARD_SIZE) return FALSE;
    if (toCol < 0 || toCol >= BOARD_SIZE) return FALSE;

    /* 起点必须有棋子 */
    if (gameState.board[fromRow][fromCol] == EMPTY) return FALSE;

    /* 终点必须为空 */
    if (gameState.board[toRow][toCol] != EMPTY) return FALSE;

    /* 必须正交相邻 */
    if (!IsAdjacent(fromRow, fromCol, toRow, toCol)) return FALSE;

    return TRUE;
}

/* --------------------------------------------------------------------
 * MakeMove - 执行走棋
 * 
 * 1. 保存当前状态到历史记录（用于悔棋）
 * 2. 移动棋子
 * 3. 检查吃子
 * 4. 更新棋子计数
 * 5. 检查胜负
 * 6. 切换走棋方
 * -------------------------------------------------------------------- */
void MakeMove(fromRow, fromCol, toRow, toCol)
int fromRow, fromCol, toRow, toCol;
{
    int player;
    int captured;

    /* 保存历史记录 */
    SaveHistoryEntry();

    player = gameState.board[fromRow][fromCol];

    /* 移动棋子 */
    gameState.board[fromRow][fromCol] = EMPTY;
    gameState.board[toRow][toCol] = player;

    /* 记录最后走棋位置 */
    gameState.lastMoveRow = toRow;
    gameState.lastMoveCol = toCol;

    /* 检查吃子 */
    captured = CheckCapture(toRow, toCol, player);

    /* 更新棋子计数 */
    if (captured > 0) {
        if (player == WHITE) {
            gameState.blackCount -= captured;
        } else {
            gameState.whiteCount -= captured;
        }
    }

    /* 检查胜负 */
    /* 条件1：对方棋子剩 1 颗 → 走棋方胜 */
    if (player == WHITE && gameState.blackCount <= 1) {
        gameState.gameOver = WHITE;
        return;
    }
    if (player == BLACK && gameState.whiteCount <= 1) {
        gameState.gameOver = BLACK;
        return;
    }

    /* 切换走棋方 */
    gameState.currentPlayer = (player == WHITE) ? BLACK : WHITE;

    /* 条件2：对方无棋可走 → 走棋方胜 */
    if (!CanPlayerMove(gameState.currentPlayer)) {
        gameState.gameOver = player;
        return;
    }
}

/* --------------------------------------------------------------------
 * CheckCapture - 检查吃子
 * 
 * 走棋后调用，检查刚走的棋子是否构成吃子。
 * 根据游戏模式调用不同的吃子判定逻辑。
 * 
 * 返回被吃掉的棋子数量。
 * -------------------------------------------------------------------- */
int CheckCapture(row, col, player)
int row, col, player;
{
    if (gameState.mode == MODE_NORMAL) {
        return CheckCaptureNormal(row, col, player);
    } else {
        return CheckCaptureChallenge(row, col, player);
    }
}

/* --------------------------------------------------------------------
 * 【内部函数】CheckCaptureNormal - 普通模式吃子判定
 * 
 * 普通模式规则：
 *   检查走棋位置所在的行和列：
 *   - 如果整行/列有 4 颗棋子 → 不触发吃子
 *   - 否则检查 3 连续位置窗口（0-1-2 和 1-2-3）
 *   - 窗口内 3 颗全满且为"己方2+对方1"模式 → 吃掉对方那颗
 * 
 *   白(1)走棋：1,1,2 → 吃位置2的黑; 2,1,1 → 吃位置0的黑
 *   黑(2)走棋：2,2,1 → 吃位置2的白; 1,2,2 → 吃位置0的白
 * -------------------------------------------------------------------- */
static int CheckCaptureNormal(row, col, player)
int row, col, player;
{
    int opponent;
    int captured = 0;
    int i, count;
    int v[4];

    opponent = (player == WHITE) ? BLACK : WHITE;

    /* ---- 检查行方向 ---- */
    /* 统计该行的棋子数 */
    count = 0;
    for (i = 0; i < 4; i++) {
        v[i] = gameState.board[row][i];
        if (v[i] != EMPTY) count++;
    }

    /* 4 颗不触发，只有 <4 颗时检查 3 连续窗口 */
    if (count < 4) {
        /* 检查窗口 [0,1,2] */
        if (v[0] != EMPTY && v[1] != EMPTY && v[2] != EMPTY) {
            if (player == WHITE && v[0] == WHITE && v[1] == WHITE && v[2] == BLACK) {
                gameState.board[row][2] = EMPTY;
                captured++;
            } else if (player == WHITE && v[0] == BLACK && v[1] == WHITE && v[2] == WHITE) {
                gameState.board[row][0] = EMPTY;
                captured++;
            } else if (player == BLACK && v[0] == BLACK && v[1] == BLACK && v[2] == WHITE) {
                gameState.board[row][2] = EMPTY;
                captured++;
            } else if (player == BLACK && v[0] == WHITE && v[1] == BLACK && v[2] == BLACK) {
                gameState.board[row][0] = EMPTY;
                captured++;
            }
        }

        /* 检查窗口 [1,2,3] */
        if (v[1] != EMPTY && v[2] != EMPTY && v[3] != EMPTY) {
            if (player == WHITE && v[1] == WHITE && v[2] == WHITE && v[3] == BLACK) {
                gameState.board[row][3] = EMPTY;
                captured++;
            } else if (player == WHITE && v[1] == BLACK && v[2] == WHITE && v[3] == WHITE) {
                gameState.board[row][1] = EMPTY;
                captured++;
            } else if (player == BLACK && v[1] == BLACK && v[2] == BLACK && v[3] == WHITE) {
                gameState.board[row][3] = EMPTY;
                captured++;
            } else if (player == BLACK && v[1] == WHITE && v[2] == BLACK && v[3] == BLACK) {
                gameState.board[row][1] = EMPTY;
                captured++;
            }
        }
    }

    /* ---- 检查列方向 ---- */
    count = 0;
    for (i = 0; i < 4; i++) {
        v[i] = gameState.board[i][col];
        if (v[i] != EMPTY) count++;
    }

    if (count < 4) {
        /* 检查窗口 [0,1,2] */
        if (v[0] != EMPTY && v[1] != EMPTY && v[2] != EMPTY) {
            if (player == WHITE && v[0] == WHITE && v[1] == WHITE && v[2] == BLACK) {
                gameState.board[2][col] = EMPTY;
                captured++;
            } else if (player == WHITE && v[0] == BLACK && v[1] == WHITE && v[2] == WHITE) {
                gameState.board[0][col] = EMPTY;
                captured++;
            } else if (player == BLACK && v[0] == BLACK && v[1] == BLACK && v[2] == WHITE) {
                gameState.board[2][col] = EMPTY;
                captured++;
            } else if (player == BLACK && v[0] == WHITE && v[1] == BLACK && v[2] == BLACK) {
                gameState.board[0][col] = EMPTY;
                captured++;
            }
        }

        /* 检查窗口 [1,2,3] */
        if (v[1] != EMPTY && v[2] != EMPTY && v[3] != EMPTY) {
            if (player == WHITE && v[1] == WHITE && v[2] == WHITE && v[3] == BLACK) {
                gameState.board[3][col] = EMPTY;
                captured++;
            } else if (player == WHITE && v[1] == BLACK && v[2] == WHITE && v[3] == WHITE) {
                gameState.board[1][col] = EMPTY;
                captured++;
            } else if (player == BLACK && v[1] == BLACK && v[2] == BLACK && v[3] == WHITE) {
                gameState.board[3][col] = EMPTY;
                captured++;
            } else if (player == BLACK && v[1] == WHITE && v[2] == BLACK && v[3] == BLACK) {
                gameState.board[1][col] = EMPTY;
                captured++;
            }
        }
    }

    return captured;
}

/* --------------------------------------------------------------------
 * 【内部函数】CheckCaptureChallenge - 挑战模式吃子判定
 * 
 * 挑战模式规则（夹吃）：
 *   走棋后，检查刚走的棋子是否与另一颗己方棋子
 *   夹住一颗对方棋子（中间无空格）。
 *   
 *   检查方向：左、右、上、下
 *   
 *   例如：走棋位置为 P，左边 P-1 是对方，P-2 是己方 → 吃 P-1
 *   
 *   同行/列 4 颗不触发吃子。
 * -------------------------------------------------------------------- */
static int CheckCaptureChallenge(row, col, player)
int row, col, player;
{
    int opponent;
    int captured = 0;
    int i, count;
    int v[4];

    opponent = (player == WHITE) ? BLACK : WHITE;

    /* ---- 检查行方向 ---- */
    count = 0;
    for (i = 0; i < 4; i++) {
        v[i] = gameState.board[row][i];
        if (v[i] != EMPTY) count++;
    }

    if (count < 4) {
        /* 向左夹吃：col-2 是己方, col-1 是对方 */
        if (col >= 2 && gameState.board[row][col - 1] == opponent
            && gameState.board[row][col - 2] == player) {
            gameState.board[row][col - 1] = EMPTY;
            captured++;
        }

        /* 向右夹吃：col+2 是己方, col+1 是对方 */
        if (col <= 1 && gameState.board[row][col + 1] == opponent
            && gameState.board[row][col + 2] == player) {
            gameState.board[row][col + 1] = EMPTY;
            captured++;
        }
    }

    /* ---- 检查列方向 ---- */
    count = 0;
    for (i = 0; i < 4; i++) {
        v[i] = gameState.board[i][col];
        if (v[i] != EMPTY) count++;
    }

    if (count < 4) {
        /* 向上夹吃：row-2 是己方, row-1 是对方 */
        if (row >= 2 && gameState.board[row - 1][col] == opponent
            && gameState.board[row - 2][col] == player) {
            gameState.board[row - 1][col] = EMPTY;
            captured++;
        }

        /* 向下夹吃：row+2 是己方, row+1 是对方 */
        if (row <= 1 && gameState.board[row + 1][col] == opponent
            && gameState.board[row + 2][col] == player) {
            gameState.board[row + 1][col] = EMPTY;
            captured++;
        }
    }

    return captured;
}

/* --------------------------------------------------------------------
 * CanPlayerMove - 检查玩家是否有合法走法
 * 
 * 遍历所有该玩家的棋子，检查是否有至少一个可走的相邻空位。
 * 如果一个都走不了，判负。
 * -------------------------------------------------------------------- */
BOOL CanPlayerMove(player)
int player;
{
    int row, col, dr, dc;

    for (row = 0; row < BOARD_SIZE; row++) {
        for (col = 0; col < BOARD_SIZE; col++) {
            if (gameState.board[row][col] != player)
                continue;

            /* 检查四个方向是否有空位 */
            for (dr = -1; dr <= 1; dr++) {
                for (dc = -1; dc <= 1; dc++) {
                    if (dr == 0 && dc == 0) continue;
                    if (dr != 0 && dc != 0) continue; /* 跳过斜向 */

                    if (row + dr >= 0 && row + dr < BOARD_SIZE &&
                        col + dc >= 0 && col + dc < BOARD_SIZE &&
                        gameState.board[row + dr][col + dc] == EMPTY) {
                        return TRUE;
                    }
                }
            }
        }
    }
    return FALSE;
}

/* --------------------------------------------------------------------
 * GetValidMoves - 获取玩家所有合法走法
 * 
 * 遍历棋盘，找出该玩家所有可走的走法，存入 moves 数组。
 * 用于 AI 搜索。
 * 
 * 走法用 4 个整数表示：fromRow, fromCol, toRow, toCol
 * -------------------------------------------------------------------- */
void GetValidMoves(pBoard, player, moves, pCount)
int pBoard[][BOARD_SIZE];
int player;
int moves[][4];
int *pCount;
{
    int row, col, dr, dc;
    int idx = 0;

    for (row = 0; row < BOARD_SIZE; row++) {
        for (col = 0; col < BOARD_SIZE; col++) {
            if (pBoard[row][col] != player)
                continue;

            for (dr = -1; dr <= 1; dr++) {
                for (dc = -1; dc <= 1; dc++) {
                    if (dr == 0 && dc == 0) continue;
                    if (dr != 0 && dc != 0) continue;

                    if (row + dr >= 0 && row + dr < BOARD_SIZE &&
                        col + dc >= 0 && col + dc < BOARD_SIZE &&
                        pBoard[row + dr][col + dc] == EMPTY) {
                        moves[idx][0] = row;
                        moves[idx][1] = col;
                        moves[idx][2] = row + dr;
                        moves[idx][3] = col + dc;
                        idx++;
                    }
                }
            }
        }
    }
    *pCount = idx;
}

/* --------------------------------------------------------------------
 * SaveHistoryEntry - 保存历史记录
 * 
 * 在每次走棋前调用，保存当前棋盘状态。
 * 用于悔棋功能。
 * -------------------------------------------------------------------- */
void SaveHistoryEntry()
{
    int row, col;

    if (historyCurrent >= MAX_HISTORY) {
        /* 历史已满，整体前移一条 */
        for (row = 0; row < MAX_HISTORY - 1; row++) {
            history[row] = history[row + 1];
        }
        historyCurrent = MAX_HISTORY - 1;
    }

    /* 保存当前状态 */
    for (row = 0; row < BOARD_SIZE; row++) {
        for (col = 0; col < BOARD_SIZE; col++) {
            history[historyCurrent].board[row][col] = gameState.board[row][col];
        }
    }
    history[historyCurrent].currentPlayer = gameState.currentPlayer;
    history[historyCurrent].whiteCount = gameState.whiteCount;
    history[historyCurrent].blackCount = gameState.blackCount;
    history[historyCurrent].lastMoveRow = gameState.lastMoveRow;
    history[historyCurrent].lastMoveCol = gameState.lastMoveCol;

    historyCurrent++;
    historyCount = historyCurrent;
}

/* --------------------------------------------------------------------
 * UndoMove - 悔棋
 *
 * 两种模式有不同的悔棋策略：
 *
 * AI 对弈模式：回退 2 步（AI 的一步 + 玩家的一步）
 *   因为只有玩家可以发起悔棋，所以悔棋发生时一定是玩家的预备状态
 *   （AI 刚走完棋）。回退 2 步后仍然落在玩家的预备状态。
 *   回退后强制 currentPlayer = WHITE，绝不触发 AI 偷走一步。
 *
 * 双人对弈模式：回退 1 步
 *   任意一方都可以发起悔棋，只撤销最后一手棋。
 *   回退后恢复到历史记录中的走棋方（发起悔棋的人可以重走）。
 * -------------------------------------------------------------------- */
BOOL UndoMove()
{
    int stepsToUndo;
    int target;
    int row, col;

    if (gameState.twoPlayer) {
        /* 双人模式：悔 1 步 */
        stepsToUndo = 1;
    } else {
        /* AI 模式：悔 2 步（AI + 玩家） */
        stepsToUndo = 2;
    }

    /* 历史不足时减少步数 */
    if (historyCurrent < stepsToUndo + 1)
        stepsToUndo = historyCurrent - 1;
    if (stepsToUndo < 1)
        return FALSE;

    target = historyCurrent - stepsToUndo;
    if (target < 0)
        target = 0;

    /* 恢复棋盘到目标状态 */
    for (row = 0; row < BOARD_SIZE; row++) {
        for (col = 0; col < BOARD_SIZE; col++) {
            gameState.board[row][col] = history[target].board[row][col];
        }
    }

    if (gameState.twoPlayer) {
        /* 双人模式：恢复到历史记录中的走棋方 */
        gameState.currentPlayer = history[target].currentPlayer;
    } else {
        /* AI 模式：悔棋后总是玩家回合，AI 绝不偷走一步 */
        /* gameState.currentPlayer = WHITE; */
		gameState.currentPlayer = history[target].currentPlayer;
    }

    gameState.whiteCount = history[target].whiteCount;
    gameState.blackCount = history[target].blackCount;
    gameState.lastMoveRow = history[target].lastMoveRow;
    gameState.lastMoveCol = history[target].lastMoveCol;
    gameState.selRow = -1;
    gameState.selCol = -1;
    gameState.gameOver = 0;

    historyCurrent = target;
	historyCount = historyCurrent;

    return TRUE;
}

/* --------------------------------------------------------------------
 * CopyBoard - 复制棋盘（供 AI 模块使用）
 * -------------------------------------------------------------------- */
void CopyBoard(pDest, pSrc)
int pDest[][BOARD_SIZE];
int pSrc[][BOARD_SIZE];
{
    int row, col;
    for (row = 0; row < BOARD_SIZE; row++) {
        for (col = 0; col < BOARD_SIZE; col++) {
            pDest[row][col] = pSrc[row][col];
        }
    }
}

/* --------------------------------------------------------------------
 * CountMoves - 统计某方可走步数（供 AI 评估用）
 * -------------------------------------------------------------------- */
int CountMoves(pBoard, player)
int pBoard[][BOARD_SIZE];
int player;
{
    int row, col, dr, dc;
    int count = 0;

    for (row = 0; row < BOARD_SIZE; row++) {
        for (col = 0; col < BOARD_SIZE; col++) {
            if (pBoard[row][col] != player) continue;
            for (dr = -1; dr <= 1; dr++) {
                for (dc = -1; dc <= 1; dc++) {
                    if (dr == 0 && dc == 0) continue;
                    if (dr != 0 && dc != 0) continue;
                    if (row + dr >= 0 && row + dr < BOARD_SIZE &&
                        col + dc >= 0 && col + dc < BOARD_SIZE &&
                        pBoard[row + dr][col + dc] == EMPTY) {
                        count++;
                    }
                }
            }
        }
    }
    return count;
}
