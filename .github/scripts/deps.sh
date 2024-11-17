#!/usr/bin/env bash

set -e

env

apt update
apt remove clang-16 clang-17
PACKAGES="libc++-dev ninja-build python3-venv gcc-arm-none-eabi gcc-x86-64-linux-gnu"
if [[ $MATRIX_CLANG == "ON" ]]; then
    PACKAGES="${PACKAGES} clang lld libclang-dev"
fi
if [[ $GITHUB_JOB == "build" ]]; then
    PACKAGES="${PACKAGES} gdisk mtools git qemu-system"
fi

apt install -y $PACKAGES
