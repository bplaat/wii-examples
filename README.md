# Bastiaan's Wii Examples

Some Wii homebrew C and Rust example programs

## Getting Started

Install devkitPro and its pacman package manager, then add the
[libogc2 package repository](https://github.com/extremscorner/pacman-packages#adding-repository).
Install the packages needed to build these examples:

```sh
dkp-pacman -Syu
dkp-pacman -S devkitPPC libogc2
```

## Triangles

A simple Hello World test program that draws some triangles with display lists.

## Canvas

A larger example that shows 2D rendering, runtime PNG decoding, font rendering, and cursor rendering.

## Blocks

A voxel example that generates a 64x64x64 world with caves, trees, cacti, a hut, and water. It draws exposed faces with GX display lists and block textures.
