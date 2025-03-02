#!/bin/bash

# Compile text-parable with ncurses
g++ -std=c++11 text-parable.cpp -o text-parable -lncurses

# Check if compilation was successful
if [ $? -eq 0 ]; then
    echo "Compilation successful!"
    echo "Run the program with: ./text-parable"
else
    echo "Compilation failed. Trying alternative method with explicit library path..."
    g++ -std=c++11 text-parable.cpp -o text-parable -L/usr/lib -lncurses
    
    if [ $? -eq 0 ]; then
        echo "Compilation successful with explicit library path!"
        echo "Run the program with: ./text-parable"
    else
        echo "Compilation failed. Please check your ncurses installation."
    fi
fi
