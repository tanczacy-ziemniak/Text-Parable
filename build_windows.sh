#!/bin/bash

# Check if MinGW is installed
if ! command -v x86_64-w64-mingw32-g++ &> /dev/null; then
    echo "MinGW cross-compiler not found. Installing with Homebrew..."
    brew install mingw-w64
fi

# Create Windows executable
echo "Building Windows executable..."
x86_64-w64-mingw32-g++ -std=c++11 text-parable.cpp -o text-parable.exe \
    -I/usr/local/opt/mingw-w64/include -L/usr/local/opt/mingw-w64/lib \
    -lpdcurses -static

if [ $? -eq 0 ]; then
    echo "Windows executable created successfully: text-parable.exe"
    echo "Note: You'll need to include PDCurses DLL with your .exe file"
else
    echo "Failed to create Windows executable"
fi
