/**
 * @file ui_webview.cpp
 * @brief Win32 desktop window host and Microsoft Edge WebView2 COM bridge.
 *
 * Implements the native Win32 window message loop, hosts the Microsoft Edge
 * WebView2 runtime, and bridges JSON-based IPC messages between the React web
 * interface and the native C cryptographic file engine.
 */

#include <windows.h>
#include <commdlg.h>
#include <shlwapi.h>
#include <string>
#include <fstream>
#include <sstream>

#include "WebView2.h"
extern "C" {
#include "encryption.h"
#include "ui.h"
}

#define IDI_ICON1 1

static HWND g_window = NULL;
static HMODULE g_loader = NULL;
static ICoreWebView2Controller *g_controller = NULL;
static ICoreWebView2 *g_view = NULL;
static std::wstring g_html;
static bool g_com_initialized = false;

/**
 * Converts a null-terminated ANSI (CP_ACP) string to a UTF-16 wide string.
 *
 * Rationale:
 * - Win32 UI and WebView2 interfaces require wide strings (wchar_t / UTF-16).
 * - MultiByteToWideChar with CP_ACP converts local system code page characters
 *   into standard Windows wide strings safely.
 *
 * @param text ANSI input string.
 * @return Converted UTF-16 std::wstring.
 */
static std::wstring ansi_to_wide(const char *text)
{
    int length;
    std::wstring result;
    if (text == NULL || text[0] == '\0') return result;
    length = MultiByteToWideChar(CP_ACP, 0, text, -1, NULL, 0);
    if (length <= 1) return result;
    result.resize(length);
    MultiByteToWideChar(CP_ACP, 0, text, -1, &result[0], length);
    result.resize(length - 1);
    return result;
}

/**
 * Converts a UTF-8 encoded byte string to a UTF-16 wide string.
 *
 * Rationale:
 * - Frontend assets and JSON string payloads are UTF-8 encoded.
 * - MultiByteToWideChar with CP_UTF8 preserves full Unicode character fidelity
 *   for international filenames and special characters.
 *
 * @param text UTF-8 input string.
 * @return Converted UTF-16 std::wstring.
 */
static std::wstring utf8_to_wide(const char *text)
{
    int length;
    std::wstring result;
    if (text == NULL || text[0] == '\0') return result;
    length = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
    if (length <= 1) return result;
    result.resize(length);
    MultiByteToWideChar(CP_UTF8, 0, text, -1, &result[0], length);
    result.resize(length - 1);
    return result;
}

/**
 * Converts a UTF-16 wide string to a local ANSI string for C filesystem I/O.
 *
 * @param text Wide string to convert.
 * @return Converted std::string in local ANSI code page.
 */
static std::string wide_to_ansi(const std::wstring &text)
{
    int length;
    std::string result;
    if (text.empty()) return result;
    length = WideCharToMultiByte(CP_ACP, 0, text.c_str(), -1, NULL, 0, NULL, NULL);
    if (length <= 1) return result;
    result.resize(length);
    WideCharToMultiByte(CP_ACP, 0, text.c_str(), -1, &result[0], length, NULL, NULL);
    result.resize(length - 1);
    return result;
}

/**
 * Escapes special characters within a string for safe JavaScript code injection.
 *
 * Rationale:
 * - When passing file paths or messages via ExecuteScript, unescaped backslashes
 *   (e.g., in "C:\Users\...") are interpreted by JavaScript as escape characters
 *   (like \U or \n), corrupting the path or causing JavaScript syntax errors.
 * - Escaping backslashes, quotes, and newlines ensures safe string literal interpolation.
 *
 * @param text Raw wide string.
 * @return JavaScript string literal wrapped in quotes with special characters escaped.
 */
static std::wstring js_quote(const std::wstring &text)
{
    std::wstring result = L"\"";
    for (wchar_t character : text)
    {
        if (character == L'\\' || character == L'\"') result += L'\\';
        if (character == L'\r') result += L"\\r";
        else if (character == L'\n') result += L"\\n";
        else result += character;
    }
    result += L"\"";
    return result;
}

/**
 * Executes a JavaScript snippet asynchronously in the hosted WebView2 context.
 *
 * @param script JavaScript code string to execute.
 */
static void execute_script(const std::wstring &script)
{
    if (g_view != NULL) g_view->ExecuteScript(script.c_str(), NULL);
}

