# SecureFile Vault

A high-performance Windows desktop application built with a modular C core and a modern React interface embedded via Microsoft Edge WebView2.

Designed for robust cryptographic file security, SecureFile Vault provides in-place symmetric stream ciphering, fail-safe backup recovery, and session audit tracking.

---

## Key Features

- **In-Place Stream Ciphering:** Encrypts and decrypts files in 8KB chunks without loading full files into memory, keeping memory consumption minimal even for multi-gigabyte files.
- **Fail-Safe Automatic Rollback:** Automatically captures a backup snapshot in `%APPDATA%\SecureFileVault\backups` before any in-place mutation. If an error occurs, the original file is restored immediately.
- **Automatic Cleanup:** Successfully decrypted files automatically trigger backup cleanup to remove lingering safety snapshots.
- **Persistent Activity Audit Trail:** Logs cryptographic operations with timestamps to `%APPDATA%\SecureFileVault\logs\activity.log` and presents an interactive session log in the UI.
- **Modern Hybrid Desktop UI:** Built with React 19, Tailwind CSS v4, and Radix UI / shadcn components, rendered natively via WebView2 with dark/light theme switching and high-DPI scaling.

---

## Architectural Overview

SecureFile Vault uses a decoupled, hybrid native-web architecture:

```
┌─────────────────────────────────────────────────────────────┐
│                 Presentation Layer (React 19)               │
│          FileSelector, CipherForm, AuditLogs, Themes        │
└──────────────────────────────┬──────────────────────────────┘
                               │ window.chrome.webview.postMessage(JSON)
                               ▼
┌─────────────────────────────────────────────────────────────┐
│              IPC Bridge Layer (C++17 / WebView2)            │
│         Window Procedure, Native File Dialogs, COM Event    │
│         Virtual Host: https://vault.securefile/             │
└──────────────────────────────┬──────────────────────────────┘
                               │ Direct C function calls
                               ▼
┌─────────────────────────────────────────────────────────────┐
│                   Core Engine Layer (Pure C11)              │
│  ┌──────────────────┐ ┌────────────────┐ ┌────────────────┐ │
│  │   encryption.c   │ │ file_manager.c │ │   storage.c    │ │
│  │ (FNV-1a Keystream│ │ (Binary I/O &  │ │(Logs & Backups │ │
│  │  & Stream XOR)   │ │ Unique Paths)  │ │ in %APPDATA%)  │ │
│  └──────────────────┘ └────────────────┘ └────────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

1. **Frontend (Presentation):** React 19 + TypeScript + Vite. Provides responsive drag-and-drop file selection, real-time key entropy calculation, audit history view, and theme toggling.
2. **Bridge (Host & IPC):** C++17 embedding Microsoft Edge WebView2. Hosts the native Win32 window, triggers native explorer file picker dialogs (`GetOpenFileNameW`), maps the frontend build directory to a secure virtual origin (`https://vault.securefile/`), and handles asynchronous JSON messaging.
3. **Core Engine:** Pure C11 modules. Implements deterministic FNV-1a keystream generation with byte-offset mixing, in-place binary I/O, AppData path resolution, and atomic backup rollbacks.

---

## Project Structure

```text
project/
├── build/                           # Output binaries and assets
│   ├── SecureFileVault.exe          # Compiled desktop application
│   └── WebView2Loader.dll           # Runtime loader DLL
├── include/                         # C header specifications
│   ├── encryption.h                 # Cipher engine interfaces
│   ├── file_manager.h               # Binary file utilities
│   ├── storage.h                    # %APPDATA% storage & log headers
│   └── ui.h                         # Native UI entry prototype
├── resources/                       # Win32 application icons & RC script
├── src/                             # Core C & C++ source code
│   ├── encryption.c                 # Stream cipher & in-place transform
│   ├── file_manager.c               # Safe file operations & path generation
│   ├── main.c                       # WinMain entry point & DPI setup
│   ├── storage.c                    # %APPDATA% path expansion, logs, backups
│   └── ui_webview.cpp               # WebView2 COM integration & IPC bridge
├── third_party/
│   └── WebView2/                    # Microsoft WebView2 native SDK
├── web/                             # React frontend client
│   ├── src/                         # React components, hooks, and types
│   ├── package.json                 # Web dependencies
│   └── vite.config.ts               # Vite build config
├── build.bat                        # Automated Windows build script
└── Makefile                         # GNU Make build configuration
```

---

## Storage Standards (Windows AppData)

To adhere to standard Windows desktop security and roaming conventions:
- **Application Data Root:** `%APPDATA%\SecureFileVault\` (`C:\Users\<User>\AppData\Roaming\SecureFileVault`)
- **Activity Log File:** `%APPDATA%\SecureFileVault\logs\activity.log`
- **Fail-Safe Backups:** `%APPDATA%\SecureFileVault\backups\<filename>.backup`
- **WebView2 Browser Cache:** `%LOCALAPPDATA%\SecureFileVault\webview\`

---

## Prerequisites

Before building, ensure the following are available on your system:
1. **MinGW-w64** (`gcc`, `g++`, `windres`) installed and configured in your system `PATH`.
2. **Microsoft Edge WebView2 Runtime** (installed by default on Windows 10/11).
3. **Node.js (v18+) & npm** (required to compile the React frontend bundle).

---

## How to Build and Run

### 1. Build Everything (Recommended)

Run the included automated batch script from the project root:

```cmd
.\build.bat
```

This will automatically:
1. Install frontend dependencies and build the optimized production web bundle (`web/dist`).
2. Compile C and C++ source files with MinGW.
3. Compile the Win32 application icon resource (`.rc`).
4. Link the executable into `build\SecureFileVault.exe` and copy required runtime DLLs.

### 2. Build Using GNU Make

If you have already built the frontend or want to compile the native code directly:

```cmd
make
```

To clean previous build objects and output files:

```cmd
make clean
```

### 3. Running the Application

Launch the compiled desktop binary:

```cmd
.\build\SecureFileVault.exe
```

---

## Development Mode (Frontend Live Reload)

For fast UI iteration with Hot Module Replacement (HMR):

1. Start the Vite development server in the `web/` directory:
   ```cmd
   cd web
   npm run dev
   ```
2. Launch `SecureFileVault.exe` in another terminal.
3. If no production `web/dist` folder is found, the C++ host automatically falls back to connecting directly to `http://localhost:5173`, allowing live frontend editing without recompiling native code.
