#!/usr/bin/env bash

baseURL="http://download.huzheng.org/dict.org"
targetPackage="stardict-dictd_www.dict.org_wn-2.4.2.tar.bz2"

# check if any downloader exist
if command -v wget >/dev/null 2>&1; then
    downloader="wget"
elif command -v curl >/dev/null 2>&1; then
    downloader="curl"
else
    echo "curl and wget not found, install one of them first; e.g.  sudo apt install wget -y" >&2
    exit 1
fi

mkdir -p "$HOME/.local/share/stardict/dic/"

# export the environment variant
shell_name=$(basename "$SHELL" 2>/dev/null || echo "")
if [ "$shell_name" = "bash" ]; then
    echo 'export STARDICT_DATA_DIR=$HOME/.local/share/stardict' >>~/.bashrc
elif [ "$shell_name" = "zsh" ]; then
    echo 'export STARDICT_DATA_DIR=$HOME/.local/share/stardict' >>~/.zshrc
fi

# for current shell environment
export STARDICT_DATA_DIR=$HOME/.local/share/stardict

# download the directory
if [ "$downloader" = "wget" ]; then
    wget "$baseURL/$targetPackage"
else
    curl -O "$baseURL/$targetPackage"
fi

# unpack
tar xvf "$targetPackage" -C "$STARDICT_DATA_DIR/dic/"

echo "testing command: sdcv -l"
sdcv -l

echo "testing command: sdcv -c test"
sdcv -c test

echo "use command for more details: man sdcv"
