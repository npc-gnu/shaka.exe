#include <windows.h>
#include <shellapi.h>
#include <fstream>
#include <tchar.h>

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "shell32.lib")

HHOOK hKeyboardHook, hMouseHook;

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    return 1;
}
LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    return 1;
}

void InstallHooks() {
    hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, NULL, 0);
    hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, NULL, 0);
}

void RemoveHooks() {
    UnhookWindowsHookEx(hKeyboardHook);
    UnhookWindowsHookEx(hMouseHook);
}

void HideTaskbar() {
    HWND taskbar = FindWindow("Shell_TrayWnd", NULL);
    if (taskbar) {
        ShowWindow(taskbar, SW_HIDE);
        EnableWindow(taskbar, FALSE);
    }
}
void ShowTaskbar() {
    HWND taskbar = FindWindow("Shell_TrayWnd", NULL);
    if (taskbar) {
        EnableWindow(taskbar, TRUE);
        ShowWindow(taskbar, SW_SHOW);
    }
}

// Yöneticilik kontrolü
bool IsRunAsAdmin()
{
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;

    if (AllocateAndInitializeSid(&NtAuthority, 2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0,
        &adminGroup)) {
        CheckTokenMembership(NULL, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }

    return isAdmin;
}

// Kendini yönetici olarak yeniden başlat
bool RelaunchAsAdmin()
{
    TCHAR szPath[MAX_PATH];
    if (!GetModuleFileName(NULL, szPath, ARRAYSIZE(szPath)))
        return false;

    SHELLEXECUTEINFO sei = { sizeof(sei) };
    sei.lpVerb = _T("runas");
    sei.lpFile = szPath;
    sei.nShow = SW_NORMAL;

    if (!ShellExecuteEx(&sei))
        return false;

    return true;
}

// Gömülü videoyu çıkart
std::string ExtractVideoResource() {
    HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(101), "VIDEO");
    HGLOBAL hData = LoadResource(NULL, hRes);
    DWORD size = SizeofResource(NULL, hRes);
    void* pData = LockResource(hData);

    char tempPath[MAX_PATH];
    GetTempPathA(MAX_PATH, tempPath);
    std::string fullPath = std::string(tempPath) + "shaka_temp.mp4";

    std::ofstream out(fullPath, std::ios::binary);
    out.write(reinterpret_cast<const char*>(pData), size);
    out.close();

    return fullPath;
}

// MCI ile video oynat
void PlayVideo(const std::string& path) {
    std::string cmd = "open \"" + path + "\" type mpegvideo alias vid";
    mciSendStringA(cmd.c_str(), NULL, 0, NULL);
    mciSendStringA("play vid fullscreen", NULL, 0, NULL);

    char status[128];
    do {
        mciSendStringA("status vid mode", status, sizeof(status), NULL);
        Sleep(500);
    } while (strcmp(status, "stopped") != 0);

    mciSendStringA("close vid", NULL, 0, NULL);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    if (!IsRunAsAdmin()) {
        if (!RelaunchAsAdmin()) {
            MessageBoxA(NULL, "Bu uygulama yönetici olarak çalıştırılmalıdır.", "Yönetici Gerekiyor", MB_OK | MB_ICONERROR);
        }
        return 0;
    }

    HWND hwnd = CreateWindowEx(WS_EX_TOPMOST,
                               "STATIC", "Şaka", WS_POPUP | WS_VISIBLE,
                               0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
                               NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    InstallHooks();
    HideTaskbar();

    std::string videoPath = ExtractVideoResource();
    PlayVideo(videoPath);
    DeleteFileA(videoPath.c_str());

    RemoveHooks();
    ShowTaskbar();

    MessageBoxA(NULL, "Bu bir şakaydı. Eğerki daha şakasız veya virüssüz bir ortam isterseniz, Linux kullanabilirsiniz!", "Şaka", MB_OK | MB_ICONINFORMATION);
    return 0;
}

