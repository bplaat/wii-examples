#!/bin/sh
set -eu

: "${DEVKITPPC:?DEVKITPPC must point to devkitPro/devkitPPC}"
: "${DEVKITPRO:?DEVKITPRO must point to devkitPro}"

output=
expect_output=false
for arg do
    if [ "$expect_output" = true ]; then
        output=$arg
        expect_output=false
    elif [ "$arg" = "-o" ]; then
        expect_output=true
    fi
done

"$DEVKITPPC/bin/powerpc-eabi-gcc" "$@"

if [ -n "$output" ]; then
    project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
    target_dir=${CARGO_TARGET_DIR:-"$project_dir/target"}
    case "$target_dir" in
        /*) ;;
        *) target_dir="$PWD/$target_dir" ;;
    esac
    target_root="$target_dir/wii"
    case "$output" in
        "$target_root"/*) profile=${output#"$target_root"/}; profile=${profile%%/*} ;;
        *) profile=release ;;
    esac
    profile_dir="$target_root/$profile"
    mkdir -p "$profile_dir"
    elf="$profile_dir/canvas.elf"
    dol="$profile_dir/canvas.dol"
    cp "$output" "$elf"
    "$DEVKITPRO/tools/bin/elf2dol" "$elf" "$dol"
fi
