/**
 * @file main.c
 * @brief Application entry point for SecureFile Vault desktop application.
 *
 * Implements the WinMain GUI lifecycle, high-DPI awareness initialization,
 * and launches the native WebView2 UI application.
 */

#include <windows.h>

#include "ui.h"

/**
 * Windows application entry point.
 *
 * Rationale:
 * - WinMain suppresses the default Windows command-line console window,
 *   presenting a clean, native desktop application appearance to the user.
 * - SetProcessDPIAware prevents Windows Desktop Window Manager (DWM) from
 *   bitmap-stretching the application window on high-DPI displays (125%, 150%, 200% scaling),
 *   keeping fonts, SVGs, and WebView2 rendering crisp and sharp.
 *
 * @param hInstance Application instance handle.
 * @param hPreviousInstance Legacy 16-bit Windows instance handle (always NULL in Win32).
 * @param lpCmdLine Raw command line arguments string.
 * @param nCmdShow Initial window display state flags.
 * @return Exit code returned from the Win32 message loop.
 */
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPreviousInstance,
                     LPSTR lpCmdLine, int nCmdShow)
{
    (void)hPreviousInstance;
    (void)lpCmdLine;

    /*
     * Declare DPI awareness so Windows does NOT bitmap-scale the window.
     * Without this, text and UI look blurry on high-DPI displays (125%, 150%).
     * SetProcessDPIAware() is available in <windows.h> with MinGW — no extra
     * headers needed. For per-monitor V2 awareness, a manifest is preferred,
     * but this is sufficient for WebView2 to render at native resolution.
     */
    SetProcessDPIAware();

    /* Start the native Win32 window and WebView2 UI thread. */
    return start_gui_application(hInstance, nCmdShow);
}