/**
 * Dispatches an operation status update to the frontend React UI.
 *
 * Rationale:
 * - Checks both window.setStatus (modern React window bridge) and legacy global
 *   setStatus to guarantee compatibility across development and production builds.
 *
 * @param message Human-readable status text.
 * @param success 1 for success (true), 0 for failure (false), -1 for neutral/loading (null).
 */
static void set_status(const std::wstring &message, int success)
{
    std::wstring script = L"if (typeof window !== 'undefined' && window.setStatus) { window.setStatus(" + js_quote(message) + L",";
    script += success > 0 ? L"true" : success == 0 ? L"false" : L"null";
    script += L"); } else if (typeof setStatus === 'function') { setStatus(" + js_quote(message) + L",";
    script += success > 0 ? L"true" : success == 0 ? L"false" : L"null";
    script += L"); }";
    execute_script(script);
}

/**
 * Retrieves the directory path containing the running executable.
 *
 * @return Wide string representing the executable's parent folder.
 */
static std::wstring module_directory()
{
    wchar_t path[4096];
    DWORD length = GetModuleFileNameW(NULL, path, sizeof(path) / sizeof(path[0]));
    if (length == 0 || length >= sizeof(path) / sizeof(path[0])) return L".";
    std::wstring result(path, length);
    size_t separator = result.find_last_of(L"\\/");
    return separator == std::wstring::npos ? L"." : result.substr(0, separator);
}

/**
 * Loads the fallback web/index.html file from the local filesystem.
 *
 * @return true if file opened and loaded, false otherwise.
 */
static bool load_html()
{
    std::wstring path = module_directory() + L"\\web\\index.html";
    std::ifstream file(path.c_str(), std::ios::binary);
    std::ostringstream contents;
    if (!file.is_open()) return false;
    contents << file.rdbuf();
    g_html = utf8_to_wide(contents.str().c_str());
    return !g_html.empty();
}

/**
 * Lightweight, zero-dependency JSON string value extractor.
 *
 * Rationale:
 * - Avoids pulling in heavy external JSON library dependencies into the binary.
 * - Extracts string values by key name while unescaping standard JSON escape sequences
 *   (\\n, \\r, \\t, \\", \\\\) to reconstruct raw file paths and passwords accurately.
 *
 * @param json Complete raw JSON string payload received from WebView2.
 * @param key JSON object key name to extract.
 * @return Extracted and unescaped wide string value, or empty string if not found.
 */
static std::wstring extract_json_string(const std::wstring &json, const std::wstring &key)
{
    std::wstring marker = L"\"" + key + L"\"";
    size_t start = json.find(marker);
    std::wstring result;
    if (start == std::wstring::npos) return result;
    start = json.find(L':', start + marker.length());
    if (start == std::wstring::npos) return result;
    start = json.find(L'\"', start + 1);
    if (start == std::wstring::npos) return result;
    ++start;
    for (size_t index = start; index < json.length(); ++index)
    {
        wchar_t character = json[index];
        if (character == L'\"') break;
        if (character == L'\\' && index + 1 < json.length())
        {
            wchar_t escaped = json[++index];
            if (escaped == L'n') result += L'\n';
            else if (escaped == L'r') result += L'\r';
            else if (escaped == L't') result += L'\t';
            else result += escaped;
        }
        else result += character;
    }
    return result;
}

/**
 * Opens the native Windows file selection dialog.
 *
 * Rationale:
 * - Browsing from native code via GetOpenFileNameW provides native OS dialog integration,
 *   supporting multi-drive navigation and localized explorer dialogs.
 * - Once selected, passes the absolute file path to the frontend via window.setFile.
 */
static void browse_for_file()
{
    wchar_t selected_file[4096] = L"";
    OPENFILENAMEW dialog;
    ZeroMemory(&dialog, sizeof(dialog));
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = g_window;
    dialog.lpstrFilter = L"All Files\0*.*\0Documents\0*.pdf;*.doc;*.docx\0Images\0*.jpg;*.jpeg;*.png\0Archives\0*.zip;*.rar;*.7z\0\0";
    dialog.lpstrFile = selected_file;
    dialog.nMaxFile = sizeof(selected_file) / sizeof(selected_file[0]);
    dialog.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    if (GetOpenFileNameW(&dialog))
    {
        std::wstring script = L"if (typeof window !== 'undefined' && window.setFile) { window.setFile(" + js_quote(selected_file) + L"); } else if (typeof setFile === 'function') { setFile(" + js_quote(selected_file) + L"); }";
        execute_script(script);
    }
}

