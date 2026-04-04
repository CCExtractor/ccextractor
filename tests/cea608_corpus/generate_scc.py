#!/usr/bin/env python3
"""
Generate 3 CEA-608 SCC test files for CCExtractor testing:
  1. popon_basic.scc   - Basic pop-on captions (ENM, RCL, PAC, text, EDM, EOC)
  2. rollup_basic.scc   - Roll-up captions (RU2/RU3, CR, PAC, text)
  3. special_chars.scc   - Special & extended characters (musical note, accented, etc.)

Reference: mcpoodle SCC format doc, CEA-608 spec
All control codes are doubled for redundancy per broadcast convention.
"""

# CEA-608 odd parity table: maps 7-bit value (0x00-0x7F) to 8-bit parity byte
PARITY = [
    0x80, 0x01, 0x02, 0x83, 0x04, 0x85, 0x86, 0x07,
    0x08, 0x89, 0x8a, 0x0b, 0x8c, 0x0d, 0x0e, 0x8f,
    0x10, 0x91, 0x92, 0x13, 0x94, 0x15, 0x16, 0x97,
    0x98, 0x19, 0x1a, 0x9b, 0x1c, 0x9d, 0x9e, 0x1f,
    0x20, 0xa1, 0xa2, 0x23, 0xa4, 0x25, 0x26, 0xa7,
    0xa8, 0x29, 0x2a, 0xab, 0x2c, 0xad, 0xae, 0x2f,
    0xb0, 0x31, 0x32, 0xb3, 0x34, 0xb5, 0xb6, 0x37,
    0x38, 0xb9, 0xba, 0x3b, 0xbc, 0x3d, 0x3e, 0xbf,
    0x40, 0xc1, 0xc2, 0x43, 0xc4, 0x45, 0x46, 0xc7,
    0xc8, 0x49, 0x4a, 0xcb, 0x4c, 0xcd, 0xce, 0x4f,
    0xd0, 0x51, 0x52, 0xd3, 0x54, 0xd5, 0xd6, 0x57,
    0x58, 0xd9, 0xda, 0x5b, 0xdc, 0x5d, 0x5e, 0xdf,
    0xe0, 0x61, 0x62, 0xe3, 0x64, 0xe5, 0xe6, 0x67,
    0x68, 0xe9, 0xea, 0x6b, 0xec, 0x6d, 0x6e, 0xef,
    0x70, 0xf1, 0xf2, 0x73, 0xf4, 0x75, 0x76, 0xf7,
    0xf8, 0x79, 0x7a, 0xfb, 0x7c, 0xfd, 0xfe, 0x7f,
]

def p(ch):
    """Apply odd parity to a 7-bit ASCII character."""
    if isinstance(ch, str):
        ch = ord(ch)
    return PARITY[ch & 0x7F]

def text_to_words(text):
    """Convert ASCII text to SCC hex words (2 chars per word, with parity)."""
    words = []
    i = 0
    while i < len(text):
        c1 = p(text[i])
        if i + 1 < len(text):
            c2 = p(text[i + 1])
            i += 2
        else:
            c2 = 0x80  # filler
            i += 1
        words.append(f"{c1:02x}{c2:02x}")
    return words

def fmt_tc(h, m, s, f):
    """Format non-drop-frame timecode."""
    return f"{h:02d}:{m:02d}:{s:02d}:{f:02d}"

def write_scc(filename, entries):
    """Write an SCC file. entries = [(timecode_str, [hex_words]), ...]"""
    with open(filename, 'w') as f:
        f.write("Scenarist_SCC V1.0\n")
        f.write("\n")
        for tc, words in entries:
            f.write(f"{tc}\t{' '.join(words)}\n")
            f.write("\n")
    print(f"  Created: {filename}")


# ── Control code constants (already include parity) ──
ENM  = "94ae"  # Erase Non-displayed Memory
RCL  = "9420"  # Resume Caption Loading (pop-on mode)
EDM  = "942c"  # Erase Displayed Memory
EOC  = "942f"  # End of Caption (flip buffers)
FILL = "8080"  # Filler / null

