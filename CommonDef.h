#pragma once

#define TXT_MESSAGEBOX_TITLE                           L"Virtual Desktop"
#define TXT_CONFIRM_MENU_ITEM                          L"&Confirm \"Desktop Switch\""
#define TXT_MANAGE_DESKTOP_MENU_ITEM                   L"&Manage Desktops"
#define TXT_ABOUT_MENU_ITEM                            L"&About Virtual Desktop..."
#define TXT_EXIT_MENU_ITEM                             L"&Exit"
#define REG_KEY_COMMON_SETTINGS                        L"CommonSettings"
#define REG_SUB_KEY_CONFIRM_SWITCH                     L"ConfirmSwitch"

// GlobalFunctions.cpp
void DebugPrintErrorMessage(const wchar_t* message, DWORD lastError);
