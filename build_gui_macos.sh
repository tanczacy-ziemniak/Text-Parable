#!/bin/bash

echo "Building Text Parable GUI for macOS"

# Check for Apple Silicon (ARM) vs Intel Mac
ARCH=$(uname -m)
if [ "$ARCH" = "arm64" ]; then
    echo "Detected Apple Silicon (ARM) Mac"
    # Make sure we have Homebrew
    if ! command -v brew &> /dev/null; then
        echo "Homebrew not found. Please install it first: https://brew.sh"
        exit 1
    fi
    
    # Make sure SDL2 and SDL2_ttf are installed
    if ! brew list sdl2 &> /dev/null; then
        echo "Installing SDL2..."
        brew install sdl2
    fi
    
    if ! brew list sdl2_ttf &> /dev/null; then
        echo "Installing SDL2_ttf..."
        brew install sdl2_ttf
    fi
    
    # Compile with ARM-specific flags
    echo "Compiling with ARM-specific settings..."
    g++ -std=c++14 text-parable-gui.cpp -o text-parable-gui \
        -I/opt/homebrew/include \
        -L/opt/homebrew/lib \
        -lSDL2 -lSDL2_ttf \
        -DSDL_DISABLE_IMMINTRIN_H \
        -D_THREAD_SAFE
else
    echo "Detected Intel Mac"
    # Make sure we have Homebrew
    if ! command -v brew &> /dev/null; then
        echo "Homebrew not found. Please install it first: https://brew.sh"
        exit 1
    fi
    
    # Make sure SDL2 and SDL2_ttf are installed
    if ! brew list sdl2 &> /dev/null; then
        echo "Installing SDL2..."
        brew install sdl2
    fi
    
    if ! brew list sdl2_ttf &> /dev/null; then
        echo "Installing SDL2_ttf..."
        brew install sdl2_ttf
    fi
    
    # Compile with Intel-specific flags
    echo "Compiling with Intel Mac settings..."
    g++ -std=c++14 text-parable-gui.cpp -o text-parable-gui \
        -I/usr/local/include \
        -L/usr/local/lib \
        -lSDL2 -lSDL2_ttf \
        -D_THREAD_SAFE
fi

if [ $? -eq 0 ]; then
    echo "Compilation successful! Run './text-parable-gui' to start the game."
    
    # Create a simple .app bundle for easier launching
    echo "Creating app bundle..."
    
    APP_NAME="TextParable"
    APP_DIR="${APP_NAME}.app"
    CONTENTS_DIR="${APP_DIR}/Contents"
    MACOS_DIR="${CONTENTS_DIR}/MacOS"
    RESOURCES_DIR="${CONTENTS_DIR}/Resources"
    
    mkdir -p "${MACOS_DIR}"
    mkdir -p "${RESOURCES_DIR}"
    
    # Copy executable
    cp text-parable-gui "${MACOS_DIR}/${APP_NAME}"
    
    # Copy any font file if it exists
    if [ -f "font.ttf" ]; then
        cp font.ttf "${RESOURCES_DIR}/"
    fi
    
    # Create Info.plist
    cat > "${CONTENTS_DIR}/Info.plist" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>${APP_NAME}</string>
    <key>CFBundleIconFile</key>
    <string>AppIcon</string>
    <key>CFBundleIdentifier</key>
    <string>com.example.TextParable</string>
    <key>CFBundleInfoDictionaryVersion</key>
    <string>6.0</string>
    <key>CFBundleName</key>
    <string>${APP_NAME}</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleShortVersionString</key>
    <string>1.0</string>
    <key>CFBundleVersion</key>
    <string>1</string>
    <key>NSHighResolutionCapable</key>
    <true/>
    <key>LSMinimumSystemVersion</key>
    <string>10.14</string>
</dict>
</plist>
EOF
    
    touch "${RESOURCES_DIR}/AppIcon.icns"
    
    echo "App bundle created at ${APP_DIR}"
    echo "You can now run the app by double-clicking it or using: open ${APP_DIR}"
else
    echo "Compilation failed."
fi
