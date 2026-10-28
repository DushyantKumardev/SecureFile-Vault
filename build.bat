@echo off
setlocal

echo.
echo  ==========================================
echo   SecureFile Vault -- Build Script
echo  ==========================================
echo.

:: ── Tools ──────────────────────────────────────────────
set CC=gcc
set CXX=g++
set WINDRES=windres

:: ── Flags ──────────────────────────────────────────────
set CFLAGS=-std=c11 -Wall -Wextra -Iinclude
set CXXFLAGS=-std=c++17 -Wall -Wextra -Iinclude -Ithird_party\WebView2\build\native\include

:: ── Paths ──────────────────────────────────────────────
set TARGET=build\SecureFileVault.exe
set WEBVIEW_DLL=third_party\WebView2\build\native\x64\WebView2Loader.dll

:: ── Create build folder ────────────────────────────────
if not exist build mkdir build
if not exist build\web mkdir build\web

:: ── Build frontend client ──────────────────────────────
echo [1/7] Building frontend client (web) ...
if not exist web (
  echo ERROR: web directory not found & exit /b 1
)

where npm >NUL 2>&1
if errorlevel 1 (
  echo ERROR: npm was not found in PATH. Please install Node.js to build the client. & exit /b 1
)

if not exist web\node_modules (
  echo       Installing frontend dependencies ...
  pushd web
  call npm install
  if errorlevel 1 ( popd & echo ERROR: npm install failed & exit /b 1 )
  popd
)

pushd web
call npm run build
if errorlevel 1 ( popd & echo ERROR: Frontend client build failed & exit /b 1 )
popd

:: ── Compile C files ────────────────────────────────────
echo [2/7] Compiling main.c ...
%CC% %CFLAGS% -c src\main.c -o build\main.o
if errorlevel 1 ( echo ERROR: main.c failed & exit /b 1 )

echo [3/7] Compiling encryption.c ...
%CC% %CFLAGS% -c src\encryption.c -o build\encryption.o
if errorlevel 1 ( echo ERROR: encryption.c failed & exit /b 1 )

echo [4/7] Compiling file_manager.c ...
%CC% %CFLAGS% -c src\file_manager.c -o build\file_manager.o
if errorlevel 1 ( echo ERROR: file_manager.c failed & exit /b 1 )

echo [5/7] Compiling storage.c ...
%CC% %CFLAGS% -c src\storage.c -o build\storage.o
if errorlevel 1 ( echo ERROR: storage.c failed & exit /b 1 )

:: ── Compile C++ (WebView2 UI) ──────────────────────────
echo [6/7] Compiling ui_webview.cpp ...
%CXX% %CXXFLAGS% -c src\ui_webview.cpp -o build\ui_webview.o
if errorlevel 1 ( echo ERROR: ui_webview.cpp failed & exit /b 1 )

:: ── Compile resource file ──────────────────────────────
echo [7/7] Compiling resource file ...
%WINDRES% resources\file_security_tool.rc -O coff -o build\file_security_tool.res
if errorlevel 1 ( echo ERROR: resource compile failed & exit /b 1 )

:: ── Link everything ────────────────────────────────────
echo.
echo  Linking ...
%CXX% build\main.o build\encryption.o build\file_manager.o build\storage.o ^
      build\ui_webview.o build\file_security_tool.res ^
      -mwindows -lole32 -lshlwapi -luuid -o %TARGET%
if errorlevel 1 ( echo ERROR: Linking failed & exit /b 1 )

:: ── Copy runtime files ─────────────────────────────────
echo  Copying WebView2Loader.dll ...
copy /Y %WEBVIEW_DLL% build\WebView2Loader.dll >NUL

echo  Copying web UI ...
copy /Y web\index.html build\web\index.html >NUL
copy /Y resources\file_security_tool.ico build\web\favicon.ico >NUL
if exist web\dist (
  if exist build\web\dist rmdir /S /Q build\web\dist
  mkdir build\web\dist
  xcopy /E /I /Y web\dist build\web\dist >NUL
)

echo.
echo  ==========================================
echo   BUILD SUCCESSFUL!
echo   Run: build\SecureFileVault.exe
echo  ==========================================
echo.

endlocal
