#!/bin/sh
set -eu

root_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
destination="$root_dir/third_party/llama.cpp"
repository="https://github.com/ggml-org/llama.cpp.git"
revision="9e99f65173e88dcdaab24011ab820c131ec4abfe"

if [ -e "$destination" ]; then
    echo "Refusing to overwrite existing path: $destination" >&2
    exit 1
fi

git clone "$repository" "$destination"
git -C "$destination" checkout --detach "$revision"
git -C "$destination" submodule update --init --recursive
printf 'llama.cpp installed at %s\n' "$revision"
