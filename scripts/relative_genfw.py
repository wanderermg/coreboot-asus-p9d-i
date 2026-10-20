#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Keep GenFw CodeView DLL names independent of the checkout directory.

GenFw embeds its input filename in PE images, including space based on its
length. Run the GCC conversion from the input directory with a basename;
keep the output absolute and preserve every other conversion option.
Only the generated workspace configuration is changed, never EDK2 sources.
"""
from pathlib import Path
import sys

path = Path(sys.argv[1])
text = Path(sys.argv[2]).read_text()
start = text.index('[Dynamic-Library-File]')
end = text.index('[Dependency-Expression-File]', start)
block = text[start:end]
start_gcc = block.index('    <Command.GCC, Command.CLANGDWARF>')
end_gcc = block.index('    <Command.XCODE>', start_gcc)
rule = block[start_gcc:end_gcc]
old = '"$(GENFW)" -e $(MODULE_TYPE) -o ${dst} ${src} $(GENFW_FLAGS)'
new = 'cd "$(dir ${src})" && "$(GENFW)" -e $(MODULE_TYPE) -o ${dst} "$(notdir ${src})" $(GENFW_FLAGS)'
if rule.count(old) != 1:
    raise SystemExit('Unexpected EDK2 GCC conversion rule; refusing to modify it')
block = block[:start_gcc] + rule.replace(old, new) + block[end_gcc:]
path.write_text(text[:start] + block + text[end:])
