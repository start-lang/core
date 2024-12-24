import os
import re
import sys
import json
import argparse
import subprocess


def gray(text):
    return f'\033[30m{text}\033[0m'

def yellow(text):
    return f'\033[33m{text}\033[0m'

def red(text):
    return f'\033[31m{text}\033[0m'

def cyan(text):
    return f'\033[36m{text}\033[0m'

def magenta(text):
    return f'\033[35m{text}\033[0m'

verbose = False

def debug(text):
    if verbose:
        print(text, file=sys.stderr)

def ign(node):
    if not isinstance(node, dict):
        return True
    if node.get('loc', {}).get('file', '').startswith('/'):
        return True
    if node.get('loc', {}).get('includedFrom') or node.get('isImplicit'):
        return True
    return False

hide = ['TranslationUnitDecl']

def _debug_node(node, depth, color=magenta):
    tree_str = ' ' * (depth * 2) + f'{node.get('kind', 'Unknown')} {node.get('id', '')}\n'
    def dict_to_str(d, depth):
        tree_str = '\n'
        depth += 1
        for key, value in d.items():
            if value and isinstance(value, dict):
                tree_str += ' ' * (depth * 2) + f'{key}: {dict_to_str(value, depth)}\n'
            elif value and isinstance(value, list):
                for v in value:
                    if v and isinstance(v, dict):
                        tree_str += ' ' * (depth * 2) + f'{key}: {dict_to_str(v, depth)}\n'
                    else:
                        tree_str += ' ' * (depth * 2) + f'{key}: {v}\n'
            else:
                tree_str += ' ' * (depth * 2) + f'{key}: {value}\n'
        return tree_str[:-1]
    for key, value in node.items():
        if value and isinstance(value, dict):
            tree_str += ' ' * (depth * 2) + f'{key}: {dict_to_str(value, depth)}\n'
        elif value and isinstance(value, list):
            for v in value:
                if v and isinstance(v, dict):
                    tree_str += ' ' * (depth * 2) + f'{key}: {dict_to_str(v, depth)}\n'
                else:
                    tree_str += ' ' * (depth * 2) + f'{key}: {v}\n'
        else:
            tree_str += ' ' * (depth * 2) + f'{key}: {value}\n'
    return color(tree_str)

def icst(t, i, c, st):
    return [{
        'type': t,
        'indent': i,
        'c': c,
        'st': st
    }]

def parseIntegerLiteral(node, depth):
    return icst(
        'int',
        depth,
        f'{node.get('value', red('0'))}\n',
        f'{node.get('value', red('0'))}st\n'
    )

def parseFloatingLiteral(node, depth):
    return icst(
        'float',
        depth,
        f'{node.get('value', red('0.0'))}f\n',
        f'{node.get('value', red('0.0'))}f_st\n'
    )

def parseStringLiteral(node, depth):
    return icst(
        'string',
        depth,
        f'{node.get('value', red('?'))}\n',
        f'{node.get('value', red('?'))}_st\n'
    )

def parseBreakStmt(node, depth):
    return icst(
        'break',
        depth,
        f'break\n',
        f'break_st\n'
    )

def parseCompoundStmt(node, depth):
    result = icst('block', depth, '{', '{')
    for child in node.get('inner', []):
        result.extend(parse_ast(child, depth))
    result.extend(icst('block', depth, '}', '}'))
    return result

def parseDeclStmt(node, depth):
    name = node.get('referencedDecl', {}).get('name', red('Unknown'))
    return icst(
        'decl',
        depth,
        f'{name}\n',
        f'{name}_st\n'
    )

def parseImplicitCastExpr(node, depth):
    cast_kind = node.get('castKind', red('Unknown'))
    result = icst(
        'cast',
        depth,
        f'~{cast_kind}\n',
        f'~{cast_kind}_st\n'
    )
    for child in node.get('inner', []):
        result.extend(parse_ast(child, depth))
    return result

def parseUnaryOperator(node, depth):
    opcode = node.get('opcode', red('Unknown'))
    result = icst(
        'unary',
        depth,
        f'{opcode}\n',
        f'{opcode}_st\n'
    )
    for child in node.get('inner', []):
        result.extend(parse_ast(child, depth))
    return result

def parseFunctionDecl(node, depth):
    name = node.get('name', red('Unknown'))
    if name == 'main':
        result = icst(
            'main',
            depth,
            '',
            ''
        )
        if len(node.get('inner', [])) == 1 and node['inner'][0].get('kind', '') == 'CompoundStmt':
            for child in node['inner'][0].get('inner', []):
                result.extend(parse_ast(child, depth))
        return result
    else:
        result = icst(
            'function',
            depth,
            f'void {name}()\n',
            f'{name.upper()}'
        )
    for child in node.get('inner', []):
        result.extend(parse_ast(child, depth))
    return result

def parseReturnStmt(node, depth):
    return icst(
        'return',
        depth,
        f'return\n',
        ''
    )

