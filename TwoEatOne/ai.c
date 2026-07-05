/* ====================================================================
 * ai.c - AI 逻辑模块
 * 
 * 使用 Minimax + Alpha-Beta 剪枝算法实现 AI 对手。
 * 普通模式和挑战模式各有独立的评估函数和吃子模拟。
 * 
 * AI 始终操控黑棋（上方），玩家始终操控白棋（下方）。
 * 先后手只影响走棋顺序，不改变棋子位置。
 * 
 * 搜索深度：3 层（在 16 位环境下兼顾性能和棋力）
 * ==================================================================== */

#include "twoeat.h"

/* 内部函数原型 */
static int SimCaptureNormal();
static int SimCaptureChallenge();
static void SimMove();
static int MaxValue();
static int MinValue();
static int MaxValueChallenge();
static int MinValueChallenge();

#define AI_DEPTH    3           /* AI 搜索深度 */
#define INFINITY    32000       /* 无穷大值（16位安全范围） */

/* --------------------------------------------------------------------
 * DoAIMove - AI 走棋
 * 
 * 调用 Minimax 搜索找到最佳走法并执行。
 * 
 * 流程：
 *   1. 生成所有合法走法
 *   2. 对每个走法进行 Minimax 搜索
 *   3. 选择得分最高的走法
 *   4. 执行走棋
 * -------------------------------------------------------------------- */
void DoAIMove()
{
    int moves[32][4];
    int moveCount;
    int bestScore;
    int bestMove;
    int i;
    int score;
    int tempBoard[BOARD_SIZE][BOARD_SIZE];
    int whiteCnt, blackCnt;

    /* 获取所有合法走法 */
    GetValidMoves(gameState.board, BLACK, moves, &moveCount);

    if (moveCount == 0) {
        /* AI 无棋可走，玩家胜 */
        gameState.gameOver = WHITE;
        return;
    }

    bestScore = -INFINITY;
    bestMove = 0;

    /* 保存当前棋子数 */
    whiteCnt = gameState.whiteCount;
    blackCnt = gameState.blackCount;

    /* 遍历所有走法，找最优 */
    for (i = 0; i < moveCount; i++) {
        /* 在临时棋盘上模拟走棋 */
        CopyBoard(tempBoard, gameState.board);
        SimMove(tempBoard, moves[i][0], moves[i][1], moves[i][2], moves[i][3]);

        /* Minimax 搜索（AI 是最大化方） */
        if (gameState.mode == MODE_NORMAL) {
            score = MinValue(tempBoard, AI_DEPTH - 1, -INFINITY, INFINITY, WHITE);
        } else {
            score = MinValueChallenge(tempBoard, AI_DEPTH - 1, -INFINITY, INFINITY, WHITE);
        }

        if (score > bestScore) {
            bestScore = score;
            bestMove = i;
        }
    }

    /* 执行最佳走法 */
    MakeMove(moves[bestMove][0], moves[bestMove][1],
             moves[bestMove][2], moves[bestMove][3]);
}

/* --------------------------------------------------------------------
 * SimMove - 在临时棋盘上模拟走棋（不改变全局状态）
 * 
 * 包含吃子检查，用于 Minimax 搜索。
 * -------------------------------------------------------------------- */
static void SimMove(pBoard, fromRow, fromCol, toRow, toCol)
int pBoard[][BOARD_SIZE];
int fromRow, fromCol, toRow, toCol;
{
    int player;
    int captured;

    player = pBoard[fromRow][fromCol];
    pBoard[fromRow][fromCol] = EMPTY;
    pBoard[toRow][toCol] = player;

    /* 模拟吃子 */
    if (gameState.mode == MODE_NORMAL)
        captured = SimCaptureNormal(pBoard, toRow, toCol, player);
    else
        captured = SimCaptureChallenge(pBoard, toRow, toCol, player);
}

/* --------------------------------------------------------------------
 * SimCaptureNormal - 普通模式模拟吃子
 * 
 * 逻辑与 game.c 中的 CheckCaptureNormal 相同，
 * 但操作在临时棋盘上。
 * -------------------------------------------------------------------- */
