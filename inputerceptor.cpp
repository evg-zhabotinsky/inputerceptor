#include <Windows.h>
#include <iostream>

using std::cerr;
using std::endl;

class WithHook {
    HHOOK hook = nullptr;
public:
    WithHook(int kind, HOOKPROC callback) {
        hook = SetWindowsHookEx(kind, callback, nullptr, 0);
        if (hook) {
            cerr << "Successfully set hook 0x" << kind << " as 0x" << hook << endl;
        } else {
            cerr << "Failed to set hook 0x" << kind << endl;
            throw 1;
        }
    }
    ~WithHook() {
        if (hook) {
            if (UnhookWindowsHookEx(hook)) {
                cerr << "Successfully unset hook 0x" << hook << endl;
            } else {
                cerr << "Failed to unset hook 0x" << hook << endl;
            }
        }
    }
};

enum InputLocked {
    IL_UNLOCKED = 0,
    IL_UNLOCK_AFTER_RELEASE,
    IL_WAIT_UNLOCK,
    IL_WAIT_RELEASE
};

InputLocked inputLocked = IL_UNLOCKED;
static constexpr size_t keyCount = 0x100;
static int8_t keys[keyCount] = { 0 };
static int pressedCount = 0;

static bool KbdHandler(int code, WPARAM wParam, LPKBDLLHOOKSTRUCT lParam) {
    if (code != HC_ACTION || (lParam->flags & LLKHF_INJECTED)) {
        return true;
    }
    //cerr << "Kbd:\t0x" << wParam << "\t0x" << lParam->vkCode << "\t0x" << lParam->scanCode << "\t0x" << lParam->flags << endl;
    //return true;
    unsigned key = lParam->vkCode;
    bool released = lParam->flags & LLKHF_UP;
    if (key >= keyCount) {
        return !inputLocked;
    }
    uint8_t wasPressed = keys[key];
    if (wasPressed) {
        pressedCount--;
    }
    keys[key] = released ? 0 : inputLocked ? -1 : 1;
    if (!released) {
        pressedCount++;
    }
    switch (inputLocked) {
    case IL_WAIT_RELEASE:
        if (!pressedCount) {
            inputLocked = IL_WAIT_UNLOCK;
            cerr << "All keys released. Wait unlock combo." << endl;
        }
        return wasPressed == 1 && released;
    case IL_WAIT_UNLOCK:
    case IL_UNLOCK_AFTER_RELEASE:
        if (key != VK_LCONTROL && key != VK_RCONTROL && key != VK_F12 && key != VK_ESCAPE) {
            inputLocked = IL_WAIT_RELEASE;
            cerr << "Stray key pressed. Wait all released." << endl;
        } else
        if (inputLocked == IL_WAIT_UNLOCK && pressedCount == 4) {
            inputLocked = IL_UNLOCK_AFTER_RELEASE;
            cerr << "Unlock combo, waiting all release." << endl;
        } else
        if (inputLocked == IL_UNLOCK_AFTER_RELEASE && !pressedCount) {
            inputLocked = IL_UNLOCKED;
            cerr << "All keys released. Input unlocked." << endl;
        }
        return false;
    case IL_UNLOCKED:
    default:
        if (!released
            && ((keys[VK_ESCAPE] && keys[VK_LCONTROL]) 
                || (keys[VK_F12] && keys[VK_RCONTROL])))
        {
            keys[key] = -1;
            inputLocked = IL_WAIT_RELEASE;
            cerr << "Input locked. Wait all keys released." << endl;
            return false;
        }
        return true;
    }
}

static constexpr size_t mouseExtraButtons = 8;
static int8_t mouseButtons[3 + mouseExtraButtons] = { 0 };

static bool MouseHandler(int code, WPARAM wParam, LPMSLLHOOKSTRUCT lParam) {
    if (code != HC_ACTION || (lParam->flags & LLMHF_INJECTED)) {
        return true;
    }
    //cerr << "Mouse:\t0x" << wParam << "\t0x" << lParam->pt.x << "\t0x" << lParam->pt.y << "\t0x" << lParam->mouseData << "\t0x" << lParam->flags << endl;
    bool released = false;
    int button = -1;
    switch (wParam) {
    case WM_LBUTTONUP:
        released = true;
    case WM_LBUTTONDOWN:
        button = 0;
        break;
    case WM_RBUTTONUP:
        released = true;
    case WM_RBUTTONDOWN:
        button = 1;
        break;
    case WM_MBUTTONUP:
        released = true;
    case WM_MBUTTONDOWN:
        button = 2;
        break;
    case WM_XBUTTONUP:
        released = true;
    case WM_XBUTTONDOWN:
        button = lParam->mouseData >> 16;
        if (button < 1 || button > mouseExtraButtons) {
            button = -1;
        }
        else {
            button += 2;
        }
        break;
    }
    if (button < 0) {
        return !inputLocked;
    }
    //cerr << "MouseBtn: 0x" << button << "\t" << pressed << endl;
    uint8_t wasPressed = mouseButtons[button];
    if (wasPressed) {
        pressedCount--;
    }
    mouseButtons[button] = released ? 0 : inputLocked ? -1 : 1;
    if (!released) {
        pressedCount++;
    }
    switch (inputLocked) {
    case IL_WAIT_RELEASE:
        if (!pressedCount) {
            inputLocked = IL_WAIT_UNLOCK;
            cerr << "All keys released. Wait unlock combo." << endl;
        }
        return wasPressed == 1 && released;
    case IL_WAIT_UNLOCK:
    case IL_UNLOCK_AFTER_RELEASE:
        inputLocked = IL_WAIT_RELEASE;
        cerr << "Stray key pressed. Wait all released." << endl;
        return false;
    case IL_UNLOCKED:
    default:
        return true;
    }
}

static LRESULT CALLBACK KbdHook(int code, WPARAM wParam, LPARAM lParam) {
    return KbdHandler(code, wParam, reinterpret_cast<LPKBDLLHOOKSTRUCT>(lParam))
        ? CallNextHookEx(0, code, wParam, lParam)
        : true;
}

static LRESULT CALLBACK MouseHook(int code, WPARAM wParam, LPARAM lParam) {
    return MouseHandler(code, wParam, reinterpret_cast<LPMSLLHOOKSTRUCT>(lParam))
        ? CallNextHookEx(0, code, wParam, lParam)
        : true;
}

static void EventLoop() {
    MSG msg;
    cerr << "Entering Event Loop..." << endl;
    while (GetMessage(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    cerr << "Event Loop terminated." << endl;
}

static void Main() {
    WithHook kbdHook(WH_KEYBOARD_LL, KbdHook), mouseHook(WH_MOUSE_LL, MouseHook);
    EventLoop();
}

int main(int argc, char* argv) {
    cerr << std::hex << std::uppercase;
    try {
        cerr << "Starting program..." << endl;
        Main();
    }
    catch (int e) {
        cerr << "Program terminated with error " << e << endl;
        return e;
    }
    cerr << "Program terminated normally." << endl;
    return 0;
}

int WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nShowCmd
) {
    return main(0, nullptr);
}