/**
 * Dispatches an encryption or decryption operation requested by the web UI.
 *
 * @param json Payload containing "file" and "password" parameters.
 * @param encrypt true to encrypt the file, false to decrypt.
 */
static void perform_operation(const std::wstring &json, bool encrypt)
{
    std::string file = wide_to_ansi(extract_json_string(json, L"file"));
    std::string password = wide_to_ansi(extract_json_string(json, L"password"));
    char output_path[4096];
    if (file.empty() || password.empty()) return;

    bool success = encrypt
        ? encrypt_file(file.c_str(), password.c_str(), output_path, sizeof(output_path)) != 0
        : decrypt_file(file.c_str(), password.c_str(), output_path, sizeof(output_path)) != 0;
    if (success)
    {
        std::string filename = file;
        size_t last_sep = filename.find_last_of("\\/");
        if (last_sep != std::string::npos)
        {
            filename = filename.substr(last_sep + 1);
        }

        std::wstring message = encrypt
            ? L"File encrypted successfully: " + ansi_to_wide(filename.c_str())
            : L"File decrypted successfully: " + ansi_to_wide(filename.c_str());
        set_status(message, 1);
    }
    else
    {
        set_status(encrypt
                       ? L"Encryption failed. Check that the file is writable and not open in another app."
                       : L"Decryption failed. The password may be incorrect or the file may be corrupted.",
                   0);
    }
}

/**
 * COM event handler for incoming messages from the frontend JavaScript context
 * via `window.chrome.webview.postMessage(...)`.
 *
 * Uses standard COM reference counting; deletes itself when reference count reaches 0.
 */
class MessageHandler : public ICoreWebView2WebMessageReceivedEventHandler
{
    ULONG references = 1;
public:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **object) override
    {
        if (object == NULL) return E_POINTER;
        *object = NULL;
        if (riid == IID_IUnknown || riid == IID_ICoreWebView2WebMessageReceivedEventHandler)
        {
            *object = static_cast<ICoreWebView2WebMessageReceivedEventHandler *>(this);
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++references; }
    ULONG STDMETHODCALLTYPE Release() override
    {
        ULONG remaining = --references;
        if (remaining == 0) delete this;
        return remaining;
    }
    HRESULT STDMETHODCALLTYPE Invoke(ICoreWebView2 *, ICoreWebView2WebMessageReceivedEventArgs *args) override
    {
        LPWSTR raw_json = NULL;
        if (args == NULL || FAILED(args->TryGetWebMessageAsString(&raw_json))) return S_OK;
        std::wstring json(raw_json);
        CoTaskMemFree(raw_json);
        std::wstring action = extract_json_string(json, L"action");
        if (action == L"browse") browse_for_file();
        else if (action == L"encrypt") perform_operation(json, true);
        else if (action == L"decrypt") perform_operation(json, false);
        return S_OK;
    }
};

/**
 * COM completion handler invoked once the CoreWebView2 controller is instantiated.
 *
 * Configures window bounds, message handlers, and loads the active frontend
 * bundle via virtual host folder mapping or Vite dev server.
 */