static int SimCaptureNormal(pBoard, row, col, player)
int pBoard[][BOARD_SIZE];
int row, col, player;
{
    int opponent;
    int captured = 0;
    int i, count;
    int v[4];

    opponent = (player == WHITE) ? BLACK : WHITE;

    /* 检查行 */
    count = 0;
    for (i = 0; i < 4; i++) {
        v[i] = pBoard[row][i];
        if (v[i] != EMPTY) count++;
    }

    if (count < 4) {
        if (v[0] != EMPTY && v[1] != EMPTY && v[2] != EMPTY) {
            if (player == WHITE && v[0] == WHITE && v[1] == WHITE && v[2] == BLACK) {
                pBoard[row][2] = EMPTY; captured++;
            } else if (player == WHITE && v[0] == BLACK && v[1] == WHITE && v[2] == WHITE) {
                pBoard[row][0] = EMPTY; captured++;
            } else if (player == BLACK && v[0] == BLACK && v[1] == BLACK && v[2] == WHITE) {
                pBoard[row][2] = EMPTY; captured++;
            } else if (player == BLACK && v[0] == WHITE && v[1] == BLACK && v[2] == BLACK) {
                pBoard[row][0] = EMPTY; captured++;
            }
        }
        if (v[1] != EMPTY && v[2] != EMPTY && v[3] != EMPTY) {
            if (player == WHITE && v[1] == WHITE && v[2] == WHITE && v[3] == BLACK) {
                pBoard[row][3] = EMPTY; captured++;
            } else if (player == WHITE && v[1] == BLACK && v[2] == WHITE && v[3] == WHITE) {
                pBoard[row][1] = EMPTY; captured++;
            } else if (player == BLACK && v[1] == BLACK && v[2] == BLACK && v[3] == WHITE) {
                pBoard[row][3] = EMPTY; captured++;
            } else if (player == BLACK && v[1] == WHITE && v[2] == BLACK && v[3] == BLACK) {
                pBoard[row][1] = EMPTY; captured++;
            }
        }
    }

    /* 检查列 */
    count = 0;
    for (i = 0; i < 4; i++) {
        v[i] = pBoard[i][col];
        if (v[i] != EMPTY) count++;
    }

    if (count < 4) {
        if (v[0] != EMPTY && v[1] != EMPTY && v[2] != EMPTY) {
            if (player == WHITE && v[0] == WHITE && v[1] == WHITE && v[2] == BLACK) {
                pBoard[2][col] = EMPTY; captured++;
            } else if (player == WHITE && v[0] == BLACK && v[1] == WHITE && v[2] == WHITE) {
                pBoard[0][col] = EMPTY; captured++;
            } else if (player == BLACK && v[0] == BLACK && v[1] == BLACK && v[2] == WHITE) {
                pBoard[2][col] = EMPTY; captured++;
            } else if (player == BLACK && v[0] == WHITE && v[1] == BLACK && v[2] == BLACK) {
                pBoard[0][col] = EMPTY; captured++;
            }
        }
        if (v[1] != EMPTY && v[2] != EMPTY && v[3] != EMPTY) {
            if (player == WHITE && v[1] == WHITE && v[2] == WHITE && v[3] == BLACK) {
                pBoard[3][col] = EMPTY; captured++;
            } else if (player == WHITE && v[1] == BLACK && v[2] == WHITE && v[3] == WHITE) {
                pBoard[1][col] = EMPTY; captured++;
            } else if (player == BLACK && v[1] == BLACK && v[2] == BLACK && v[3] == WHITE) {
                pBoard[3][col] = EMPTY; captured++;
            } else if (player == BLACK && v[1] == WHITE && v[2] == BLACK && v[3] == BLACK) {
                pBoard[1][col] = EMPTY; captured++;
            }
        }
    }

    return captured;
}

/* --------------------------------------------------------------------
 * SimCaptureChallenge - 挑战模式模拟吃子
 * -------------------------------------------------------------------- */