# Roll-up commands
RU2  = "9425"  # Roll-Up 2 rows
RU3  = "94a7"  # Roll-Up 3 rows
CR   = "94ad"  # Carriage Return

# PAC codes (Row, Column 0, White)
PAC_R14_C0 = "9470"  # Row 14 (commonly used bottom row), col 0, white
PAC_R15_C0 = "94d0"  # Row 15, col 0, white
PAC_R13_C0 = "1370"  # Row 13, col 0, white
PAC_R12_C0 = "13d0"  # Row 12, col 0, white
PAC_R11_C0 = "9770"  # Row 11 (used for 3-line rollup base)

# ═══════════════════════════════════════════════════════
# TEST 1: BASIC POP-ON
# Tests: ENM, RCL, PAC positioning, text encoding,
#        EDM (clear), EOC (display), multi-line captions
# ═══════════════════════════════════════════════════════
print("Generating popon_basic.scc ...")

popon_entries = []

# Caption 1: "HELLO WORLD" at row 15 (single line)
words = [ENM, ENM, RCL, RCL, PAC_R15_C0, PAC_R15_C0]
words += text_to_words("HELLO WORLD")
words += [EDM, EDM, FILL, FILL, EOC, EOC]
popon_entries.append((fmt_tc(0, 0, 1, 0), words))

# Clear at 00:00:03:00
popon_entries.append((fmt_tc(0, 0, 3, 0), [EDM, EDM]))

# Caption 2: Two-line pop-on
# Line 1 at row 14: "THIS IS A TEST"
# Line 2 at row 15: "OF POP-ON CC"
words = [ENM, ENM, RCL, RCL]
words += [PAC_R14_C0, PAC_R14_C0]
words += text_to_words("THIS IS A TEST")
words += [PAC_R15_C0, PAC_R15_C0]
words += text_to_words("OF POP-ON CC")
words += [EDM, EDM, FILL, FILL, EOC, EOC]
popon_entries.append((fmt_tc(0, 0, 5, 0), words))

# Clear at 00:00:08:00
popon_entries.append((fmt_tc(0, 0, 8, 0), [EDM, EDM]))

# Caption 3: ">> SPEAKER CHANGE" (standard CC convention)
words = [ENM, ENM, RCL, RCL, PAC_R15_C0, PAC_R15_C0]
words += text_to_words(">> NEW SPEAKER")
words += [EDM, EDM, FILL, FILL, EOC, EOC]
popon_entries.append((fmt_tc(0, 0, 10, 0), words))

# Clear
popon_entries.append((fmt_tc(0, 0, 13, 0), [EDM, EDM]))

write_scc("popon_basic.scc", popon_entries)


# ═══════════════════════════════════════════════════════
# TEST 2: ROLL-UP
# Tests: RU2/RU3 mode, CR (carriage return to scroll),
#        PAC for base row, continuous text flow
# ═══════════════════════════════════════════════════════
print("Generating rollup_basic.scc ...")

rollup_entries = []

# Start 2-row roll-up mode, position at row 15
words = [RU2, RU2, PAC_R15_C0, PAC_R15_C0]
words += text_to_words("FIRST LINE")
rollup_entries.append((fmt_tc(0, 0, 1, 0), words))

# CR + second line (scrolls first line up)
words = [CR, CR, PAC_R15_C0, PAC_R15_C0]
words += text_to_words("SECOND LINE")
rollup_entries.append((fmt_tc(0, 0, 3, 0), words))

# CR + third line (first line disappears in RU2)
words = [CR, CR, PAC_R15_C0, PAC_R15_C0]
words += text_to_words("THIRD LINE")
rollup_entries.append((fmt_tc(0, 0, 5, 0), words))

# Switch to 3-row roll-up
words = [RU3, RU3, CR, CR, PAC_R15_C0, PAC_R15_C0]
words += text_to_words("NOW THREE ROWS")
rollup_entries.append((fmt_tc(0, 0, 7, 0), words))

# Another CR
words = [CR, CR, PAC_R15_C0, PAC_R15_C0]
words += text_to_words("STILL ROLLING")
rollup_entries.append((fmt_tc(0, 0, 9, 0), words))

