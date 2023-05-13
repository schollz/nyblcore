import json
import re
import math
import random

data = json.load(open("classic.json", "rb"))


def bb_converter(bbt, i):
    txt = bbt[0]
    digits = [int(s) for s in re.findall(r"\b\d+\b", txt)]
    for digit in digits:
        if digit > 128:
            return False
    if "x" in txt:
        return False
    if "." in txt:
        return False
    # print(digits)
    # print(",".join([str(x) for x in digits]))
    print(f"// {bbt[1]}")
    print(f"byte bb{i}_vals[] = {{" + ",".join([str(x) for x in digits]) + "};")
    print(
        f"byte bb{i}_set(byte a, byte b) {{ bb{i}_vals[a * {len(digits)}  / 255] = b * {max(digits)*2-1} / 255; }}"
    )
    for j, digit in enumerate(digits):
        txt = txt.replace(str(digit), f"bb_vals[{j}]", 1)
    txt = txt.replace("bb_vals", f"bb{i}_vals")
    print(f"byte bb{i}() {{ return {txt}; }}")
    return True


bbs = []
for d1 in data:
    if "children" in d1:
        for d2 in d1["children"]:
            bbs.append((d2["codeOriginal"], json.dumps(d1)))
    else:
        bbs.append((d1["codeOriginal"], json.dumps(d1)))

random.shuffle(bbs)

i = 0
for bbt in bbs:
    bb = bbt[0]
    try:
        bb = bb.replace(" ", "")
    except:
        continue
    if len(bb) > 64:
        continue
    if bb_converter(bbt, i):
        i = i + 1
        if i == 40:
            break

print("byte bb_set(byte k, byte a, byte b) { ")
for j in range(i):
    if j == 0:
        print("\tif (k == 0) { return bb0_set(a,b); }")
    else:
        print(f"\telse if (k == {j}) {{return bb{j}_set(a,b); }} ")
print("}")

print("byte bb(byte k) { ")
for j in range(i):
    if j == 0:
        print("\tif (k == 0) { return bb0(); }")
    else:
        print(f"\telse if (k == {j}) {{return bb{j}(); }} ")
print("}")

print(f"#define BBMAX {i-1}")
