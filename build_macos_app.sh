#!/bin/bash

APP_NAME="TextParable"
APP_DIR="${APP_NAME}.app"
CONTENTS_DIR="${APP_DIR}/Contents"
MACOS_DIR="${CONTENTS_DIR}/MacOS"
RESOURCES_DIR="${CONTENTS_DIR}/Resources"

# Compile the application
echo "Compiling text-parable..."
g++ -std=c++11 text-parable.cpp -o text-parable -lncurses

if [ $? -ne 0 ]; then
    echo "Compilation failed!"
    exit 1
fi

# Create app bundle structure
echo "Creating app bundle structure..."
mkdir -p "${MACOS_DIR}"
mkdir -p "${RESOURCES_DIR}"

# Copy executable
cp text-parable "${MACOS_DIR}/${APP_NAME}"

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
    <key>NSMainNibFile</key>
    <string>MainMenu</string>
    <key>NSPrincipalClass</key>
    <string>NSApplication</string>
</dict>
</plist>
EOF

# Create a simple icon
# (for a real app, you would create and add a proper .icns file)
touch "${RESOURCES_DIR}/AppIcon.icns"

# Create a wrapper script to launch in Terminal
cat > "${MACOS_DIR}/launch.sh" << EOF
#!/bin/bash
open -a Terminal "\$(dirname "\$0")/${APP_NAME}"
EOF
chmod +x "${MACOS_DIR}/launch.sh"

echo "App bundle created at ${APP_DIR}"
echo "Note: This app will open in Terminal when launched."
