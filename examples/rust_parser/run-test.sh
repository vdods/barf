#!/bin/sh
# 2026.10.02 - Copyright Daniel Palm - Licensed under Apache 2.0

set -eu
repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
example_dir="$repo_root/examples/rust_parser"
build_dir="$repo_root/build/rust_parser"
mkdir -p "$build_dir"
cp "$example_dir/main.rs" "$build_dir/main.rs"
"$repo_root/build/bin/trison" --clear-targets-search-path --include-targets-search-path "$repo_root/targets" --without-line-directives --output-directory "$build_dir" "$example_dir/parser.trison"
rustc --edition 2024 --deny warnings "$build_dir/main.rs" -o "$build_dir/rust_parser_test"
"$build_dir/rust_parser_test"