# Clear with EDM
rollup_entries.append((fmt_tc(0, 0, 12, 0), [EDM, EDM]))

write_scc("rollup_basic.scc", rollup_entries)


# ═══════════════════════════════════════════════════════
# TEST 3: SPECIAL & EXTENDED CHARACTERS
# Tests: Special chars (1130-113f range), extended chars
#        (1220-123f, 1320-133f ranges), music note, etc.
#
# CEA-608 Special Characters (Channel 1):
#   1130 = ®    1131 = °    1132 = ½    1133 = ¿
#   1134 = ™    1135 = ¢    1136 = £    1137 = ♪ (music note)
#   1138 = à    1139 = (transparent space)
#   113a = è    113b = â    113c = ê    113d = î
#   113e = ô    113f = û
#
# CEA-608 Extended Spanish/French (Channel 1):
#   1220 = Á    1221 = É    1222 = Ó    1223 = Ú
#   1224 = Ü    1225 = ü    1226 = '    1227 = ¡
#   1228 = *    1229 = '    122a = —    122b = ©
#   122c = ℠    122d = •    122e = "    122f = "
#   1230 = À    1231 = Â    1232 = Ç    1233 = È
#   1234 = Ê    1235 = Ë    1236 = ë    1237 = Î
#   1238 = Ï    1239 = ï    123a = Ô    123b = Ù
#   123c = ù    123d = Û    123e = «    123f = »
# ═══════════════════════════════════════════════════════
print("Generating special_chars.scc ...")

special_entries = []

# Caption 1: Music notes ♪ around text (common CC pattern)
# ♪ = 1137
words = [ENM, ENM, RCL, RCL, PAC_R15_C0, PAC_R15_C0]
words += ["1137", "1137"]  # ♪ (special chars are doubled like control codes)
words += text_to_words("SINGING")
words += ["1137", "1137"]  # ♪
words += [EDM, EDM, FILL, FILL, EOC, EOC]
special_entries.append((fmt_tc(0, 0, 1, 0), words))

# Clear
special_entries.append((fmt_tc(0, 0, 4, 0), [EDM, EDM]))

# Caption 2: Spanish chars - Á É Ó Ú ¿ ¡
# ¿ = 1133, ¡ = 1227
# Á = 1220, É = 1221, Ó = 1222, Ú = 1223
words = [ENM, ENM, RCL, RCL, PAC_R14_C0, PAC_R14_C0]
words += ["1227", "1227"]  # ¡  (replaces prev char position)
words += text_to_words("QU")
words += ["1221", "1221"]  # É
words += text_to_words(" TAL")
words += ["1133", "1133"]  # ¿
words += [PAC_R15_C0, PAC_R15_C0]
words += text_to_words("CAMI")
words += ["1222", "1222"]  # Ó  (replaces prev char - the I)
words += text_to_words("N")
special_entries.append(("", []))  # placeholder
# Actually let me redo this more carefully
# Extended chars REPLACE the previous character on the current row
# So we need to be careful about placement

# Let me restructure:
# Row 14: "♪ CAFÉ ♪"
# Row 15: "RÉSUMÉ ©2024"
special_entries = []  # reset

# Caption 1: Music notes  ♪ SINGING ♪
words = [ENM, ENM, RCL, RCL, PAC_R15_C0, PAC_R15_C0]
words += ["1137", "1137"]  # ♪
words += text_to_words(" SINGING ")
words += ["1137", "1137"]  # ♪
words += [EDM, EDM, FILL, FILL, EOC, EOC]
special_entries.append((fmt_tc(0, 0, 1, 0), words))

special_entries.append((fmt_tc(0, 0, 4, 0), [EDM, EDM]))

# Caption 2: Special symbols  ® ™ ©
# ® = 1130, ™ = 1134, © = 122b
words = [ENM, ENM, RCL, RCL, PAC_R14_C0, PAC_R14_C0]
words += text_to_words("BRAND")
words += ["1134", "1134"]  # ™ (replaces prev char 'D')
# Hmm, extended chars replace prev char. Let me use a different approach.
# Actually, SPECIAL chars (11xx) do NOT replace - they just insert.
# Only EXTENDED chars (12xx, 13xx) replace the previous character.
# Special chars occupy one character position.

