#!/usr/bin/env python3
"""Render UxAir PNG launcher assets from assets/uxair-icon.svg.

Requires Pillow only when regenerating artwork; it is not a build/runtime dependency.
The small supported SVG subset here covers the shapes used in the source artwork.
"""
import re
import xml.etree.ElementTree as ET
from pathlib import Path

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parent.parent
SOURCE = ROOT / 'assets/uxair-icon.svg'
OUTPUTS = {
    80: ROOT / 'packaging/icon.png',
    130: ROOT / 'packaging/largeIcon.png',
    256: ROOT / 'assets/uxair-icon-256.png',
    512: ROOT / 'assets/uxair-icon-512.png',
}
SUPERSAMPLE = 8


def color(value):
    if not value or value == 'none':
        return None
    value = value.lstrip('#')
    return tuple(int(value[i:i + 2], 16) for i in (0, 2, 4)) + (255,)


def path_points(data):
    tokens = re.findall(r'[MmLlHhZz]|-?\d+(?:\.\d+)?', data)
    points, i, command, current, start = [], 0, None, (0.0, 0.0), None
    while i < len(tokens):
        if tokens[i].isalpha():
            command = tokens[i]
            i += 1
            if command in 'Zz':
                if start is not None:
                    points.append(start)
                    current = start
                continue
        if command in 'MmLl':
            x, y = float(tokens[i]), float(tokens[i + 1])
            i += 2
            if command.islower():
                x, y = current[0] + x, current[1] + y
            current = (x, y)
            points.append(current)
            if command in 'Mm':
                start = current
                command = 'l' if command == 'm' else 'L'
        elif command in 'Hh':
            x = float(tokens[i])
            i += 1
            current = (current[0] + x if command == 'h' else x, current[1])
            points.append(current)
        else:
            raise ValueError(f'Unsupported path command {command!r} in {data!r}')
    return points


def render(size):
    scale = size * SUPERSAMPLE / 512
    canvas = Image.new('RGBA', (size * SUPERSAMPLE, size * SUPERSAMPLE), (0, 0, 0, 0))
    draw = ImageDraw.Draw(canvas)
    root = ET.parse(SOURCE).getroot()
    for shape in root:
        tag = shape.tag.rsplit('}', 1)[-1]
        attrs = shape.attrib
        fill = color(attrs.get('fill'))
        stroke = color(attrs.get('stroke'))
        stroke_width = float(attrs.get('stroke-width', '0')) * scale
        if tag == 'rect':
            x, y = float(attrs.get('x', 0)), float(attrs.get('y', 0))
            x2, y2 = x + float(attrs['width']), y + float(attrs['height'])
            half = float(attrs.get('stroke-width', '0')) / 2
            box = ((x - half) * scale, (y - half) * scale,
                   (x2 + half) * scale, (y2 + half) * scale)
            radius = (float(attrs.get('rx', 0)) + half) * scale
            if fill:
                draw.rounded_rectangle(box, radius=radius, fill=fill)
            if stroke:
                draw.rounded_rectangle(box, radius=radius, outline=stroke,
                                       width=max(1, round(stroke_width)))
        elif tag == 'path':
            pts = [(x * scale, y * scale) for x, y in path_points(attrs['d'])]
            if fill:
                draw.polygon(pts, fill=fill)
            if stroke:
                draw.line(pts, fill=stroke, width=max(1, round(stroke_width)),
                          joint='curve')
                if attrs.get('stroke-linecap') == 'round':
                    radius = stroke_width / 2
                    for x, y in (pts[0], pts[-1]):
                        draw.ellipse((x - radius, y - radius, x + radius, y + radius), fill=stroke)
        else:
            raise ValueError(f'Unsupported SVG shape: {tag}')
    return canvas.resize((size, size), Image.Resampling.LANCZOS)


def main():
    for size, path in OUTPUTS.items():
        path.parent.mkdir(parents=True, exist_ok=True)
        render(size).save(path, format='PNG', optimize=True)
        print(f'Wrote {path.relative_to(ROOT)} ({size}x{size})')


if __name__ == '__main__':
    main()
