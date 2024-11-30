@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
cl -Zi win32_handmade.cpp user32.lib gdi32.lib