class ControllerHandler : public ICoreWebView2CreateCoreWebView2ControllerCompletedHandler
{
    ULONG references = 1;
public:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **object) override
    {
        if (object == NULL) return E_POINTER;
        *object = NULL;
        if (riid == IID_IUnknown || riid == IID_ICoreWebView2CreateCoreWebView2ControllerCompletedHandler)
        {
            *object = static_cast<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler *>(this);
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++references; }
    ULONG STDMETHODCALLTYPE Release() override
    {
        ULONG remaining = --references;
        if (remaining == 0) delete this;
        return remaining;
    }
    HRESULT STDMETHODCALLTYPE Invoke(HRESULT error, ICoreWebView2Controller *controller) override
    {
        if (FAILED(error) || controller == NULL) return S_OK;
        g_controller = controller;
        g_controller->AddRef();
        if (FAILED(g_controller->get_CoreWebView2(&g_view))) return S_OK;
        RECT bounds;
        GetClientRect(g_window, &bounds);
        g_controller->put_Bounds(bounds);
        g_controller->put_IsVisible(TRUE);
        EventRegistrationToken token;
        MessageHandler *handler = new MessageHandler();
        g_view->add_WebMessageReceived(handler, &token);
        handler->Release();

        /*
         * Frontend Resolution Strategy:
         * 1. Look for pre-built production distribution files in web\dist.
         * 2. If present, map them to a virtual HTTPS host (https://vault.securefile/)
         *    using SetVirtualHostNameToFolderMapping. This provides a secure origin context,
         *    enabling CORS, modern ES modules, and localStorage without file:/// protocol limitations.
         * 3. If no production build exists, automatically fall back to the live Vite dev server
         *    at http://localhost:5173 for rapid interactive development and HMR.
         */
        std::wstring module_dir = module_directory();
        std::wstring dist_dir1 = module_dir + L"\\web\\dist";
        std::wstring dist_dir2 = module_dir + L"\\..\\web\\dist";
        std::wstring active_dist;

        if (GetFileAttributesW((dist_dir1 + L"\\index.html").c_str()) != INVALID_FILE_ATTRIBUTES)
        {
            active_dist = dist_dir1;
        }
        else if (GetFileAttributesW((dist_dir2 + L"\\index.html").c_str()) != INVALID_FILE_ATTRIBUTES)
        {
            active_dist = dist_dir2;
        }

        if (!active_dist.empty())
        {
            wchar_t full_dist_path[MAX_PATH];
            if (GetFullPathNameW(active_dist.c_str(), MAX_PATH, full_dist_path, NULL) > 0)
            {
                active_dist = full_dist_path;
            }

            ICoreWebView2_3 *view3 = NULL;
            if (SUCCEEDED(g_view->QueryInterface(IID_ICoreWebView2_3, reinterpret_cast<void **>(&view3))) && view3 != NULL)
            {
                view3->SetVirtualHostNameToFolderMapping(
                    L"vault.securefile",
                    active_dist.c_str(),
                    COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_ALLOW
                );
                view3->Release();
                g_view->Navigate(L"https://vault.securefile/index.html");
                return S_OK;
            }

            /* Fallback to file URL if ICoreWebView2_3 is not supported by runtime. */
            g_view->Navigate((L"file:///" + active_dist + L"\\index.html").c_str());
        }
        else
        {
            /* Fallback to active Vite development server for live HMR frontend iteration. */
            g_view->Navigate(L"http://localhost:5173");
        }
        return S_OK;
    }
};

/**
 * COM completion handler invoked once the WebView2 environment is created.
 */
class EnvironmentHandler : public ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler
{
    ULONG references = 1;
public:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **object) override
    {
        if (object == NULL) return E_POINTER;
        *object = NULL;
        if (riid == IID_IUnknown || riid == IID_ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler)
        {
            *object = static_cast<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler *>(this);
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++references; }
    ULONG STDMETHODCALLTYPE Release() override
    {
        ULONG remaining = --references;
        if (remaining == 0) delete this;
        return remaining;
    }
    HRESULT STDMETHODCALLTYPE Invoke(HRESULT error, ICoreWebView2Environment *environment) override
    {
        if (FAILED(error) || environment == NULL) return S_OK;
        environment->CreateCoreWebView2Controller(g_window, new ControllerHandler());
        return S_OK;
    }
};

typedef HRESULT (STDAPICALLTYPE *CreateEnvironmentFunction)(PCWSTR, PCWSTR,
    ICoreWebView2EnvironmentOptions *, ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler *);

/**
 * Initializes the Microsoft Edge WebView2 runtime environment.
 *
 * Rationale:
 * - Dynamically loads WebView2Loader.dll from the application directory.
 * - Stores WebView2 browser cache and profile data in %LOCALAPPDATA%\SecureFileVault\webview
 *   to avoid requiring administrative privileges and prevent corrupting user roaming profiles.
 *
 * @return true on successful environment initialization initiation, false on missing DLL or runtime.
 */
static bool initialize_webview()
{
    wchar_t local_app_data[4096];
    wchar_t user_data[4096];
    DWORD length;
    CreateEnvironmentFunction create_environment;

    load_html();
    g_loader = LoadLibraryW(L"WebView2Loader.dll");
    if (g_loader == NULL) return false;
    create_environment = reinterpret_cast<CreateEnvironmentFunction>(GetProcAddress(g_loader, "CreateCoreWebView2EnvironmentWithOptions"));
    if (create_environment == NULL) return false;
    length = GetEnvironmentVariableW(L"LOCALAPPDATA", local_app_data, sizeof(local_app_data) / sizeof(local_app_data[0]));
    if (length == 0 || length >= sizeof(local_app_data) / sizeof(local_app_data[0])) return false;
    wsprintfW(user_data, L"%s\\SecureFileVault\\webview", local_app_data);
    CreateDirectoryW(local_app_data, NULL);
    std::wstring vault_directory = std::wstring(local_app_data) + L"\\SecureFileVault";
    CreateDirectoryW(vault_directory.c_str(), NULL);
    CreateDirectoryW(user_data, NULL);
    create_environment(NULL, user_data, NULL, new EnvironmentHandler());
    return true;
}

