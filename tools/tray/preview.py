# SPDX-FileCopyrightText: 2026 Ujudhammo
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
#
# DhammaKey - a DhammaWare project
# Homepage: https://dhammaware.org
#
# Copyright (C) 2026 Ujudhammo.
#
# TERMS OF USE AND NONCOMMERCIAL COMMITMENT:
# This software is offered as dana (a free gift) to serve the community, to
# support study and research, and to help spread the Dhamma. You may use, read,
# study, modify and contribute improvements to this source code for
# noncommercial purposes.
#
# PROHIBITED: copying, packaging, redistributing or using this source code or
# any derivative work for commercial purposes, for sale, or for any fee.
#
# The full, legally binding terms are in the LICENSE file at the project root.
# Third-party components keep their own licenses; see THIRD-PARTY-NOTICES.txt.
#
# THIS SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND. IN NO EVENT
# SHALL THE AUTHOR BE LIABLE FOR ANY DAMAGES ARISING FROM ITS USE.

import sys
from PIL import Image, ImageDraw, ImageFont
sys.path.insert(0,'.')
from make_tray import MODES
OUT='out'
fv=ImageFont.truetype('/usr/share/fonts/opentype/inter/Inter-Regular.otf',15)
fb=ImageFont.truetype('/usr/share/fonts/opentype/inter/Inter-SemiBold.otf',17)
sizes=[16,20,24,32]
rowh=80; colw=60
W=170+2*(len(sizes)*44+20+120)+40; H=70+len(MODES)*rowh
def sheet(setname):
    im=Image.new('RGB',(W,H),'#FFFFFF'); d=ImageDraw.Draw(im)
    d.text((20,18),f"Bộ {'chữ bản địa' if setname=='native' else 'chữ Latinh'} — thanh tác vụ sáng | thanh tác vụ tối  (16 · 20 · 24 · 32 px, và 16 px phóng 4 lần)",font=fb,fill='#2A0F24')
    for th_i,(theme,bg) in enumerate((('light','#EEEEEE'),('dark','#1F1F1F'))):
        x0=170+th_i*(len(sizes)*44+20+120+10)
        d.rectangle([x0-10,55,x0+len(sizes)*44+20+110,H-10],fill=bg)
        for r,m in enumerate(MODES):
            y=60+r*rowh
            if th_i==0: d.text((20,y+25),m[5],font=fv,fill='#333')
            x=x0
            for px in sizes:
                ic=Image.open(f"{OUT}/png/{setname}/dk-{m[0]}-{theme}-{px}.png").convert('RGBA')
                im.paste(ic,(x,y+40-px//2),ic); x+=44
            big=Image.open(f"{OUT}/png/{setname}/dk-{m[0]}-{theme}-16.png").convert('RGBA').resize((64,64),Image.NEAREST)
            im.paste(big,(x+10,y+8),big)
    im.save(f'preview-{setname}.png')
for s in ('native','latin'): sheet(s)