parse = {
    'IntegerLiteral': parseIntegerLiteral,
    'FloatingLiteral': parseFloatingLiteral,
    'StringLiteral': parseStringLiteral,
    'ReturnStmt': parseReturnStmt,
    'BreakStmt': parseBreakStmt,
    'CompoundStmt': parseCompoundStmt,
    'DeclRefExpr': parseDeclStmt,
    'ImplicitCastExpr': parseImplicitCastExpr,
    'UnaryOperator': parseUnaryOperator,
    'FunctionDecl': parseFunctionDecl,
}

miss = {}

def parse_ast(node, depth=0):
    if ign(node):
        return ''
    kind = node.get('kind', yellow('Unknown'))
    if kind in parse:
        return parse[kind](node, depth)
    result = []
    if kind not in hide:
        miss[kind] = miss.get(kind, 0) + 1
        name = node.get('name', '')
        if name:
            name = f': {name}'
        fallback_c = f'{red(kind)}{name} {gray(node.get('id',''))}\n'
        fallback_st = f'{red(kind)}{name}_st {gray(node.get('id',''))}\n'
        parsed_dict = icst(depth, fallback_c, fallback_st)
        result.extend(parsed_dict)
    for child in node.get('inner', []):
        result.extend(parse_ast(child, depth))
    return result

def generate_c_code(dict_list):
    lines = []
    for item in dict_list:
        code_lines = item['c'].split('\n')
        for line in code_lines:
            if line.strip():
                lines.append(' ' * (item['indent'] * 2) + line)
    return '\n'.join(lines) + '\n'

def generate_st_code(dict_list):
    lines = []
    for item in dict_list:
        code_lines = item['st'].split('\n')
        for line in code_lines:
            if line.strip():
                lines.append(' ' * (item['indent'] * 2) + line)
    return ''.join(lines)

def load_source(file):
    try:
        if not os.path.exists('debug'):
            os.makedirs('debug')

        name = os.path.basename(file)
        name = os.path.splitext(name)[0]

        result = subprocess.run(
            ['clang', '-Xclang', '-ast-dump=json', '-fsyntax-only', file],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            check=True
        )

        with open('debug/%s.json' % name, 'w') as f:
            f.write(result.stdout)
        ast = json.loads(result.stdout)

        if not os.path.exists('io'):
            os.makedirs('io')

        with open(file, 'r') as f:
            content = f.read()

        test = re.search(r'/\*\n(.*?)\n\*/', content, re.DOTALL)
        if test:
            test = test.group(1).split('\n')
            info = {}
            t = '?'
            for line in test:
                line = line.strip()
                if line.startswith('test:'):
                    if 'test' in info:
                        if 'io' not in info:
                            info['io'] = []
                        info['io'].append({})
                        info['io'][-1]['test'] = line[5:].strip()
                    else:
                        info['test'] = line[5:].strip()
                    t = 't'
                elif line.startswith('expect:'):
                    info['expect'] = line[7:].strip()
                    t = 'e'
                elif line.startswith('input:'):
                    if 'io' not in info:
                        info['io'] = [{}]
                    if 'input' in info['io'][-1]:
                        info['io'].append({})
                    info['io'][-1]['input'] = line[6:].strip()
                    t = 'i'
                elif line.startswith('output:'):
                    if 'io' not in info:
                        info['io'] = [{}]
                    if 'output' in info['io'][-1]:
                        info['io'].append({})
                    info['io'][-1]['output'] = line[7:].strip()
                    t = 'o'
                else:
                    if t == 't':
                        info['test'] += '\n' + line
                    elif t == 'e':
                        info['expect'] += '\n' + line
                    elif t == 'i':
                        info['io'][-1]['input'] += '\n' + line
                    elif t == 'o':
                        info['io'][-1]['output'] += '\n' + line
            debug(info)
            with open('io/%s.json' % name, 'w') as f:
                f.write(json.dumps(info))

        return ast
    except subprocess.CalledProcessError as e:
        print(f'Clang error: {e.stderr.strip()}', file=sys.stderr)
        sys.exit(1)
    except json.JSONDecodeError as e:
        print(f'Error decoding JSON: {e}', file=sys.stderr)
        sys.exit(1)
    except FileNotFoundError:
        print('Error: clang is not installed or not found in PATH', file=sys.stderr)
        sys.exit(1)
    except Exception as e:
        print(f'Error: {e}', file=sys.stderr)
        sys.exit(1)

def main():
    parser = argparse.ArgumentParser(description='clang AST to *T transpiler.')
    parser.add_argument('--silent', action='store_true', help='Do not print debug information.')
    parser.add_argument('source', help='C source file.')
    args = parser.parse_args()

    global verbose
    verbose = not args.silent

    json_input = load_source(args.source)

    ast = parse_ast(json_input)

    outputC = generate_c_code(ast)

    outputST = generate_st_code(ast)

    output = f'{outputC}\n{outputST}'
    debug(output)
    print(outputST)
    if miss:
        print(red('Unknown nodes:'), file=sys.stderr)
        for k, v in miss.items():
            print(f'  {yellow(k)}: {v}', file=sys.stderr)
        sys.exit(1)

if __name__ == '__main__':
    main()