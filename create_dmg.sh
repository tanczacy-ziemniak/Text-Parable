#!/bin/bash

# Create a DMG file for distribution
hdiutil create -volname "TextParable" -srcfolder TextParable.app -ov -format UDZO TextParable.dmg

echo "DMG installer created: TextParable.dmg"
