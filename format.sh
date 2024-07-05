#!/bin/bash

FORMAT_DIR=./Source

function format() {
    echo "formatting..."
    clang-format -i -style=file:.clang-format ${FORMAT_DIR}/**/*.cpp ${FORMAT_DIR}/**/*.h
    echo "-> done"
}

format