/**
 * Synchronizes the WebView2 viewport bounds with the Win32 window client area.
 */
static void resize_webview()
{
    if (g_controller != NULL)
    {
        RECT bounds;
        GetClientRect(g_window, &bounds);
        g_controller->put_Bounds(bounds);
    }
}

/**
 * Win32 Window Procedure handling OS messages for the desktop container.
 */
static LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        g_window = hwnd;

        /*
         * WebView2 COM architecture requires a Single-Threaded Apartment (STA)
         * thread to process COM callbacks and marshal DOM events reliably.
         */
        if (SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)))
        {
            g_com_initialized = true;
        }
        if (!initialize_webview())
        {
            MessageBoxW(hwnd, L"WebView2 could not be initialized. Ensure the WebView2 Runtime is installed.", L"SecureFile Vault", MB_ICONERROR);
            PostQuitMessage(1);
        }
        return 0;
    case WM_SIZE:
        resize_webview();
        return 0;
    case WM_MOUSEMOVE:
        /*
         * In Win32, WM_MOUSEWHEEL is delivered to the window that currently owns
         * KEYBOARD FOCUS, not necessarily the window beneath the mouse cursor.
         * When the user moves the mouse into the WebView2 client region, we explicitly
         * transfer focus to the WebView2 controller so mouse wheel and touchpad
         * scrolling immediately work without requiring an explicit preliminary click.
         */
        if (g_controller != NULL)
        {
            g_controller->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);
        }
        return 0;
    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL:
        if (g_controller != NULL)
        {
            g_controller->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);
        }
        return DefWindowProcW(hwnd, message, wParam, lParam);
    case WM_DESTROY:
        if (g_view != NULL) { g_view->Release(); g_view = NULL; }
        if (g_controller != NULL) { g_controller->Close(); g_controller->Release(); g_controller = NULL; }
        if (g_loader != NULL) { FreeLibrary(g_loader); g_loader = NULL; }
        if (g_com_initialized) { CoUninitialize(); g_com_initialized = false; }
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

/**
 * Registers the Win32 window class and starts the application GUI message pump.
 *
 * @param hInstance Application instance handle.
 * @param nCmdShow Initial window show flag.
 * @return Exit code from the message loop.
 */
extern "C" int start_gui_application(HINSTANCE hInstance, int nCmdShow)
{
    WNDCLASSEXW window_class;
    HWND window;
    MSG message;
    ZeroMemory(&window_class, sizeof(window_class));
    window_class.cbSize = sizeof(WNDCLASSEXW);
    window_class.lpfnWndProc = WindowProcedure;
    window_class.hInstance = hInstance;
    window_class.hCursor = LoadCursor(NULL, IDC_ARROW);
    window_class.lpszClassName = L"SecureFileVaultWebViewWindow";
    window_class.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    /*
     * Explicitly load 32x32 and 16x16 icon sizes from embedded resources.
     * Prevents Windows from blurry downscaling of standard 256x256 icon frames.
     */
    HICON hIconBig = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_ICON1),
                                       IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
    if (!hIconBig)
    {
        hIconBig = (HICON)LoadImageW(hInstance, L"IDI_ICON1",
                                     IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
    }

    HICON hIconSmall = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_ICON1),
                                         IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    if (!hIconSmall)
    {
        hIconSmall = (HICON)LoadImageW(hInstance, L"IDI_ICON1",
                                       IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    }

    window_class.hIcon   = hIconBig;
    window_class.hIconSm = hIconSmall;

    if (!RegisterClassExW(&window_class)) return 1;

    window = CreateWindowExW(0, window_class.lpszClassName, L"SecureFile Vault",
                            WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                            1180, 820, NULL, NULL, hInstance, NULL);
    if (window == NULL) return 1;

    /* Set WM_SETICON so titlebar and taskbar display the icons immediately. */
    SendMessageW(window, WM_SETICON, ICON_BIG,   (LPARAM)hIconBig);
    SendMessageW(window, WM_SETICON, ICON_SMALL, (LPARAM)hIconSmall);

    ShowWindow(window, nCmdShow);
    UpdateWindow(window);
    while (GetMessageW(&message, NULL, 0, 0))
    {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return (int)message.wParam;
}
