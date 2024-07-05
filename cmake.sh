#!/bin/bash

FORMAT_DIR=./Source
DCMAKE_BUILD_TYPE=Debug

export CXX='ccache g++'

function format() {
    echo "formatting..."
    clang-format -i -style=file:.clang-format ${FORMAT_DIR}/**/*.cpp ${FORMAT_DIR}/**/*.h
    echo "-> done"
}

function is_cmake_configure_required() {
    # 初回ビルドのことも考慮して、buildディレクトリがない場合はtrueを返す
    if [ ! -d build ]; then
        return 0
    fi

    # build ディレクトリの更新日時が, CMakeLists.txt よりも古い場合は true を返す
    # stat コマンドを使用して更新日時を取得する
    build_mtime=$(stat -c %Y build)
    cmakelists_mtime=$(stat -c %Y CMakeLists.txt)

    if [ "$build_mtime" -lt "$cmakelists_mtime" ]; then
        return 0
    fi

    return 1
}

function build() {
    echo "building..."
    if is_cmake_configure_required; then
        echo "CMakeLists.txt has changed, reconfiguring..."
        cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=${DCMAKE_BUILD_TYPE}
    fi
    cmake --build build
    echo "-> done"
}

format
build