static int SimCaptureChallenge(pBoard, row, col, player)
int pBoard[][BOARD_SIZE];
int row, col, player;
{
    int opponent;
    int captured = 0;
    int i, count;
    int v[4];

    opponent = (player == WHITE) ? BLACK : WHITE;

    /* 检查行 */
    count = 0;
    for (i = 0; i < 4; i++) {
        v[i] = pBoard[row][i];
        if (v[i] != EMPTY) count++;
    }

    if (count < 4) {
        if (col >= 2 && pBoard[row][col-1] == opponent && pBoard[row][col-2] == player) {
            pBoard[row][col-1] = EMPTY; captured++;
        }
        if (col <= 1 && pBoard[row][col+1] == opponent && pBoard[row][col+2] == player) {
            pBoard[row][col+1] = EMPTY; captured++;
        }
    }

    /* 检查列 */
    count = 0;
    for (i = 0; i < 4; i++) {
        v[i] = pBoard[i][col];
        if (v[i] != EMPTY) count++;
    }

    if (count < 4) {
        if (row >= 2 && pBoard[row-1][col] == opponent && pBoard[row-2][col] == player) {
            pBoard[row-1][col] = EMPTY; captured++;
        }
        if (row <= 1 && pBoard[row+1][col] == opponent && pBoard[row+2][col] == player) {
            pBoard[row+1][col] = EMPTY; captured++;
        }
    }

    return captured;
}

/* --------------------------------------------------------------------
 * EvaluateNormal - 普通模式评估函数
 * 
 * 从黑棋（AI）角度评估当前局面。
 * 
 * 评估要素：
 *   1. 棋子数量差：每多一颗 +100
 *   2. 机动性：每多一步可走 +3
 *   3. 威胁：能形成二吃一的位置 +20
 *   4. 危险：对方能形成二吃一的位置 -20
 * -------------------------------------------------------------------- */
int EvaluateNormal(pBoard)
int pBoard[][BOARD_SIZE];
{
    int score = 0;
    int whiteCnt = 0, blackCnt = 0;
    int row, col;
    int blackMoves, whiteMoves;
    int i, count;
    int v[4];

    /* 统计棋子数 */
    for (row = 0; row < BOARD_SIZE; row++) {
        for (col = 0; col < BOARD_SIZE; col++) {
            if (pBoard[row][col] == WHITE) whiteCnt++;
            else if (pBoard[row][col] == BLACK) blackCnt++;
        }
    }

    /* 棋子差 */
    score += (blackCnt - whiteCnt) * 100;

    /* 胜负判定 */
    if (whiteCnt <= 1) return INFINITY - 1;
    if (blackCnt <= 1) return -INFINITY + 1;

    /* 机动性 */
    blackMoves = CountMoves(pBoard, BLACK);
    whiteMoves = CountMoves(pBoard, WHITE);
    score += (blackMoves - whiteMoves) * 3;

    /* 如果一方无棋可走 */
    if (blackMoves == 0) return -INFINITY + 1;
    if (whiteMoves == 0) return INFINITY - 1;

    /* 威胁评估：检查每行/列的 3 连续窗口 */
    /* 检查行 */
    for (row = 0; row < BOARD_SIZE; row++) {
        count = 0;
        for (i = 0; i < 4; i++) {
            v[i] = pBoard[row][i];
            if (v[i] != EMPTY) count++;
        }
        if (count < 4) {
            /* 窗口 0-1-2 */
            if (v[0] == BLACK && v[1] == BLACK && v[2] == EMPTY) score += 15;
            if (v[0] == WHITE && v[1] == WHITE && v[2] == EMPTY) score -= 15;
            if (v[0] == EMPTY && v[1] == BLACK && v[2] == BLACK) score += 15;
            if (v[0] == EMPTY && v[1] == WHITE && v[2] == WHITE) score -= 15;

            /* 窗口 1-2-3 */
            if (v[1] == BLACK && v[2] == BLACK && v[3] == EMPTY) score += 15;
            if (v[1] == WHITE && v[2] == WHITE && v[3] == EMPTY) score -= 15;
            if (v[1] == EMPTY && v[2] == BLACK && v[3] == BLACK) score += 15;
            if (v[1] == EMPTY && v[2] == WHITE && v[3] == WHITE) score -= 15;
        }
    }

    /* 检查列 */
    for (col = 0; col < BOARD_SIZE; col++) {
        count = 0;
        for (i = 0; i < 4; i++) {
            v[i] = pBoard[i][col];
            if (v[i] != EMPTY) count++;
        }
        if (count < 4) {
            if (v[0] == BLACK && v[1] == BLACK && v[2] == EMPTY) score += 15;
            if (v[0] == WHITE && v[1] == WHITE && v[2] == EMPTY) score -= 15;
            if (v[0] == EMPTY && v[1] == BLACK && v[2] == BLACK) score += 15;
            if (v[0] == EMPTY && v[1] == WHITE && v[2] == WHITE) score -= 15;
            if (v[1] == BLACK && v[2] == BLACK && v[3] == EMPTY) score += 15;
            if (v[1] == WHITE && v[2] == WHITE && v[3] == EMPTY) score -= 15;
            if (v[1] == EMPTY && v[2] == BLACK && v[3] == BLACK) score += 15;
            if (v[1] == EMPTY && v[2] == WHITE && v[3] == WHITE) score -= 15;
        }
    }

    return score;
}