# Let me be precise:
# Special chars (1130-113f): Insert at current position, take 1 col
# Extended chars (1220-123f, 1320-133f): REPLACE previous character

# Caption 2: "BRAND® Recipe™"
words = [ENM, ENM, RCL, RCL, PAC_R15_C0, PAC_R15_C0]
words += text_to_words("BRAND")
words += ["1130", "1130"]  # ® (special - inserts)
words += text_to_words(" RECIPE")
words += ["1134", "1134"]  # ™ (special - inserts)
words += [EDM, EDM, FILL, FILL, EOC, EOC]
special_entries.append((fmt_tc(0, 0, 5, 0), words))

special_entries.append((fmt_tc(0, 0, 8, 0), [EDM, EDM]))

# Caption 3: French/Spanish extended chars
# "CAFÉ" - we need: C A F then É (extended 1221 replaces prev char)
# So write "CAFE" then 1221 replaces E with É
# "RÉSUMÉ" - R then 1221(É replaces R? no...)
# Actually extended replaces the LAST character written.
# So: write "CAFE" -> screen shows "CAFE", then 1221 -> "CAFÈ"
# Wait, 1221 = É. Extended chars replace last char with the extended char.
# So text "CAFE" then 1221,1221 -> "CAFÉ"  ✓

# Let's do: "CAFÉ" on row 14, "NAÏVE" on row 15
# CAFÉ: text_to_words("CAFE") + ["1221", "1221"]  (É replaces E)
# NAÏVE: text_to_words("NAI") + ["1238", "1238"] (Ï replaces I) + text_to_words("VE")
# Wait 1238 = Ï? Let me check: 1238 = Ï yes

words = [ENM, ENM, RCL, RCL, PAC_R14_C0, PAC_R14_C0]
words += text_to_words("CAFE")
words += ["1221", "1221"]  # É replaces E -> CAFÉ
words += [PAC_R15_C0, PAC_R15_C0]
words += text_to_words("NAI")
words += ["1238", "1238"]  # Ï replaces I -> NAÏ
words += text_to_words("VE")
words += [EDM, EDM, FILL, FILL, EOC, EOC]
special_entries.append((fmt_tc(0, 0, 9, 0), words))

special_entries.append((fmt_tc(0, 0, 12, 0), [EDM, EDM]))

# Caption 4: Music note + accented together  ♪ À LA MODE ♪
# à = 1138 (special, inserts), À = 1230 (extended, replaces)
# Use: write "A" then 1230,1230 -> À
words = [ENM, ENM, RCL, RCL, PAC_R15_C0, PAC_R15_C0]
words += ["1137", "1137"]  # ♪
words += text_to_words(" A")
words += ["1230", "1230"]  # À replaces A
words += text_to_words(" LA MODE ")
words += ["1137", "1137"]  # ♪
words += [EDM, EDM, FILL, FILL, EOC, EOC]
special_entries.append((fmt_tc(0, 0, 13, 0), words))

special_entries.append((fmt_tc(0, 0, 16, 0), [EDM, EDM]))

write_scc("special_chars.scc", special_entries)

print("\nDone! Generated 3 SCC files.")
print("\nExpected SRT output:")
print("─" * 40)
print("popon_basic.scc:")
print("  1) HELLO WORLD")
print("  2) THIS IS A TEST / OF POP-ON CC")
print("  3) >> NEW SPEAKER")
print()
print("rollup_basic.scc:")
print("  Roll-up lines: FIRST LINE, SECOND LINE, THIRD LINE,")
print("  NOW THREE ROWS, STILL ROLLING")
print()
print("special_chars.scc:")
print("  1) ♪ SINGING ♪")
print("  2) BRAND® RECIPE™")
print("  3) CAFÉ / NAÏVE")
print("  4) ♪ À LA MODE ♪")
