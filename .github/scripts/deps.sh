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

cat > /etc/apt/sources.list.d/llvm.sources <<EOF
deb https://apt.llvm.org/noble/ llvm-toolchain-noble-19 main
deb-src https://apt.llvm.org/noble/ llvm-toolchain-noble-19 main
Types: deb deb-src
URIs: https://apt.llvm.org/noble/
Suites: llvm-toolchain-noble-19
Components: main
Trusted: yes
Options: trusted=yes
EOF

test -f /usr/share/doc/kitware-archive-keyring/copyright ||
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | sudo tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null

CLANG_VERSION=19
for i in /usr/lib/llvm-*
do
  VER=$(basename $i | sed -e 's/llvm-//')
  if [[ $VER != "${CLANG_VERSION}" ]]
  then
    REMOVE="$REMOVE llvm-$VER clang-$VER"
  fi
done

apt update
apt remove $REMOVE
PACKAGES="cmake libc++-dev ninja-build python3-venv valgrind clang-${CLANG_VERSION} lld-${CLANG_VERSION} libclang-${CLANG_VERSION}-dev clang-tools-${CLANG_VERSION} clang-tidy-${CLANG_VERSION} clang-format-${CLANG_VERSION}"
if [[ $GITHUB_JOB == "build" ]]; then
    PACKAGES="${PACKAGES} gdisk mtools git qemu-system"
fi

echo "Installing packages: $PACKAGES"
apt install -y $PACKAGES

if [[ ! -f /usr/bin/ld.lld ]]
then
  ln -sv /usr/lib/llvm-18/bin/ld.lld /usr/bin/ld.lld
fi
