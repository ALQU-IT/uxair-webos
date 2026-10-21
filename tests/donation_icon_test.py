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
assert 'TTF_SizeUTF8(font_,rows[i].label.c_str(),nullptr,&title_h)' in shell
assert 'TTF_SizeUTF8(small_,rows[i].note.c_str(),nullptr,&note_h)' in shell
assert 'icon_size=std::max(48,std::min(68,text_height))' in shell
assert 'icon_y=text_top+(text_height-icon_size)/2' in shell
assert 'label_x=donation_row?panel+18+icon_size+16:panel+18' in shell
assert 'SDL_Rect icon={panel+18,icon_y,icon_size,icon_size}' in shell
print('PASS: exact supplied square coffee art is preserved and sized from the About text block in one SDL texture')
