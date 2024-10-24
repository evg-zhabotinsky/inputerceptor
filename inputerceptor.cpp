#include <Windows.h>
#include <iostream>
#include <string>
#include <vector>

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
static bool disableLocking = false;

static bool KbdLockHandler(WPARAM wParam, LPKBDLLHOOKSTRUCT lParam) {
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
    if (disableLocking) {
        return true;
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
        return wasPressed == 1 && released;;
    case IL_UNLOCKED:
    default:
        if (!released
            && ((keys[VK_ESCAPE] && keys[VK_LCONTROL] && !keys[VK_LSHIFT] && !keys[VK_LMENU] && !keys[VK_LWIN])
                || (keys[VK_F12] && keys[VK_RCONTROL] && !keys[VK_RSHIFT] && !keys[VK_RMENU] && !keys[VK_RWIN])))
        {
            bool strays = keys[VK_LCONTROL] + keys[VK_RCONTROL] + keys[VK_ESCAPE] + keys[VK_F12] < pressedCount;
            inputLocked = strays ? IL_WAIT_RELEASE : IL_WAIT_UNLOCK;
            cerr << "Input locked. Wait " << (strays ? "all keys released." : "unlock combo.") << endl;
            keys[key] = -1;
            return false;
        }
        return true;
    }
}

static constexpr size_t mouseExtraButtons = 8;
static int8_t mouseButtons[3 + mouseExtraButtons] = { 0 };

static bool MouseLockHandler(WPARAM wParam, LPMSLLHOOKSTRUCT lParam) {
    bool released = false;
    int button = -1;
    switch (wParam) {
    case WM_LBUTTONUP:
        released = true;
        [[fallthrough]];
    case WM_LBUTTONDOWN:
        button = 0;
        break;
    case WM_RBUTTONUP:
        released = true;
        [[fallthrough]];
    case WM_RBUTTONDOWN:
        button = 1;
        break;
    case WM_MBUTTONUP:
        released = true;
        [[fallthrough]];
    case WM_MBUTTONDOWN:
        button = 2;
        break;
    case WM_XBUTTONUP:
        released = true;
        [[fallthrough]];
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
    if (disableLocking) {
        return true;
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

static void NextLayout() {
    // Assumes neither LWin nor Space are pressed!
    static INPUT layoutInputs[] = {
        {.type = INPUT_KEYBOARD, .ki = {VK_LWIN}},
        {.type = INPUT_KEYBOARD, .ki = {VK_SPACE}},
        {.type = INPUT_KEYBOARD, .ki = {VK_SPACE, 0, KEYEVENTF_KEYUP}},
        {.type = INPUT_KEYBOARD, .ki = {VK_LWIN, 0, KEYEVENTF_KEYUP}}
    };
    cerr << "Switching keyboard layout." << endl;
    SendInput(sizeof(layoutInputs) / sizeof(layoutInputs[0]), layoutInputs, sizeof(layoutInputs[0]));
}

static bool useCapsLayout = false, releaseCaps = false;

static bool CapsLockHandler(WPARAM wParam, LPKBDLLHOOKSTRUCT lParam) {
    if (!useCapsLayout || lParam->vkCode != VK_CAPITAL) {
        return true;
    }
    bool released = lParam->flags & LLKHF_UP;
    if (releaseCaps) {
        if (released) {
            releaseCaps = false;
        }
        return false;
    }
    if (released || pressedCount != 1) {
        return true;
    }
    releaseCaps = true;
    NextLayout();
    return false;
}

static bool KbdHandler(int code, WPARAM wParam, LPKBDLLHOOKSTRUCT lParam) {
    if (code != HC_ACTION || (lParam->flags & LLKHF_INJECTED)) {
        return true;
    }
    //cerr << "Kbd:\t0x" << wParam << "\t0x" << lParam->vkCode << "\t0x" << lParam->scanCode << "\t0x" << lParam->flags << endl;
    //return true;
    if (!KbdLockHandler(wParam, lParam)) {
        return false;
    }
    if (!CapsLockHandler(wParam, lParam)) {
        return false;
    }
    cerr << "Kbd:\t0x" << wParam << "\t0x" << lParam->vkCode << "\t0x" << lParam->scanCode << "\t0x" << lParam->flags << endl;
    return true;
}

static bool MouseHandler(int code, WPARAM wParam, LPMSLLHOOKSTRUCT lParam) {
    if (code != HC_ACTION || (lParam->flags & LLMHF_INJECTED)) {
        return true;
    }
    //cerr << "Mouse:\t0x" << wParam << "\t0x" << lParam->pt.x << "\t0x" << lParam->pt.y << "\t0x" << lParam->mouseData << "\t0x" << lParam->flags << endl;
    if (!MouseLockHandler(wParam, lParam)) {
        return false;
    }
    return true;
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

static void ParseArgs(const std::vector<std::string> &args) {
    for (auto s : args) {
        if (s == "caps-layout") {
            if (useCapsLayout) {
                cerr << "Duplicate option: " << s << endl;
                throw 2;
            }
            useCapsLayout = true;
            cerr << "Using CapsLock to switch keyboard layout." << endl;
        }
        else if (s == "disable-locking") {
            if (disableLocking) {
                cerr << "Duplicate option: " << s << endl;
                throw 2;
            }
            disableLocking = true;
            cerr << "Input locking functionality disabled." << endl;
        }
        else {
            cerr << "Unknown option: " << s << endl;
            throw 2;
        }
    }
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

static int Main(const std::vector<std::string> &args) {
    try {
        cerr << "Starting inputerceptor..." << endl;
        cerr << std::hex << std::uppercase;
        ParseArgs(args);
        WithHook kbdHook(WH_KEYBOARD_LL, KbdHook), mouseHook(WH_MOUSE_LL, MouseHook);
        EventLoop();
    }
    catch (int e) {
        cerr << "Program terminated with error " << e << endl;
        return e;
    }
    cerr << "Program terminated normally." << endl;
    return 0;
}

int main(int argc, const char* argv[]) {
    std::vector<std::string> args;
    for (int i = 1; i < argc; i++) {
        args.emplace_back(argv[i]);
    }
    return Main(args);
}

int WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nShowCmd
) {
    // Copy and split command line with zeros
    std::string cmdLine(lpCmdLine);
    std::vector<const char*> argv;
    bool space = true;
    for (char &c : cmdLine) {
        if (space && c != ' ') {
            space = false;
            argv.push_back(&c);
        }
        else if (!space && c == ' ') {
            space = true;
            c = 0;
        }
    }
    std::vector<std::string> args;
    for (const char* s : argv) {
        args.emplace_back(s);
    }
    return Main(args);
}
