/* ====================================================================
 * dialogs.c - 对话框模块
 * 
 * 包含所有对话框过程函数：
 *   - AboutDlgProc  : 关于（模态对话框）
 *   - HelpDlgProc   : 帮助（模态对话框）
 *   - SettingsDlgProc: 设置（无模式悬浮窗口）
 *   - SaveDlgProc   : 存档（无模式悬浮窗口）
 *   - LoadDlgProc   : 载入（无模式悬浮窗口）
 * 
 * 模态对话框使用 DialogBox/EndDialog。
 * 无模式对话框使用 CreateDialog/DestroyWindow。
 * ==================================================================== */

#include "twoeat.h"
#include <string.h>
#include <stdio.h>

/* --------------------------------------------------------------------
 * AboutDlgProc - 关于对话框过程（模态）
 * 
 * 显示游戏名称、版本号、作者信息。
 * 使用 DS_MODALFRAME 风格，接近 Win 1.03 原生外观。
 * -------------------------------------------------------------------- */
BOOL FAR PASCAL AboutDlgProc(hDlg, message, wParam, lParam)
HWND hDlg;
unsigned message;
WORD wParam;
LONG lParam;
{
    switch (message) {

    case WM_INITDIALOG:
        /* 对话框初始化，无需特殊处理 */
        return TRUE;

    case WM_COMMAND:
        /* IDOK 或 IDCANCEL 都关闭对话框 */
        if (wParam == IDOK || wParam == IDCANCEL) {
            EndDialog(hDlg, TRUE);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

/* --------------------------------------------------------------------
 * HelpDlgProc - 帮助对话框过程（模态）
 * 
 * 向玩家简要介绍游戏玩法。
 * -------------------------------------------------------------------- */
BOOL FAR PASCAL HelpDlgProc(hDlg, message, wParam, lParam)
HWND hDlg;
unsigned message;
WORD wParam;
LONG lParam;
{
    switch (message) {

    case WM_INITDIALOG:
        return TRUE;

    case WM_COMMAND:
        if (wParam == IDOK || wParam == IDCANCEL) {
            EndDialog(hDlg, TRUE);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

/* --------------------------------------------------------------------
 * SettingsDlgProc - 设置对话框过程（无模式）
 * 
 * 设置先后手（默认/先手/后手）和颜色模式（EGA/VGA）。
 * 使用无模式对话框，玩家可在游戏中随时调整设置。
 * 
 * 对话框初始化时根据 gameState 当前值设置单选按钮选中状态。
 * 确认时读取单选按钮状态并更新 gameState。
 * -------------------------------------------------------------------- */
BOOL FAR PASCAL SettingsDlgProc(hDlg, message, wParam, lParam)
HWND hDlg;
unsigned message;
WORD wParam;
LONG lParam;
{
    switch (message) {

    case WM_INITDIALOG:
    {
        /* ---- 初始化先后手单选按钮 ---- */
        CheckRadioButton(hDlg, IDC_SET_DEFAULT, IDC_SET_SECOND,
                         IDC_SET_DEFAULT + gameState.firstSetting);

        /* ---- 初始化颜色模式单选按钮 ---- */
        /* 颜色模式单选按钮 */
        if (gameState.egaMode == 0)
            CheckRadioButton(hDlg, IDC_SET_EGA, IDC_SET_BRIGHT, IDC_SET_EGA);
        else if (gameState.egaMode == 1)
            CheckRadioButton(hDlg, IDC_SET_EGA, IDC_SET_BRIGHT, IDC_SET_VGA);
        else
            CheckRadioButton(hDlg, IDC_SET_EGA, IDC_SET_BRIGHT, IDC_SET_BRIGHT);

        return TRUE;
    }

    case WM_COMMAND:
    {
        switch (wParam) {

        /* ---- 单选按钮点击处理 ----
         * Win 1.03 的 RADIOBUTTON（BS_RADIOBUTTON）不会自动切换选中状态，
         * 必须在 BN_CLICKED 时手动调用 CheckRadioButton 更新整组 */
        case IDC_SET_DEFAULT:
            CheckRadioButton(hDlg, IDC_SET_DEFAULT, IDC_SET_SECOND, IDC_SET_DEFAULT);
            return TRUE;

        case IDC_SET_FIRST:
            CheckRadioButton(hDlg, IDC_SET_DEFAULT, IDC_SET_SECOND, IDC_SET_FIRST);
            return TRUE;

        case IDC_SET_SECOND:
            CheckRadioButton(hDlg, IDC_SET_DEFAULT, IDC_SET_SECOND, IDC_SET_SECOND);
            return TRUE;

        case IDC_SET_EGA:
            CheckRadioButton(hDlg, IDC_SET_EGA, IDC_SET_BRIGHT, IDC_SET_EGA);
            return TRUE;

        case IDC_SET_VGA:
            CheckRadioButton(hDlg, IDC_SET_EGA, IDC_SET_BRIGHT, IDC_SET_VGA);
            return TRUE;

        case IDC_SET_BRIGHT:
            CheckRadioButton(hDlg, IDC_SET_EGA, IDC_SET_BRIGHT, IDC_SET_BRIGHT);
            return TRUE;

        case IDOK:
        {
            /* ---- 读取先后手设置 ---- */
            if (IsDlgButtonChecked(hDlg, IDC_SET_DEFAULT))
                gameState.firstSetting = FIRST_DEFAULT;
            else if (IsDlgButtonChecked(hDlg, IDC_SET_FIRST))
                gameState.firstSetting = FIRST_ALWAYS;
            else
                gameState.firstSetting = SECOND_ALWAYS;

            /* ---- 读取颜色模式设置 ---- */
            if (IsDlgButtonChecked(hDlg, IDC_SET_EGA))
                gameState.egaMode = 0;
            else if (IsDlgButtonChecked(hDlg, IDC_SET_VGA))
                gameState.egaMode = 1;
            else
                gameState.egaMode = 2;

            /* 更新背景画刷 */
            if (hbrBackground != NULL)
                DeleteObject(hbrBackground);
            if (gameState.egaMode == 0)
                hbrBackground = CreateSolidBrush(RGB(0, 128, 0));
            else if (gameState.egaMode == 1)
                hbrBackground = CreateSolidBrush(RGB(0, 64, 0));
            else
                hbrBackground = CreateSolidBrush(RGB(0, 255, 0));

            /* 重绘主窗口 */
            InvalidateRect(hWndMain, NULL, FALSE);

            /* 关闭无模式对话框 */
            DestroyWindow(hDlg);
            hModelessDlg = NULL;
            return TRUE;
        }

        case IDCANCEL:
            DestroyWindow(hDlg);
            hModelessDlg = NULL;
            return TRUE;
        }
        break;
    }

    case WM_CLOSE:
        DestroyWindow(hDlg);
        return TRUE;

    case WM_DESTROY:
        /* 窗口销毁时确保 hModelessDlg 被清零
         * 这是窗口生命周期中保证收到的最后一条消息，
         * 比在 IDOK/IDCANCEL 里清零更可靠
         *
         * 不在此处 FreeProcInstance！WM_DESTROY 是在 DestroyWindow()
         * 内同步发送的，此时对话框过程仍在调用栈上（通过 thunk 调用）。
         * 释放 thunk 后 return 会跳到已释放内存，导致内存损坏/卡死。
         * SDK 文档说 MakeProcInstance 对同一函数+实例返回相同地址，
         * 所以 thunk 会自动复用，不需要释放。 */
        hModelessDlg = NULL;
        break;
    }
    return FALSE;
}

/* --------------------------------------------------------------------
 * SaveDlgProc - 存档对话框过程（无模式）
 * 
 * 玩家输入存档名称，点击确认后保存游戏。
 * 存档列表显示已有存档供参考。
 * -------------------------------------------------------------------- */
BOOL FAR PASCAL SaveDlgProc(hDlg, message, wParam, lParam)
HWND hDlg;
unsigned message;
WORD wParam;
LONG lParam;
{
    /* 使用 static 避免栈溢出（数组较大） */
    static char saveNames[MAX_SAVES][SAVE_NAME_LEN];
    static int saveSlots[MAX_SAVES];
    static int saveCount;

    switch (message) {

    case WM_INITDIALOG:
    {
        int i;
        int defaultNum;
        char defaultName[16];

        /* 填充已有存档列表 */
        saveCount = GetSaveList(saveNames, saveSlots);

        for (i = 0; i < saveCount; i++) {
            SendDlgItemMessage(hDlg, IDC_SAVE_LIST, LB_ADDSTRING, 0,
                             (LONG)(LPSTR)saveNames[i]);
        }

        /* 默认存档名：Save + 下一个可用编号
         * 例如已有 3 个存档，默认名就是 "Save4" */
        defaultNum = saveCount + 1;
        sprintf(defaultName, "Save%d", defaultNum);
        SetDlgItemText(hDlg, IDC_SAVE_EDIT, defaultName);
        return TRUE;
    }

    case WM_COMMAND:
    {
        switch (wParam) {

        case IDOK:
        {
            char saveName[SAVE_NAME_LEN];

            /* 获取存档名称 */
            GetDlgItemText(hDlg, IDC_SAVE_EDIT, saveName, SAVE_NAME_LEN);

            /* 检查名称非空 */
            if (saveName[0] == '\0') {
                MessageBeep(0);
                return TRUE;
            }

            /* 执行保存——成功不弹 MessageBox（避免无模式对话框内嵌套消息循环） */
            if (!SaveGame(saveName)) {
                MessageBox(hDlg, "Save failed!", "Error", MB_OK | MB_ICONEXCLAMATION);
            }

            DestroyWindow(hDlg);
            hModelessDlg = NULL;
            return TRUE;
        }

        case IDC_SAVE_DELETE:
        {
            int sel;
            int i;
            sel = SendDlgItemMessage(hDlg, IDC_SAVE_LIST, LB_GETCURSEL, 0, 0L);
            if (sel < 0) {
                MessageBeep(0);
                return TRUE;
            }
            /* 确认删除——防止误操作 */
            if (MessageBox(hDlg, "Delete this save?", "Confirm",
                           MB_YESNO | MB_ICONEXCLAMATION) != IDYES) {
                return TRUE;
            }
            /* 删除存档文件 */
            DeleteSave(saveSlots[sel]);
            /* 从列表中移除 */
            SendDlgItemMessage(hDlg, IDC_SAVE_LIST, LB_DELETESTRING, sel, 0L);
            /* 更新内部数组：后面的前移 */
            for (i = sel; i < saveCount - 1; i++) {
                strcpy(saveNames[i], saveNames[i + 1]);
                saveSlots[i] = saveSlots[i + 1];
            }
            saveCount--;
            return TRUE;
        }

        case IDCANCEL:
            DestroyWindow(hDlg);
            hModelessDlg = NULL;
            return TRUE;

        case IDC_SAVE_LIST:
        {
            /* 双击列表项时填充名称 */
            if (HIWORD(lParam) == LBN_DBLCLK) {
                int sel;
                char name[SAVE_NAME_LEN];
                sel = SendDlgItemMessage(hDlg, IDC_SAVE_LIST, LB_GETCURSEL, 0, 0L);
                if (sel >= 0) {
                    SendDlgItemMessage(hDlg, IDC_SAVE_LIST, LB_GETTEXT,
                                       sel, (LONG)(LPSTR)name);
                    SetDlgItemText(hDlg, IDC_SAVE_EDIT, name);
                }
            }
            return TRUE;
        }
        }
        break;
    }

    case WM_CLOSE:
        DestroyWindow(hDlg);
        return TRUE;

    case WM_DESTROY:
        hModelessDlg = NULL;
        /* 不释放 thunk——见 SettingsDlgProc WM_DESTROY 注释 */
        break;
    }
    return FALSE;
}

/* --------------------------------------------------------------------
 * LoadDlgProc - 载入对话框过程（无模式）
 * 
 * 显示存档列表，玩家选择后载入。
 * 如果没有存档，提示"无存档"。
 * -------------------------------------------------------------------- */
BOOL FAR PASCAL LoadDlgProc(hDlg, message, wParam, lParam)
HWND hDlg;
unsigned message;
WORD wParam;
LONG lParam;
{
    static char saveNames[MAX_SAVES][SAVE_NAME_LEN];
    static int saveSlots[MAX_SAVES];
    static int saveCount;

    switch (message) {

    case WM_INITDIALOG:
    {
        int i;

        saveCount = GetSaveList(saveNames, saveSlots);

        if (saveCount == 0) {
            /* 无存档时禁用 OK 按钮，不弹 MessageBox
             * （WM_INITDIALOG 期间对话框尚未完全初始化，调用 MessageBox
             *   会进入嵌套消息循环，可能导致问题） */
            EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
            return TRUE;
        }

        /* 填充列表 */
        for (i = 0; i < saveCount; i++) {
            SendDlgItemMessage(hDlg, IDC_LOAD_LIST, LB_ADDSTRING, 0,
                             (LONG)(LPSTR)saveNames[i]);
        }

        /* 默认选中第一项 */
        SendDlgItemMessage(hDlg, IDC_LOAD_LIST, LB_SETCURSEL, 0, 0L);
        return TRUE;
    }

    case WM_COMMAND:
    {
        switch (wParam) {

        case IDOK:
        {
            int sel;

            sel = SendDlgItemMessage(hDlg, IDC_LOAD_LIST, LB_GETCURSEL, 0, 0L);

            if (sel < 0) {
                MessageBeep(0);
                return TRUE;
            }

            /* 载入选中的存档 */
            if (LoadGame(saveSlots[sel])) {
                HMENU hMenu;

                /* 更新菜单对勾以反映载入的对弈模式 */
                hMenu = GetMenu(hWndMain);
                if (gameState.twoPlayer) {
                    CheckMenuItem(hMenu, IDM_AI_MODE, MF_UNCHECKED);
                    CheckMenuItem(hMenu, IDM_TWO_PLAYER, MF_CHECKED);
                } else {
                    CheckMenuItem(hMenu, IDM_AI_MODE, MF_CHECKED);
                    CheckMenuItem(hMenu, IDM_TWO_PLAYER, MF_UNCHECKED);
                }

                /* 如果 AI 模式下轮到黑棋，启动 AI 定时器 */
                if (gameState.currentPlayer == BLACK && !gameState.gameOver && !gameState.twoPlayer) {
                    SetTimer(hWndMain, IDT_AI_MOVE, 500, NULL);
                }

                InvalidateRect(hWndMain, NULL, FALSE);
                /* 载入成功不弹 MessageBox——棋盘已重绘，玩家直接看到效果 */
            } else {
                MessageBox(hDlg, "Load failed!", "Error", MB_OK | MB_ICONEXCLAMATION);
            }

            DestroyWindow(hDlg);
            hModelessDlg = NULL;
            return TRUE;
        }

        case IDCANCEL:
            DestroyWindow(hDlg);
            hModelessDlg = NULL;
            return TRUE;

        case IDC_LOAD_DELETE:
        {
            int sel;
            int i;
            sel = SendDlgItemMessage(hDlg, IDC_LOAD_LIST, LB_GETCURSEL, 0, 0L);
            if (sel < 0) {
                MessageBeep(0);
                return TRUE;
            }
            /* 确认删除——防止误操作 */
            if (MessageBox(hDlg, "Delete this save?", "Confirm",
                           MB_YESNO | MB_ICONEXCLAMATION) != IDYES) {
                return TRUE;
            }
            /* 删除存档文件 */
            DeleteSave(saveSlots[sel]);
            /* 从列表中移除 */
            SendDlgItemMessage(hDlg, IDC_LOAD_LIST, LB_DELETESTRING, sel, 0L);
            /* 更新内部数组 */
            for (i = sel; i < saveCount - 1; i++) {
                strcpy(saveNames[i], saveNames[i + 1]);
                saveSlots[i] = saveSlots[i + 1];
            }
            saveCount--;
            /* 如果删完了，禁用 OK 按钮 */
            if (saveCount == 0) {
                EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
            }
            return TRUE;
        }

        case IDC_LOAD_LIST:
            /* 双击列表项直接载入 */
            if (HIWORD(lParam) == LBN_DBLCLK) {
                int sel;
                sel = SendDlgItemMessage(hDlg, IDC_LOAD_LIST, LB_GETCURSEL, 0, 0L);
                if (sel >= 0) {
                    if (LoadGame(saveSlots[sel])) {
                        HMENU hMenu;

                        hMenu = GetMenu(hWndMain);
                        if (gameState.twoPlayer) {
                            CheckMenuItem(hMenu, IDM_AI_MODE, MF_UNCHECKED);
                            CheckMenuItem(hMenu, IDM_TWO_PLAYER, MF_CHECKED);
                        } else {
                            CheckMenuItem(hMenu, IDM_AI_MODE, MF_CHECKED);
                            CheckMenuItem(hMenu, IDM_TWO_PLAYER, MF_UNCHECKED);
                        }

                        if (gameState.currentPlayer == BLACK && !gameState.gameOver && !gameState.twoPlayer) {
                            SetTimer(hWndMain, IDT_AI_MOVE, 500, NULL);
                        }

                        InvalidateRect(hWndMain, NULL, FALSE);
                    }
                    DestroyWindow(hDlg);
                    hModelessDlg = NULL;
                }
                return TRUE;
            }
            break;
        }
        break;
    }

    case WM_CLOSE:
        DestroyWindow(hDlg);
        return TRUE;

    case WM_DESTROY:
        hModelessDlg = NULL;
        /* 不释放 thunk——见 SettingsDlgProc WM_DESTROY 注释 */
        break;
    }
    return FALSE;
}
