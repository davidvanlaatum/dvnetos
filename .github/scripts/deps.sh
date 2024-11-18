#!/usr/bin/env bash

set -e

env

cat > /etc/apt/sources.list.d/kitware.sources <<EOF
Types: deb
URIs: https://apt.kitware.com/ubuntu/
Suites: noble
Components: main
Signed-By: /usr/share/keyrings/kitware-archive-keyring.gpg
EOF

test -f /usr/share/doc/kitware-archive-keyring/copyright ||
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | sudo tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null

apt update
apt remove clang-16 clang-17
PACKAGES="cmake libc++-dev ninja-build python3-venv gcc-arm-none-eabi gcc-x86-64-linux-gnu valgrind"
if [[ $MATRIX_CLANG == "ON" ]]; then
    PACKAGES="${PACKAGES} clang lld libclang-dev"
fi
if [[ $GITHUB_JOB == "build" ]]; then
    PACKAGES="${PACKAGES} gdisk mtools git qemu-system"
fi

echo "Installing packages: $PACKAGES"
apt install -y $PACKAGES
