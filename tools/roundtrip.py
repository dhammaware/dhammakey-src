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

# Sinh chuỗi phím Telex/VNI từ văn bản tiếng Việt, gõ lại bằng dkdemo, so sánh.
import sys, subprocess, unicodedata as ud
TONES = {'́':('s','1'),'̀':('f','2'),'̉':('r','3'),'̃':('x','4'),'̣':('j','5')}
def encode(word, vni=False):
    keys=''; tone=''
    for ch in ud.normalize('NFD', word):
        if ch in TONES: tone = TONES[ch][1 if vni else 0]; continue
        if ch == '̂': keys += '6' if vni else keys[-1]; continue
        if ch == '̆': keys += '8' if vni else 'w'; continue
        if ch == '̛': keys += '7' if vni else 'w'; continue
        if ch in 'đĐ': keys += ch.replace('đ','d').replace('Đ','D') + ('9' if vni else 'd'); continue
        keys += ch
    return keys + tone
text = open(sys.argv[1], encoding='utf-8').read().split()
bad = 0
for vni in (False, True):
    keys = ' '.join(encode(w.strip('.,;:!?"()'), vni) for w in text)
    exp  = ' '.join(w.strip('.,;:!?"()') for w in text)
    out = subprocess.run(['build/dkdemo', 'vni' if vni else 'telex', keys], capture_output=True, text=True).stdout.strip()
    for a,b,k in zip(out.split(), exp.split(), keys.split()):
        if a != b: bad += 1; print(('VNI ' if vni else 'Telex ') + f'{k} -> {a} (cần {b})')
    print(('VNI' if vni else 'Telex'), 'số từ:', len(exp.split()))
print('Sai:', bad)