/* --------------------------------------------------------------------
 * EvaluateChallenge - 挑战模式评估函数
 * 
 * 评估要素类似普通模式，但威胁检测基于夹吃规则。
 * -------------------------------------------------------------------- */
int EvaluateChallenge(pBoard)
int pBoard[][BOARD_SIZE];
{
    int score = 0;
    int whiteCnt = 0, blackCnt = 0;
    int row, col;
    int blackMoves, whiteMoves;

    /* 统计棋子数 */
    for (row = 0; row < BOARD_SIZE; row++) {
        for (col = 0; col < BOARD_SIZE; col++) {
            if (pBoard[row][col] == WHITE) whiteCnt++;
            else if (pBoard[row][col] == BLACK) blackCnt++;
        }
    }

    score += (blackCnt - whiteCnt) * 100;

    if (whiteCnt <= 1) return INFINITY - 1;
    if (blackCnt <= 1) return -INFINITY + 1;

    blackMoves = CountMoves(pBoard, BLACK);
    whiteMoves = CountMoves(pBoard, WHITE);
    score += (blackMoves - whiteMoves) * 3;

    if (blackMoves == 0) return -INFINITY + 1;
    if (whiteMoves == 0) return INFINITY - 1;

    /* 夹吃威胁评估 */
    /* 检查每行：模式 X-O-_ 或 _-O-X (X=己方, O=对方, _=空) */
    for (row = 0; row < BOARD_SIZE; row++) {
        /* 水平方向 */
        for (col = 0; col <= 1; col++) {
            if (pBoard[row][col] == BLACK && pBoard[row][col+1] == WHITE
                && pBoard[row][col+2] == EMPTY) {
                /* 黑棋有机会夹吃：走 col+2 可以吃 col+1 */
                score += 15;
            }
            if (pBoard[row][col] == EMPTY && pBoard[row][col+1] == WHITE
                && pBoard[row][col+2] == BLACK) {
                score += 15;
            }
            if (pBoard[row][col] == WHITE && pBoard[row][col+1] == BLACK
                && pBoard[row][col+2] == EMPTY) {
                score -= 15;
            }
            if (pBoard[row][col] == EMPTY && pBoard[row][col+1] == BLACK
                && pBoard[row][col+2] == WHITE) {
                score -= 15;
            }
        }
    }

    /* 检查每列 */
    for (col = 0; col < BOARD_SIZE; col++) {
        for (row = 0; row <= 1; row++) {
            if (pBoard[row][col] == BLACK && pBoard[row+1][col] == WHITE
                && pBoard[row+2][col] == EMPTY) {
                score += 15;
            }
            if (pBoard[row][col] == EMPTY && pBoard[row+1][col] == WHITE
                && pBoard[row+2][col] == BLACK) {
                score += 15;
            }
            if (pBoard[row][col] == WHITE && pBoard[row+1][col] == BLACK
                && pBoard[row+2][col] == EMPTY) {
                score -= 15;
            }
            if (pBoard[row][col] == EMPTY && pBoard[row+1][col] == BLACK
                && pBoard[row+2][col] == WHITE) {
                score -= 15;
            }
        }
    }

    return score;
}

/* --------------------------------------------------------------------
 * MaxValue - Minimax 最大化层（AI/黑棋走棋）
 * 
 * Alpha-Beta 剪枝：
 *   alpha - 当前最大化层已知最优值
 *   beta  - 当前最小化层已知最优值
 *   如果 alpha >= beta，剪枝（对手不会让这个局面出现）
 * -------------------------------------------------------------------- */
