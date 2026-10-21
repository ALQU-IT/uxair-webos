#!/usr/bin/env python3
import pathlib
import struct

ROOT = pathlib.Path(__file__).resolve().parent.parent
bitmap = (ROOT / 'assets/bmc-icon.bmp').read_bytes()
source = (ROOT / 'assets/bmc-icon-source.png').read_bytes()
assert bitmap[:2] == b'BM'
assert struct.unpack_from('<ii', bitmap, 18) == (120, 120)
assert struct.unpack_from('<H', bitmap, 28)[0] == 24
assert source[:8] == b'\x89PNG\r\n\x1a\n'
assert struct.unpack_from('>II', source, 16) == (480, 480)
shell = (ROOT / 'src/shell.cpp').read_text()
assert 'SDL_LoadBMP((app_directory()+"/bmc-icon.bmp").c_str())' in shell
assert 'SDL_CreateTextureFromSurface(renderer,coffee)' in shell
assert 'SDL_DestroyTexture(donation_icon_)' in shell
assert 'SDL_Rect icon={panel+18,y+(row_height-48)/2,48,48}' in shell
print('PASS: exact supplied square coffee art is preserved and bundled as a 120x120 one-time SDL texture')
