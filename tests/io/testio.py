import os
import glob
import json
import subprocess

def gray(text):
    return f'\033[30m{text}\033[0m'

def yellow(text):
    return f'\033[33m{text}\033[0m'

def red(text):
    return f'\033[31m{text}\033[0m'

def green(text):
    return f'\033[32m{text}\033[0m'

def cyan(text):
    return f'\033[36m{text}\033[0m'

def main():
    p = 0
    f = 0
    w = 0
    for j in glob.glob('./*.json'):
        n = os.path.splitext(os.path.basename(j))[0]
        s = f'../c/{n}.st'
        if not os.path.isfile(s):
            print(yellow(f'[SKIP] {n}'))
            continue
        with open(j) as a:
            d = json.load(a)
        with open(s) as b:
            st = b.read().strip()
        for t in d.get('io', []):
            inp = t.get('input', '')
            exp = t.get('output', '')
            cmd = [f'../build/{n}']
            test = t.get('test', d.get('test', ''))
            r = subprocess.run(cmd, input=inp, text=True, capture_output=True).stdout
            if r == exp:
                p += 1
                print(green(f'[PASS c io] {n} => {test}'))
            else:
                f += 1
                print(red(f'[FAIL c io] {n} => {test} "{r}" != "{exp}"'))
        if d.get('expect') != st:
            if d.get('expect') != st.replace('\n', ''):
                print(red(f'[FAIL expect] "{d.get("expect")}" != "{st}"'))
                f += 1
                continue
            else:
                print(yellow(f'[WARN expect] "{d.get("expect")}" != "{st}"'))
                w += 1

        for t in d.get('io', []):
            inp = t.get('input', '')
            exp = t.get('output', '')
            cmd = ['../../build/start', '-f', s]
            test = t.get('test', d.get('test', ''))
            r = subprocess.run(cmd, input=inp, text=True, capture_output=True).stdout
            if r == exp:
                p += 1
                print(green(f'[PASS st io] {n} => {test}'))
            else:
                f += 1
                print(red(f'[FAIL st io] {n} => {test} "{r}" != "{exp}"'))
    print(cyan('[IO]'), end=' ')
    if p == 0:
        print(red(f'{f}/{f+p} tests failed ({w} warnings)'))
    elif f > 0:
        print(yellow(f'{f}/{f+p} tests failed ({w} warnings)'))
    else:
        print(green(f'{p} tests passed ({w} warnings)'))

if __name__ == '__main__':
    main()