static int MaxValue(pBoard, depth, alpha, beta, player)
int pBoard[][BOARD_SIZE];
int depth, alpha, beta, player;
{
    int moves[32][4];
    int moveCount;
    int i;
    int score;
    int tempBoard[BOARD_SIZE][BOARD_SIZE];

    if (depth == 0)
        return EvaluateNormal(pBoard);

    GetValidMoves(pBoard, player, moves, &moveCount);

    if (moveCount == 0)
        return -INFINITY + 1;  /* 无棋可走，判负 */

    for (i = 0; i < moveCount; i++) {
        CopyBoard(tempBoard, pBoard);
        SimMove(tempBoard, moves[i][0], moves[i][1], moves[i][2], moves[i][3]);

        score = MinValue(tempBoard, depth - 1, alpha, beta,
                         (player == WHITE) ? BLACK : WHITE);

        if (score > alpha) alpha = score;
        if (alpha >= beta) break;  /* Beta 剪枝 */
    }
    return alpha;
}

/* --------------------------------------------------------------------
 * MinValue - Minimax 最小化层（玩家/白棋走棋）
 * -------------------------------------------------------------------- */
static int MinValue(pBoard, depth, alpha, beta, player)
int pBoard[][BOARD_SIZE];
int depth, alpha, beta, player;
{
    int moves[32][4];
    int moveCount;
    int i;
    int score;
    int tempBoard[BOARD_SIZE][BOARD_SIZE];

    if (depth == 0)
        return EvaluateNormal(pBoard);

    GetValidMoves(pBoard, player, moves, &moveCount);

    if (moveCount == 0)
        return INFINITY - 1;  /* 对手无棋可走，AI 胜 */

    for (i = 0; i < moveCount; i++) {
        CopyBoard(tempBoard, pBoard);
        SimMove(tempBoard, moves[i][0], moves[i][1], moves[i][2], moves[i][3]);

        score = MaxValue(tempBoard, depth - 1, alpha, beta,
                         (player == WHITE) ? BLACK : WHITE);

        if (score < beta) beta = score;
        if (alpha >= beta) break;  /* Alpha 剪枝 */
    }
    return beta;
}

/* --------------------------------------------------------------------
 * 挑战模式的 Minimax 函数
 * -------------------------------------------------------------------- */
static int MaxValueChallenge(pBoard, depth, alpha, beta, player)
int pBoard[][BOARD_SIZE];
int depth, alpha, beta, player;
{
    int moves[32][4];
    int moveCount;
    int i, score;
    int tempBoard[BOARD_SIZE][BOARD_SIZE];

    if (depth == 0)
        return EvaluateChallenge(pBoard);

    GetValidMoves(pBoard, player, moves, &moveCount);
    if (moveCount == 0) return -INFINITY + 1;

    for (i = 0; i < moveCount; i++) {
        CopyBoard(tempBoard, pBoard);
        SimMove(tempBoard, moves[i][0], moves[i][1], moves[i][2], moves[i][3]);
        score = MinValueChallenge(tempBoard, depth - 1, alpha, beta,
                                  (player == WHITE) ? BLACK : WHITE);
        if (score > alpha) alpha = score;
        if (alpha >= beta) break;
    }
    return alpha;
}

static int MinValueChallenge(pBoard, depth, alpha, beta, player)
int pBoard[][BOARD_SIZE];
int depth, alpha, beta, player;
{
    int moves[32][4];
    int moveCount;
    int i, score;
    int tempBoard[BOARD_SIZE][BOARD_SIZE];

    if (depth == 0)
        return EvaluateChallenge(pBoard);

    GetValidMoves(pBoard, player, moves, &moveCount);
    if (moveCount == 0) return INFINITY - 1;

    for (i = 0; i < moveCount; i++) {
        CopyBoard(tempBoard, pBoard);
        SimMove(tempBoard, moves[i][0], moves[i][1], moves[i][2], moves[i][3]);
        score = MaxValueChallenge(tempBoard, depth - 1, alpha, beta,
                                  (player == WHITE) ? BLACK : WHITE);
        if (score < beta) beta = score;
        if (alpha >= beta) break;
    }
    return beta;
}
