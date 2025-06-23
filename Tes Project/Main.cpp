#include <Windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <cstdio>
#include <cstdarg>
#include <string>
#include <fstream>
#include <MinHook.h>
#include <ctime>
#include <cmath>

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "libMinHook.x86.lib")

// Logging to console and debugger
void Log(const char* fmt, ...) {
    char buf[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    OutputDebugStringA("[D3DMenu] ");
    OutputDebugStringA(buf);
    OutputDebugStringA("\n");

    printf("[D3DMenu] %s\n", buf);
    fflush(stdout);
}

// Predefined colors
struct Color {
    const char* name;
    DWORD value;
};

Color predefinedColors[] = {
    {"White", D3DCOLOR_ARGB(255, 255, 255, 255)},
    {"Red", D3DCOLOR_ARGB(255, 255, 0, 0)},
    {"Green", D3DCOLOR_ARGB(255, 0, 255, 0)},
    {"Blue", D3DCOLOR_ARGB(255, 0, 0, 255)},
    {"Yellow", D3DCOLOR_ARGB(255, 255, 255, 0)},
    {"Cyan", D3DCOLOR_ARGB(255, 0, 255, 255)},
    {"Magenta", D3DCOLOR_ARGB(255, 255, 0, 255)},
    {"Orange", D3DCOLOR_ARGB(255, 255, 165, 0)},
    {"Purple", D3DCOLOR_ARGB(255, 128, 0, 128)},
    {"Pink", D3DCOLOR_ARGB(255, 255, 192, 203)},
    {"RGB", D3DCOLOR_ARGB(255, 255, 255, 255)} // RGB option
};
const int colorCount = sizeof(predefinedColors) / sizeof(predefinedColors[0]);

struct MenuItem {
    const char* title = "";
    int* hack = nullptr;
    int maxval = 1;
    int type = 0;
    DWORD color = D3DCOLOR_ARGB(255, 255, 255, 255);
};

class D3DMenu {
public:
    LPD3DXFONT pFont = nullptr;
    LPD3DXSPRITE pSprite = nullptr;
    LPD3DXLINE pLine = nullptr;
    int itemCount = 0;
    int selector = 0;
    int x = 0, y = 0, w = 0, h = 0;
    int baseW = 250, baseH = 300;
    float scaleFactor = 1.0f;
    DWORD titleColor = D3DCOLOR_ARGB(255, 255, 255, 255);
    DWORD backColor = D3DCOLOR_ARGB(230, 25, 25, 35);
    DWORD borderColor = D3DCOLOR_ARGB(255, 100, 150, 255);
    DWORD shadowColor = D3DCOLOR_ARGB(180, 0, 0, 0);
    MenuItem items[50];
    bool show = true;
    float animProgress = 0.0f;
    float rgbAnimProgress = 0.0f;
    int menuColorIndex = 3;
    int borderColorIndex = 3;
    int crosshairColorIndex = 1;
    int boxStyle = 1;
    int crosshairType = 0;
    int crosshairSize = 5;
    int espNameTag = 0;
    int espNameTagLineColorIndex = 1;
    ULONGLONG lastKeyTime = 0;
    const ULONGLONG keyDelay = 150;

    DWORD GetRGBColor(float progress) {
        float r = sinf(progress * 2.0f * D3DX_PI) * 127.5f + 127.5f;
        float g = sinf(progress * 2.0f * D3DX_PI + 2.0f * 3.14159f / 3.0f) * 127.5f + 127.5f;
        float b = sinf(progress * 2.0f * D3DX_PI + 4.0f * 3.14159f / 3.0f) * 127.5f + 127.5f;
        return D3DCOLOR_ARGB(255, (BYTE)r, (BYTE)g, (BYTE)b);
    }

    void CalculateDimensions(LPDIRECT3DDEVICE9 device) {
        D3DVIEWPORT9 vp;
        device->GetViewport(&vp);
        float screenW = (float)vp.Width;
        float screenH = (float)vp.Height;

        scaleFactor = min(screenW / 1920.0f, screenH / 1080.0f);
        w = (int)(baseW * scaleFactor);
        h = (int)((60 + itemCount * 22 + 30) * scaleFactor);
        x = (int)(50 * scaleFactor);
        y = (int)(50 * scaleFactor);
    }

    void CreateItem(const char* title, int* hack, int maxval = 1, int type = 0, DWORD color = D3DCOLOR_ARGB(255, 255, 255, 255)) {
        if (itemCount < 50) {
            items[itemCount++] = { title, hack, maxval, type, color };
            Log("Menu item created: %s", title);
        }
    }

    void InitFont(LPDIRECT3DDEVICE9 device) {
        if (!pFont && device) {
            int fontSize = (int)(16 * scaleFactor);
            if (SUCCEEDED(D3DXCreateFontA(device, fontSize, 0, FW_BOLD, 1, FALSE, DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                "Verdana", &pFont))) {
                Log("Font initialized.");
            }
            else {
                Log("Font initialization failed.");
            }
        }
        if (!pSprite && device) {
            if (SUCCEEDED(D3DXCreateSprite(device, &pSprite))) {
                Log("Sprite initialized.");
            }
            else {
                Log("Sprite initialization failed.");
            }
        }
        if (!pLine && device) {
            if (SUCCEEDED(D3DXCreateLine(device, &pLine))) {
                Log("Line initialized.");
                pLine->SetWidth(2.0f * scaleFactor);
                pLine->SetAntialias(TRUE);
            }
            else {
                Log("Line initialization failed.");
            }
        }
    }

    void DrawText(LPDIRECT3DDEVICE9 device, const char* text, int x, int y, DWORD color, bool shadow = false, float textAnimProgress = 1.0f) {
        if (!pFont) InitFont(device);
        if (pFont && pSprite) {
            BYTE alpha = (BYTE)(255 * (0.7f + 0.3f * sinf(textAnimProgress * 2.0f * D3DX_PI)));
            DWORD animColor = D3DCOLOR_ARGB(alpha, (BYTE)GetRValue(color), (BYTE)GetGValue(color), (BYTE)GetBValue(color));

            RECT rect = { x, y, x + w, y + (int)(20 * scaleFactor) };
            if (shadow) {
                RECT shadowRect = { x + (int)(2 * scaleFactor), y + (int)(2 * scaleFactor), x + w + (int)(2 * scaleFactor), y + (int)(22 * scaleFactor) };
                pFont->DrawTextA(pSprite, text, -1, &shadowRect, DT_LEFT | DT_NOCLIP, shadowColor);
            }
            pFont->DrawTextA(pSprite, text, -1, &rect, DT_LEFT | DT_NOCLIP, animColor);
        }
    }

    void DrawBox(LPDIRECT3DDEVICE9 device, int x, int y, int w, int h, DWORD fill, DWORD border) {
        if (boxStyle == 1) {
            D3DRECT mainRect = { x + (int)(4 * scaleFactor), y + (int)(4 * scaleFactor), x + w - (int)(4 * scaleFactor), y + h - (int)(4 * scaleFactor) };
            device->Clear(1, &mainRect, D3DCLEAR_TARGET, fill, 0, 0);

            D3DRECT gradientRect = { x + (int)(4 * scaleFactor), y + (int)(4 * scaleFactor), x + w - (int)(4 * scaleFactor), y + h / 2 };
            device->Clear(1, &gradientRect, D3DCLEAR_TARGET, D3DCOLOR_ARGB(230, 35, 35, 50), 0, 0);
        }

        int borderWidth = (int)(2 * scaleFactor);
        DWORD animatedBorderColor = (borderColorIndex == colorCount - 1) ? GetRGBColor(rgbAnimProgress) : predefinedColors[borderColorIndex].value;
        D3DRECT borders[] = {
            { x, y, x + w, y + borderWidth },
            { x, y + h - borderWidth, x + w, y + h },
            { x, y + borderWidth, x + borderWidth, y + h - borderWidth },
            { x + w - borderWidth, y + borderWidth, x + w, y + h - borderWidth }
        };
        for (auto& r : borders)
            device->Clear(1, &r, D3DCLEAR_TARGET, animatedBorderColor, 0, 0);

        D3DRECT shadow[] = {
            { x + (int)(2 * scaleFactor), y + h, x + w + (int)(2 * scaleFactor), y + h + (int)(4 * scaleFactor) },
            { x + w, y + (int)(2 * scaleFactor), x + w + (int)(4 * scaleFactor), y + h + (int)(2 * scaleFactor) }
        };
        for (auto& r : shadow)
            device->Clear(1, &r, D3DCLEAR_TARGET, shadowColor, 0, 0);
    }

    void DrawSelectionBox(LPDIRECT3DDEVICE9 device, int x, int y, int w, int h, float progress) {
        DWORD selColor = predefinedColors[menuColorIndex].value;
        DWORD selFade = D3DCOLOR_ARGB((int)(255 * (1.0f - progress)),
            (BYTE)(GetRValue(selColor) / 2),
            (BYTE)(GetGValue(selColor) / 2),
            (BYTE)(GetBValue(selColor) / 2));
        D3DRECT selRect = { x, y, x + w, y + h };
        device->Clear(1, &selRect, D3DCLEAR_TARGET, selFade, 0, 0);
        D3DRECT selBorder = { x, y, x + w, y + (int)(1 * scaleFactor) };
        device->Clear(1, &selBorder, D3DCLEAR_TARGET, selColor, 0, 0);
    }

    void DrawCrosshair(LPDIRECT3DDEVICE9 device) {
        if (crosshairType == 0) return;

        D3DVIEWPORT9 vp;
        device->GetViewport(&vp);
        float centerX = vp.Width / 2.0f;
        float centerY = vp.Height / 2.0f;
        DWORD crosshairColor = (crosshairColorIndex == colorCount - 1) ? GetRGBColor(rgbAnimProgress) : predefinedColors[crosshairColorIndex].value;

        if (pLine) {
            pLine->Begin();
            if (crosshairType == 1) {
                D3DXVECTOR2 points[2] = { {centerX, centerY}, {centerX + 1, centerY} };
                pLine->SetWidth((float)crosshairSize * scaleFactor);
                pLine->Draw(points, 2, crosshairColor);
            }
            else if (crosshairType == 2) {
                D3DXVECTOR2 pointsH[2] = { {centerX - crosshairSize * scaleFactor, centerY}, {centerX + crosshairSize * scaleFactor, centerY} };
                D3DXVECTOR2 pointsV[2] = { {centerX, centerY - crosshairSize * scaleFactor}, {centerX, centerY + crosshairSize * scaleFactor} };
                pLine->SetWidth(2.0f * scaleFactor);
                pLine->Draw(pointsH, 2, crosshairColor);
                pLine->Draw(pointsV, 2, crosshairColor);
            }
            else if (crosshairType == 3) {
                const int segments = 32;
                D3DXVECTOR2 points[33];
                float radius = (float)crosshairSize * scaleFactor;
                for (int i = 0; i <= segments; i++) {
                    float angle = (float)i / segments * 2.0f * D3DX_PI;
                    points[i] = D3DXVECTOR2(centerX + radius * cosf(angle), centerY + radius * sinf(angle));
                }
                pLine->SetWidth(2.0f * scaleFactor);
                pLine->Draw(points, segments + 1, crosshairColor);
            }
            pLine->End();
        }
    }

    void DrawESPBody(LPDIRECT3DDEVICE9 device, int espNameTag) {
        if (!pLine || !pFont || !pSprite) {
            InitFont(device);
            if (!pLine || !pFont || !pSprite) {
                Log("Failed to initialize pLine, pFont, or pSprite for ESP.");
                return;
            }
        }

        pLine->Begin();
        D3DVIEWPORT9 vp;
        device->GetViewport(&vp);
        float centerX = vp.Width / 2.0f;
        float centerY = vp.Height / 2.0f;

        // Draw ESP body box
        float boxWidth = 40.0f * scaleFactor;
        float boxHeight = 40.0f * scaleFactor;
        D3DXVECTOR2 points[5] = {
            {centerX - boxWidth / 2, centerY - boxHeight / 2},
            {centerX + boxWidth / 2, centerY - boxHeight / 2},
            {centerX + boxWidth / 2, centerY + boxHeight / 2},
            {centerX - boxWidth / 2, centerY + boxHeight / 2},
            {centerX - boxWidth / 2, centerY - boxHeight / 2}
        };
        pLine->SetWidth(2.0f * scaleFactor);
        pLine->Draw(points, 5, D3DCOLOR_ARGB(255, 255, 0, 0));

        // Draw nametag and line if espNameTag is enabled
        if (espNameTag) {
            const char* nickname = "Player"; // Placeholder; replace with actual player name
            DWORD lineColor = (espNameTagLineColorIndex == colorCount - 1) ? GetRGBColor(rgbAnimProgress) : predefinedColors[espNameTagLineColorIndex].value;

            // Calculate nickname text position
            int textHeight = (int)(20 * scaleFactor);
            int textY = (int)(centerY - boxHeight / 2 - textHeight - 5 * scaleFactor); // 5 pixels above box
            int textX = (int)(centerX - (strlen(nickname) * 8 * scaleFactor) / 2); // Center text

            // Draw nickname text
            DrawText(device, nickname, textX, textY, lineColor, true);

            // Draw line from top of box to bottom of text
            D3DXVECTOR2 linePoints[2] = {
                {centerX, centerY - boxHeight / 2}, // Top center of the box
                {centerX, (float)textY + textHeight} // Bottom of the text
            };
            pLine->SetWidth(2.0f * scaleFactor);
            pLine->Draw(linePoints, 2, lineColor);
        }

        pLine->End();
    }

    std::string GetCurrentTime() {
        time_t now = time(nullptr);
        struct tm tstruct;
        char buf[80];
        localtime_s(&tstruct, &now);
        strftime(buf, sizeof(buf), "%H:%M:%S", &tstruct);
        return std::string(buf);
    }

    void Render(LPDIRECT3DDEVICE9 device, const char* title) {
        CalculateDimensions(device);

        ULONGLONG currentTime = GetTickCount64();

        if (GetAsyncKeyState(VK_INSERT) & 0x8000) {
            if (currentTime - lastKeyTime >= keyDelay) {
                show = !show;
                Log("INSERT pressed, menu %s", show ? "shown" : "hidden");
                lastKeyTime = currentTime;
            }
        }

        if (pSprite) pSprite->Begin(D3DXSPRITE_ALPHABLEND);

        DrawCrosshair(device);
        DrawESPBody(device, espNameTag);

        if (!show) {
            if (pSprite) pSprite->End();
            return;
        }

        animProgress += 0.05f;
        if (animProgress > 1.0f) animProgress = 0.0f;
        rgbAnimProgress += 0.02f;
        if (rgbAnimProgress > 1.0f) rgbAnimProgress = 0.0f;

        DrawBox(device, x, y, w, h, backColor, borderColor);

        int titleWidth = (int)(strlen(title) * 8 * scaleFactor);
        DrawText(device, title, x + (w - titleWidth) / 2, y + (int)(10 * scaleFactor), titleColor, true);

        for (int i = 0; i < itemCount; i++) {
            int itemY = y + (int)(40 * scaleFactor) + i * (int)(22 * scaleFactor);
            std::string line = (i == selector ? "> " : "  ");
            line += items[i].title;
            if (items[i].hack) {
                if (items[i].type == 0) {
                    line += *items[i].hack ? " [ON]" : " [OFF]";
                }
                else if (items[i].title == std::string("Crosshair")) {
                    switch (*items[i].hack) {
                    case 0: line += " [OFF]"; break;
                    case 1: line += " [DOT]"; break;
                    case 2: line += " [CROSS]"; break;
                    case 3: line += " [CIRCLE]"; break;
                    }
                }
                else if (items[i].title == std::string("Menu Color") ||
                    items[i].title == std::string("Border Color") ||
                    items[i].title == std::string("Crosshair Color") ||
                    items[i].title == std::string("NameTag Line Color")) {
                    int index = *items[i].hack;
                    line += " [" + std::string(predefinedColors[index].name) + "]";
                }
                else if (items[i].title == std::string("Box Style")) {
                    line += *items[i].hack ? " [WITH BG]" : " [BORDER ONLY]";
                }
                else {
                    line += " [" + std::to_string(*items[i].hack) + "]";
                }
            }
            DWORD itemColor = (i == selector) ? D3DCOLOR_ARGB(255, 200, 220, 255) : items[i].color;
            if (i == selector) {
                DrawSelectionBox(device, x + (int)(5 * scaleFactor), itemY, w - (int)(10 * scaleFactor), (int)(20 * scaleFactor), animProgress);
                DrawText(device, line.c_str(), x + (int)(15 * scaleFactor), itemY, itemColor, true, animProgress);
            }
            else {
                DrawText(device, line.c_str(), x + (int)(15 * scaleFactor), itemY, itemColor, true);
            }
        }

        std::string timeStr = "Time: " + GetCurrentTime();
        int timeWidth = (int)(timeStr.length() * 8 * scaleFactor);
        int timeY = y + (int)(40 * scaleFactor) + itemCount * (int)(22 * scaleFactor) + (int)(10 * scaleFactor);
        DrawText(device, timeStr.c_str(), x + (w - timeWidth) / 2, timeY, D3DCOLOR_ARGB(255, 200, 200, 200), true);

        if (pSprite) pSprite->End();
    }

    void UpdateInput() {
        ULONGLONG currentTime = GetTickCount64();
        if (currentTime - lastKeyTime >= keyDelay) {
            if (GetAsyncKeyState(VK_UP) & 0x8000) {
                selector = (selector - 1 + itemCount) % itemCount;
                animProgress = 0.0f;
                lastKeyTime = currentTime;
            }
            if (GetAsyncKeyState(VK_DOWN) & 0x8000) {
                selector = (selector + 1) % itemCount;
                animProgress = 0.0f;
                lastKeyTime = currentTime;
            }
            if (items[selector].hack) {
                if (GetAsyncKeyState(VK_RIGHT) & 0x8000) {
                    if (items[selector].type == 0) *items[selector].hack = !*items[selector].hack;
                    else if (*items[selector].hack < items[selector].maxval) (*items[selector].hack)++;
                    lastKeyTime = currentTime;
                }
                if (GetAsyncKeyState(VK_LEFT) & 0x8000) {
                    if (items[selector].type == 0) *items[selector].hack = !*items[selector].hack;
                    else if (*items[selector].hack > 0) (*items[selector].hack)--;
                    lastKeyTime = currentTime;
                }
            }
        }
    }

    void Release() {
        if (pFont) {
            pFont->Release();
            pFont = nullptr;
        }
        if (pSprite) {
            pSprite->Release();
            pSprite = nullptr;
        }
        if (pLine) {
            pLine->Release();
            pLine = nullptr;
        }
    }
};

// Global variables
D3DMenu g_Menu;
int espNameTag = 0;
int espNameTagLineColorIndex = 1;
int antiKick = 0;
int menuColorIndex = 3;
int borderColorIndex = 3;
int crosshairType = 0;
int crosshairColorIndex = 1;
int crosshairSize = 5;
int boxStyle = 1;

void ToggleESPNameTag(bool enable) {
    static BYTE originalBytes[6];
    static bool isPatched = false;
    static LPVOID targetAddress = nullptr;

    if (!targetAddress) {
        HMODULE hModule = GetModuleHandleA("PointBlank.exe");
        if (hModule) {
            targetAddress = (LPVOID)((DWORD)hModule + 0x62CB10);
            Log("Target address for ESP Name Tag: %p", targetAddress);
        }
        else {
            Log("Failed to get PointBlank.exe module handle.");
            return;
        }
    }

    if (enable && !isPatched) {
        ReadProcessMemory(GetCurrentProcess(), targetAddress, originalBytes, 6, nullptr);
        BYTE patchOn[] = { 0xC6, 0x01, 0x01, 0x8D, 0x49, 0x0C };
        WriteProcessMemory(GetCurrentProcess(), targetAddress, patchOn, sizeof(patchOn), nullptr);
        isPatched = true;
        Log("ESP Name Tag enabled.");
    }
    else if (!enable && isPatched) {
        WriteProcessMemory(GetCurrentProcess(), targetAddress, originalBytes, 6, nullptr);
        isPatched = false;
        Log("ESP Name Tag disabled.");
    }
}

void ToggleAntiKick(bool enable) {
    static BYTE originalBytes[2];
    static bool isPatched = false;
    static LPVOID targetAddress = nullptr;

    if (!targetAddress) {
        HMODULE hModule = GetModuleHandleA("PointBlank.exe");
        if (hModule) {
            targetAddress = (LPVOID)((DWORD)hModule + 0x78C63C);
            Log("Target address for Anti-Kick: %p", targetAddress);
        }
        else {
            Log("Failed to get PointBlank.exe module handle for Anti-Kick.");
            return;
        }
    }

    if (enable && !isPatched) {
        ReadProcessMemory(GetCurrentProcess(), targetAddress, originalBytes, 2, nullptr);
        BYTE patchOn[] = { 0xEB, 0x15 };
        WriteProcessMemory(GetCurrentProcess(), targetAddress, patchOn, sizeof(patchOn), nullptr);
        isPatched = true;
        Log("Anti-Kick enabled.");
    }
    else if (!enable && isPatched) {
        BYTE patchOff[] = { 0x78, 0x15 };
        WriteProcessMemory(GetCurrentProcess(), targetAddress, patchOff, sizeof(patchOff), nullptr);
        isPatched = false;
        Log("Anti-Kick disabled.");
    }
}

typedef HRESULT(WINAPI* EndScene_t)(LPDIRECT3DDEVICE9);
EndScene_t oEndScene = nullptr;

HRESULT WINAPI hkEndScene(LPDIRECT3DDEVICE9 pDevice) {
    Log("hkEndScene called");
    static bool init = false;
    if (!init) {
        g_Menu.CreateItem("ESP Name Tag", &espNameTag, 1, 0, D3DCOLOR_ARGB(255, 255, 100, 100));
        g_Menu.CreateItem("NameTag Line Color", &espNameTagLineColorIndex, colorCount - 1, 1, D3DCOLOR_ARGB(255, 255, 100, 100));
        g_Menu.CreateItem("Anti-Kick", &antiKick, 1, 0, D3DCOLOR_ARGB(255, 255, 100, 100));
        g_Menu.CreateItem("Crosshair", &crosshairType, 3, 1, D3DCOLOR_ARGB(255, 255, 100, 100));
        g_Menu.CreateItem("Crosshair Color", &crosshairColorIndex, colorCount - 1, 1, D3DCOLOR_ARGB(255, 255, 100, 100));
        g_Menu.CreateItem("Crosshair Size", &crosshairSize, 20, 1, D3DCOLOR_ARGB(255, 200, 200, 200));
        g_Menu.CreateItem("Menu Color", &menuColorIndex, colorCount - 1, 1, D3DCOLOR_ARGB(255, 100, 255, 100));
        g_Menu.CreateItem("Border Color", &borderColorIndex, colorCount - 1, 1, D3DCOLOR_ARGB(255, 100, 100, 255));
        g_Menu.CreateItem("Box Style", &boxStyle, 1, 0, D3DCOLOR_ARGB(255, 100, 255, 255));
        init = true;
        g_Menu.menuColorIndex = menuColorIndex;
        g_Menu.borderColorIndex = borderColorIndex;
        g_Menu.crosshairType = crosshairType;
        g_Menu.crosshairColorIndex = crosshairColorIndex;
        g_Menu.crosshairSize = crosshairSize;
        g_Menu.boxStyle = boxStyle;
        g_Menu.espNameTagLineColorIndex = espNameTagLineColorIndex;
        Log("Menu items initialized.");
    }
    g_Menu.menuColorIndex = menuColorIndex;
    g_Menu.borderColorIndex = borderColorIndex;
    g_Menu.crosshairType = crosshairType;
    g_Menu.crosshairColorIndex = crosshairColorIndex;
    g_Menu.crosshairSize = crosshairSize;
    g_Menu.boxStyle = boxStyle;
    g_Menu.espNameTagLineColorIndex = espNameTagLineColorIndex;

    ToggleESPNameTag(espNameTag != 0);
    ToggleAntiKick(antiKick != 0);

    g_Menu.Render(pDevice, "Kenny Chan || Simple Menu V1");
    g_Menu.UpdateInput();
    return oEndScene(pDevice);
}

bool HookD3D() {
    HWND hwnd = nullptr;

    for (int i = 0; i < 300; i++) {
        hwnd = FindWindowA(nullptr, "Point Blank");
        if (hwnd) break;
        Log("Window not found, retrying...");
        Sleep(100);
    }

    if (!hwnd) {
        Log("Failed to find window with title \"Point Blank\" after waiting.");
        return false;
    }
    Log("Window found.");

    LPDIRECT3D9 pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (!pD3D) {
        Log("Direct3DCreate9 failed.");
        return false;
    }

    D3DPRESENT_PARAMETERS d3dpp = {};
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.hDeviceWindow = hwnd;

    LPDIRECT3DDEVICE9 pDevice = nullptr;
    if (FAILED(pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDevice))) {
        Log("CreateDevice failed.");
        pD3D->Release();
        return false;
    }

    void** vtable = *reinterpret_cast<void***>(pDevice);
    EndScene_t pEndScene = (EndScene_t)vtable[42];

    if (MH_Initialize() != MH_OK) {
        Log("MinHook initialization failed.");
        pDevice->Release();
        pD3D->Release();
        return false;
    }

    if (MH_CreateHook(pEndScene, &hkEndScene, (LPVOID*)&oEndScene) != MH_OK) {
        Log("Failed to create hook.");
        pDevice->Release();
        pD3D->Release();
        return false;
    }

    if (MH_EnableHook(pEndScene) != MH_OK) {
        Log("Failed to enable hook.");
        pDevice->Release();
        pD3D->Release();
        return false;
    }

    pDevice->Release();
    pD3D->Release();
    Log("Hook applied to EndScene using MinHook.");
    MessageBoxA(nullptr, "Sukses inject ke PointBlank, Happy Cheating", "Success", MB_OK | MB_ICONINFORMATION);
    return true;
}

DWORD WINAPI InitThread(LPVOID) {
    AllocConsole();
    FILE* fDummy;
    freopen_s(&fDummy, "CONOUT$", "w", stdout);
    freopen_s(&fDummy, "CONOUT$", "w", stderr);
    freopen_s(&fDummy, "CONIN$", "r", stdin);

    Log("Console allocated for logging.");

    while (!HookD3D()) {
        Log("Retrying hook...");
        Sleep(1000);
    }
    Log("Hooking done.");
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, InitThread, nullptr, 0, nullptr);
    }
    else if (reason == DLL_PROCESS_DETACH) {
        g_Menu.Release();
        ToggleESPNameTag(false);
        ToggleAntiKick(false);
        MH_DisableHook(MH_ALL_HOOKS);
        MH_Uninitialize();
        Log("DLL unloaded.");
        FreeConsole();
    }
    return TRUE;
}