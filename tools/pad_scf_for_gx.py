#!/usr/bin/env python3
"""Prepare textures referenced by a gxtexconv .scf for tile-aligned conversion.

Writes copies of referenced images and a rewritten .scf into --out-dir, leaving
original files untouched. Default behavior is stretch: copy the image verbatim
and let gxtexconv handle tile alignment. When an entry has resize="pad", the
image is padded with transparent pixels on the right/bottom so existing pixel
coordinates remain valid.
"""
import argparse, os, re, shutil, sys
from PIL import Image

# GX block sizes (width, height) by colfmt
BLOCK_SIZES = {
    0: (8, 8),   # I4
    1: (8, 4),   # I8
    2: (8, 4),   # IA4
    3: (4, 4),   # IA8
    4: (4, 4),   # RGB565
    5: (4, 4),   # RGB5A3
    6: (4, 4),   # RGBA8
    8: (8, 8),   # CI4
    9: (8, 4),   # CI8
    10: (8, 4),  # CI14X2? rare
    14: (8, 8),  # CMPR
}

def align(n, block):
    return ((n + block - 1) // block) * block

def pad_image(src, dst, bw, bh, method='stretch'):
    if method != 'pad':
        shutil.copy2(src, dst)
        return False
    img = Image.open(src).convert('RGBA')
    nw, nh = align(img.size[0], bw), align(img.size[1], bh)
    if (nw, nh) == img.size:
        shutil.copy2(src, dst)
        return False
    canvas = Image.new('RGBA', (nw, nh), (0, 0, 0, 0))
    canvas.paste(img, (0, 0))
    canvas.save(dst)
    return True

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('scf', help='input .scf file')
    ap.add_argument('-o', '--out-scf', required=True, help='output .scf path')
    ap.add_argument('-d', '--out-dir', required=True, help='directory for padded images')
    args = ap.parse_args()

    scf_path = os.path.abspath(args.scf)
    scf_dir = os.path.dirname(scf_path)
    out_dir = os.path.abspath(args.out_dir)
    os.makedirs(out_dir, exist_ok=True)

    with open(scf_path, 'r', encoding='utf-8', errors='replace') as f:
        content = f.read()

    # Match filepath="..." and optional colfmt=N in the same tag
    def repl(m):
        tag = m.group(0)
        fp_m = re.search(r'filepath="([^"]+)"', tag)
        if not fp_m:
            return tag
        rel = fp_m.group(1)
        src = rel if os.path.isabs(rel) else os.path.normpath(os.path.join(scf_dir, rel))
        colfmt_m = re.search(r'colfmt\s*=\s*(\d+)', tag)
        colfmt = int(colfmt_m.group(1)) if colfmt_m else 6
        resize_m = re.search(r'resize\s*=\s*"([^"]+)"', tag)
        method = resize_m.group(1).lower() if resize_m else 'stretch'
        if method != 'pad':
            method = 'stretch'
        bw, bh = BLOCK_SIZES.get(colfmt, (8, 8))
        base = os.path.basename(rel)
        dst = os.path.join(out_dir, base)
        changed = pad_image(src, dst, bw, bh, method)
        if changed:
            print(f'{method} {base}: {Image.open(src).size} -> {Image.open(dst).size}', file=sys.stderr)
        # Rewrite filepath to basename; rewritten .scf is placed in out_dir
        new_tag = tag.replace(f'filepath="{rel}"', f'filepath="{base}"', 1)
        return new_tag

    # Replace in <gxtexconv> / <texture> style tags; gxtexconv scf uses <filepath ... />
    new_content = re.sub(r'<[^>]*filepath="[^"]+"[^>]*/?>', repl, content)

    with open(args.out_scf, 'w', encoding='utf-8') as f:
        f.write(new_content)

if __name__ == '__main__':
    main()
