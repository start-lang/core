#!/usr/bin/env python3

import re
import sys

if len(sys.argv) != 2:
    print("Usage: bf_to_start.py <file>")
    sys.exit(1)

with open(sys.argv[1], "r") as f:
    code = f.read()

# find "# init:(.*)"
init = re.search(r"# init:(.*)", code)
code = re.sub(r"#.*", "", code)
code = code.replace("\n", "")
code = code.replace(" ", "")
out = ""
if init:
    out += init.group(1).strip()
    print("init: " + out, file=sys.stderr)
print(code, file=sys.stderr);
last = "";
count = 1
for c in code:
    if c == last:
        count += 1
    elif last:
        if count > 1 and last in "<>":
            out += str(count) + last
        elif count > 0 and last in "+-":
            out += str(count) + last
        else:
            out += last*count
        count = 1
    last = c
out += last
out = out.replace("<", "<")
out = out.replace(">", ">")
out = out.replace("-", "-")
out = out.replace("+", "+")
out = out.replace("[", "??[")
out = out.replace("]", "??]")
out = out.replace(".", " PRINT ")
out = out.replace(",", "10!")
out = out
r = -100 + len(out) / len(code) * 100
rc = len(code) - len(out)
print(f'increase: {r:.2f}% ({len(code)} + {-rc} chars)', file=sys.stderr)
print(out, end="")
