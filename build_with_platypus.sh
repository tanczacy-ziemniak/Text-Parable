#!/bin/bash

# Check if Platypus is installed
if ! command -v platypus &> /dev/null; then
    echo "Platypus not found. Installing with Homebrew..."
    brew install --cask platypus
fi

# Compile the program
g++ -std=c++11 text-parable.cpp -o text-parable -lncurses

if [ $? -ne 0 ]; then
    echo "Compilation failed!"
    exit 1
fi

# Create a simple wrapper script
cat > run_text_parable.sh << EOF
#!/bin/bash
cd "\$(dirname "\$0")"
./text-parable
EOF
chmod +x run_text_parable.sh

# Create the app bundle using Platypus
platypus --app-icon /System/Library/CoreServices/CoreTypes.bundle/Contents/Resources/GenericApplicationIcon.icns \
    --name "TextParable" \
    --interface-type "Terminal" \
    --app-version "1.0" \
    --author "Developer" \
    --bundle-identifier "com.example.textparable" \
    --background-only NO \
    --quit-after-execution NO \
    --optimize-nib YES \
    --overwrite YES \
    run_text_parable.sh \
    TextParable.app

# Copy the executable to the app bundle
cp text-parable TextParable.app/Contents/Resources/

echo "App bundle created: TextParable.app"
