CC := gcc
CXX := g++
WINDRES := windres
CFLAGS ?= -std=c11 -Wall -Wextra -Iinclude
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Iinclude -Ithird_party\WebView2\build\native\include
TARGET := build\SecureFileVault.exe
C_OBJECTS := build\main.o build\encryption.o build\file_manager.o build\storage.o
UI_OBJECT := build\ui_webview.o

RESOURCE_SCRIPT := resources\file_security_tool.rc
RESOURCE_OBJECT := build\file_security_tool.res
WEBVIEW_DLL := third_party\WebView2\build\native\x64\WebView2Loader.dll

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(C_OBJECTS) $(UI_OBJECT) $(RESOURCE_OBJECT) | build
	$(CXX) $(C_OBJECTS) $(UI_OBJECT) $(RESOURCE_OBJECT) -mwindows -lole32 -lshlwapi -luuid -o $(TARGET)
	@copy /Y $(WEBVIEW_DLL) build\WebView2Loader.dll >NUL
	@if not exist build\web mkdir build\web
	@copy /Y web\index.html build\web\index.html >NUL
	@copy /Y resources\file_security_tool.ico build\web\favicon.ico >NUL

build\main.o: src\main.c include\ui.h
	$(CC) $(CFLAGS) -c src\main.c -o build\main.o

build\encryption.o: src\encryption.c include\encryption.h include\file_manager.h include\storage.h
	$(CC) $(CFLAGS) -c src\encryption.c -o build\encryption.o

build\file_manager.o: src\file_manager.c include\file_manager.h
	$(CC) $(CFLAGS) -c src\file_manager.c -o build\file_manager.o

build\storage.o: src\storage.c include\storage.h include\file_manager.h
	$(CC) $(CFLAGS) -c src\storage.c -o build\storage.o

build\ui_webview.o: src\ui_webview.cpp web\index.html include\encryption.h include\ui.h third_party\WebView2\build\native\include\WebView2.h
	$(CXX) $(CXXFLAGS) -c src\ui_webview.cpp -o $(UI_OBJECT)

$(RESOURCE_OBJECT): $(RESOURCE_SCRIPT) resources\file_security_tool.ico | build
	$(WINDRES) $(RESOURCE_SCRIPT) -O coff -o $(RESOURCE_OBJECT)

build:
	@if not exist build mkdir build

clean:
	@if exist build\SecureFileVault.exe del /Q build\SecureFileVault.exe
	@if exist build\fileguard.exe del /Q build\fileguard.exe
	@if exist build\fileguard_resource.o del /Q build\fileguard_resource.o
	@if exist build\file_security_tool.res del /Q build\file_security_tool.res
	@if exist build\main.o del /Q build\main.o
	@if exist build\encryption.o del /Q build\encryption.o
	@if exist build\file_manager.o del /Q build\file_manager.o
	@if exist build\storage.o del /Q build\storage.o
	@if exist build\ui_webview.o del /Q build\ui_webview.o
	@if exist build\WebView2Loader.dll del /Q build\WebView2Loader.dll
	@if exist build\web rmdir /S /Q build\web
	@if exist build\testdata rmdir /S /Q build\testdata
