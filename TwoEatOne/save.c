/* ====================================================================
 * save.c - 存档/读档模块
 * 
 * Win 1.03 没有 lstrcpy/lstrcmp/wsprintf/_lread/_lwrite/_lclose
 * 全部用 C 运行库函数代替：
 *   lstrcpy  -> strcpy
 *   lstrcmp  -> strcmp
 *   wsprintf -> sprintf
 *   _lread   -> read  (OpenFile 返回 DOS fd，直接用 C 运行库)
 *   _lwrite  -> write
 *   _lclose  -> close
 * ==================================================================== */

#include "twoeat.h"
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <direct.h>   /* mkdir */
#include <io.h>       /* read, write, close */
#include <fcntl.h>    /* O_RDONLY, O_WRONLY, O_CREAT, O_BINARY */

/* MSC 4.0 没有 sys\stat.h，手动定义文件权限常量 */
#ifndef S_IREAD
#define S_IREAD  0x0100
#endif
#ifndef S_IWRITE
#define S_IWRITE 0x0080
#endif

/* --------------------------------------------------------------------
 * EnsureSaveDir - 确保存档目录存在
 * -------------------------------------------------------------------- */
void EnsureSaveDir()
{
    mkdir("save");
}

/* --------------------------------------------------------------------
 * GetSaveFileName - 生成存档文件路径
 * -------------------------------------------------------------------- */
void GetSaveFileName(buffer, slot)
char *buffer;
int slot;
{
    sprintf(buffer, "save\\save%02d.sav", slot);
}

/* --------------------------------------------------------------------
 * SaveGame - 保存游戏
 * -------------------------------------------------------------------- */
BOOL SaveGame(saveName)
char *saveName;
{
    SaveData data;
    int slot;
    int row, col;
    int foundSlot;
    int i, count;
    int used;
    char filePath[32];
    static char names[MAX_SAVES][SAVE_NAME_LEN];
    static int slots[MAX_SAVES];
    int fd;

    /* 寻找同名存档或空槽位 */
    count = GetSaveList(names, slots);
    foundSlot = -1;

    for (i = 0; i < count; i++) {
        if (strcmp(names[i], saveName) == 0) {
            foundSlot = slots[i];
            break;
        }
    }

    if (foundSlot < 0) {
        for (slot = 1; slot <= MAX_SAVES; slot++) {
            used = 0;
            for (i = 0; i < count; i++) {
                if (slots[i] == slot) {
                    used = 1;
                    break;
                }
            }
            if (!used) {
                foundSlot = slot;
                break;
            }
        }
    }

    if (foundSlot < 0)
        return FALSE;

    /* 填充存档数据 */
    data.magic[0] = 'T'; data.magic[1] = 'E';
    data.magic[2] = 'O'; data.magic[3] = '1';
    data.version = 1;
    data.mode = gameState.mode;

    for (row = 0; row < BOARD_SIZE; row++) {
        for (col = 0; col < BOARD_SIZE; col++) {
            data.board[row][col] = gameState.board[row][col];
        }
    }

    data.currentPlayer = gameState.currentPlayer;
    data.whiteCount = gameState.whiteCount;
    data.blackCount = gameState.blackCount;
    data.firstPlayer = gameState.firstPlayer;
    data.firstSetting = gameState.firstSetting;
    data.twoPlayer = gameState.twoPlayer;
    data.egaMode = gameState.egaMode;
    data.gameOver = gameState.gameOver;

    strcpy(data.saveName, saveName);
    data.timestamp = time(NULL);

    /* 写入文件 - 用 C 运行库 open/write/close 代替 _lwrite/_lclose */
    GetSaveFileName(filePath, foundSlot);
    fd = open(filePath, O_WRONLY | O_CREAT | O_BINARY | O_TRUNC, S_IREAD | S_IWRITE);

    if (fd < 0)
        return FALSE;

    write(fd, (char *)&data, sizeof(SaveData));
    close(fd);

    return TRUE;
}

/* --------------------------------------------------------------------
 * LoadGame - 载入游戏
 * -------------------------------------------------------------------- */
BOOL LoadGame(slot)
int slot;
{
    SaveData data;
    char filePath[32];
    int fd;
    int row, col;

    GetSaveFileName(filePath, slot);
    fd = open(filePath, O_RDONLY | O_BINARY);

    if (fd < 0)
        return FALSE;

    if (read(fd, (char *)&data, sizeof(SaveData)) != sizeof(SaveData)) {
        close(fd);
        return FALSE;
    }

    close(fd);

    /* 校验魔数 */
    if (data.magic[0] != 'T' || data.magic[1] != 'E' ||
        data.magic[2] != 'O' || data.magic[3] != '1')
        return FALSE;

    /* 恢复游戏状态 */
    gameState.mode = data.mode;
    for (row = 0; row < BOARD_SIZE; row++) {
        for (col = 0; col < BOARD_SIZE; col++) {
            gameState.board[row][col] = data.board[row][col];
        }
    }
    gameState.currentPlayer = data.currentPlayer;
    gameState.whiteCount = data.whiteCount;
    gameState.blackCount = data.blackCount;
    gameState.firstPlayer = data.firstPlayer;
    gameState.firstSetting = data.firstSetting;
    gameState.twoPlayer = data.twoPlayer;
    gameState.egaMode = data.egaMode;
    gameState.gameOver = data.gameOver;
    gameState.selRow = -1;
    gameState.selCol = -1;
    gameState.lastMoveRow = -1;
    gameState.lastMoveCol = -1;

    /* 重置历史记录 */
    historyCount = 0;
    historyCurrent = 0;
    SaveHistoryEntry();

    /* 更新背景画刷 */
    if (hbrBackground != NULL)
        DeleteObject(hbrBackground);
    if (gameState.egaMode == 0)
        hbrBackground = CreateSolidBrush(RGB(0, 128, 0));
    else if (gameState.egaMode == 1)
        hbrBackground = CreateSolidBrush(RGB(0, 64, 0));
    else
        hbrBackground = CreateSolidBrush(RGB(0, 255, 0));

    /* 如果载入后轮到 AI，设置定时器 */
    if (!gameState.gameOver && gameState.currentPlayer == BLACK) {
        SetTimer(hWndMain, IDT_AI_MOVE, 500, NULL);
    }

    return TRUE;
}

/* --------------------------------------------------------------------
 * GetSaveList - 获取存档列表
 * -------------------------------------------------------------------- */
int GetSaveList(names, slots)
char names[][SAVE_NAME_LEN];
int slots[];
{
    int slot;
    int count = 0;
    char filePath[32];
    int fd;
    SaveData data;

    for (slot = 1; slot <= MAX_SAVES; slot++) {
        GetSaveFileName(filePath, slot);
        fd = open(filePath, O_RDONLY | O_BINARY);

        if (fd < 0)
            continue;

        if (read(fd, (char *)&data, sizeof(SaveData)) == sizeof(SaveData)) {
            if (data.magic[0] == 'T' && data.magic[1] == 'E' &&
                data.magic[2] == 'O' && data.magic[3] == '1') {
                strcpy(names[count], data.saveName);
                slots[count] = slot;
                count++;
            }
        }
        close(fd);
    }

    return count;
}

/* --------------------------------------------------------------------
 * LoadLatestSave - 载入最新存档
 * -------------------------------------------------------------------- */
BOOL LoadLatestSave()
{
    static char names[MAX_SAVES][SAVE_NAME_LEN];
    static int slots[MAX_SAVES];
    int count;
    int i;
    int bestSlot;
    long bestTime;
    char filePath[32];
    int fd;
    SaveData data;

    count = GetSaveList(names, slots);

    if (count == 0)
        return FALSE;

    bestSlot = slots[0];
    bestTime = 0;

    for (i = 0; i < count; i++) {
        GetSaveFileName(filePath, slots[i]);
        fd = open(filePath, O_RDONLY | O_BINARY);
        if (fd < 0)
            continue;

        if (read(fd, (char *)&data, sizeof(SaveData)) == sizeof(SaveData)) {
            if (data.timestamp > bestTime) {
                bestTime = data.timestamp;
                bestSlot = slots[i];
            }
        }
        close(fd);
    }

    return LoadGame(bestSlot);
}

/* --------------------------------------------------------------------
 * DeleteSave - 删除存档
 *
 * 删除指定槽位的存档文件。
 * 使用 C 运行库 unlink 函数（相当于 DOS 的 delete）。
 * -------------------------------------------------------------------- */
BOOL DeleteSave(slot)
int slot;
{
    char filePath[32];

    GetSaveFileName(filePath, slot);
    if (unlink(filePath) == 0)
        return TRUE;
    return FALSE;
}
