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

def green(text):
    return f'\033[32m{text}\033[0m'

def cyan(text):
    return f'\033[36m{text}\033[0m'

def magenta(text):
    return f'\033[35m{text}\033[0m'

verbose = False

def debug(text):
    if verbose:
        print(text, file=sys.stderr)

def error(text):
    print(f'{red('Error:')} {text}', file=sys.stderr)
    sys.exit(1)

def ign(node):
    if not isinstance(node, dict):
        return True
    if node.get('loc', {}).get('file', '').startswith('/'):
        return True
    if node.get('loc', {}).get('includedFrom') or node.get('isImplicit'):
        return True
    return False

hide = ['TranslationUnitDecl']
vars = []
string_prefix = 'STR__'
strings = []

def _debug_node(node, depth=0, prefix='', is_last=True, kind=None):
    kind = kind or cyan(node.get('kind', 'Unknown'))
    connector = '└─ ' if is_last else '├─ '
    if depth == 0:
        connector = ''
    tree_str = f'{prefix}{connector}{kind} {gray(node.get('id', ''))}\n'

    if depth == 0:
        new_prefix = '   '
    else:
        new_prefix = prefix + ('    ' if is_last else '│   ')

    items = [(k, v) for k, v in node.items() if k not in ['kind', 'id']]
    total = len(items)

    for idx, (key, value) in enumerate(items):
        is_last_item = idx == total - 1
        connector_child = '└─ ' if is_last_item else '├─ '

        if isinstance(value, dict):
            tree_str += _debug_node(
                value,
                depth + 1,
                prefix=new_prefix,
                is_last=is_last_item,
                kind=f'{key}: {cyan(value.get('kind', ''))}'
            )
        elif isinstance(value, list):
            for i, item in enumerate(value):
                item_is_last = i == len(value) - 1
                if isinstance(item, dict):
                    tree_str += _debug_node(
                        item,
                        depth + 1,
                        prefix=new_prefix,
                        is_last=item_is_last,
                        kind=f'{key}[{yellow(i)}]: {cyan(item.get('kind', ''))}'
                    )
                else:
                    item_connector = '└─ ' if item_is_last else '├─ '
                    tree_str += f'{new_prefix}{item_connector}{key}[{yellow(i)}]: {item}\n'
        else:
            tree_str += f'{new_prefix}{connector_child}{key}: {green(value)}\n'

    return tree_str

def unknownNodeDebug(node, msg):
    debug(f'Node parsing not implemented:\n{msg}\n{_debug_node(node, 0)}')
    if os.environ.get('STOPFAIL'):
        debug(red('STOPFAIL is set, stopping execution'))
        sys.exit(2)

def icst(t, i, c, st):
    return [{
        'type': t,
        'indent': i,
        'c': c,
        'st': st
    }]

def parseIntegerLiteral(node, depth):
    value = node.get('value')
    return icst(
        'int',
        depth,
        f'{value}\n',
        f'{value}'
    )

def parseFloatingLiteral(node, depth):
    value = node.get('value')
    if not value.isnumeric():
        unknownNodeDebug(node, 'FloatingLiteral with non-integer value')
    return icst(
        'float',
        depth,
        f'{value}f\n',
        f'{value}'
    )

def parseCharacterLiteral(node, depth):
    value = node.get('value')
    return icst(
        'char',
        depth,
        f'{value}\n',
        f'{value}'
    )

def parseStringLiteral(node, depth):
    value = node.get('value')
    if value == '"%c"':
        # ignore %c from printf
        pass
    else:
        strings.append(value)
    return []

def parseBreakStmt(node, depth):
    return icst(
        'break',
        depth,
        f'break\n',
        f'break_st\n'
    )

def parseCompoundStmt(node, depth):
    result = []
    for child in node.get('inner', []):
        result.extend(parse_ast(child, depth))
    return result

def parseDeclRefExpr(node, depth):
    if node.get('referencedDecl', ''):
        name = node['referencedDecl'].get('name', red('Unknown'))
        return icst(
            'deref',
            depth,
            f'{name}\n',
            f'{name.upper()} '
        )
    unknownNodeDebug(node, 'DeclRefExpr without referencedDecl')
    return []

def parseImplicitCastExpr(node, depth):
    cast_kind = node.get('castKind', red('Unknown'))
    inner = node.get('inner', [])
    if len(inner) != 1:
        unknownNodeDebug(node, 'ImplicitCastExpr with more than one child')
    inner = inner[0]
    ignore_casts = [
        'IntegralCast',
        'NoOp',
        'ArrayToPointerDecay',
        'LValueToRValue',
    ]
    if cast_kind in ignore_casts:
        return parse_ast(inner, depth)
    ckind = inner.get('kind', '')
    if ckind == 'ImplicitCastExpr':
        ckind = inner.get('castKind', '')
    unknownNodeDebug(node, f'ImplicitCastExpr {cast_kind} with {ckind}')
    return []

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

def parseBinaryOperator(node, depth):
    opcode = node.get('opcode', red('Unknown'))
    inner = node.get('inner', [])
    if len(inner) != 2:
        unknownNodeDebug(node, 'BinaryOperator with children != 2')
    lhs = inner[0]
    rhs = inner[1]
    if opcode == '=':
        if lhs.get('kind', '') == 'DeclRefExpr':
            name = lhs.get('referencedDecl', {}).get('name', red('Unknown'))
            result = icst(
                'var',
                depth,
                f'',
                f'{name.upper()} '
            )
            result.extend(parse_ast(rhs, depth))
            return result
        else:
            unknownNodeDebug(node, 'BinaryOperator = with non-DeclRefExpr LHS')
    elif opcode == '==':
        result = []
        result.extend(parse_ast(rhs, depth))
        result.extend(parse_ast(lhs, depth))
        result += icst(
            'eq',
            depth,
            '==\n',
            '?='
        )
        return result
    unknownNodeDebug(node, f'BinaryOperator {opcode}')

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
        ) + icst('fblock', depth, '{', '{')
    for child in node.get('inner', []):
        result.extend(parse_ast(child, depth))
    result += icst('fend', depth, '}', '}')
    return result

def parseReturnStmt(node, depth):
    return icst(
        'return',
        depth,
        f'return\n',
        ''
    )

def parseDeclStmt(node, depth):
    inner = node.get('inner', [])
    if len(inner) == 1:
        return parse_ast(inner[0], depth)
    else:
        unknownNodeDebug(node, 'DeclStmt with more than one child')
    return []

def parseVarDecl(node, depth):
    name = node.get('name', red('Unknown'))
    typeName = node.get('type', {}).get('qualType', red('Unknown'))
    if typeName == 'uint8_t':
        typest = 'b'
    elif typeName == 'uint16_t':
        typest = 's'
    elif typeName == 'uint32_t':
        typest = 'i'
    elif typeName == 'float':
        typest = 'f'
    else:
        error(f'Unknown type: {typeName} for {name} use only uint8_t, uint16_t, uint32_t or float')
    inner = node.get('inner', [])
    value = []
    if len(inner) == 1:
        value = parse_ast(inner[0], depth)
        value += icst(
            'store',
            depth,
            f'',
            f'!'
        )
    elif len(inner) > 1:
        unknownNodeDebug(node, 'VarDecl with more than one child')
    decl = icst(
        'var',
        depth,
        f'int {name}\n',
        f'{name.upper()}^'
    ) + value
    vars.append({
        'name': name,
        'type': typest,
        'decl': decl
    })
    return []

def parseCallExpr(node, depth):
    inner = node.get('inner', [])
    name = red('Unknown')
    result = []
    if len(inner) > 0:
        if inner[0].get('castKind', '') == 'FunctionToPointerDecay':
            ref = inner[0].get('inner', [{}])[0]
            if ref.get('kind', '') == 'DeclRefExpr':
                name = ref.get('referencedDecl', {}).get('name', red('Unknown'))
    nstrings = len(strings)
    for child in inner[1:]:
        result.extend(parse_ast(child, depth))
    if name == 'printf':
        if nstrings == len(strings):
            result.extend(icst('printf', depth, '', '.'))
        else:
            result.extend(icst('printf', depth, '', f'{string_prefix}{nstrings} PS'))
    elif name == 'getchar':
        result.extend(icst('getchar', depth, '', ','))
    else:
        result = icst(
            'call',
            depth,
            f'{name}()\n',
            f'{name.upper()}'
        )
        unknownNodeDebug(node, 'CallExpr')
    return result

def parseIfStmt(node, depth):
    has_else = node.get('hasElse', False)
    result = []
    inner = node.get('inner', [])
    cond = inner[0]
    body = inner[1]
    result.extend(parse_ast(cond, depth))
    result.extend(icst('if', depth, 'if (', '('))
    result.extend(parse_ast(body, depth))
    if has_else:
        result.extend(icst('else', depth, 'else', ':'))
        result.extend(parse_ast(inner[2], depth))
    result.extend(icst('endif', depth, '', ')'))
    return result

parse = {
    'IntegerLiteral': parseIntegerLiteral,
    'FloatingLiteral': parseFloatingLiteral,
    'CharacterLiteral': parseCharacterLiteral,
    'StringLiteral': parseStringLiteral,
    'ReturnStmt': parseReturnStmt,
    'BreakStmt': parseBreakStmt,
    'CompoundStmt': parseCompoundStmt,
    'DeclRefExpr': parseDeclRefExpr,
    'ImplicitCastExpr': parseImplicitCastExpr,
    'UnaryOperator': parseUnaryOperator,
    'BinaryOperator': parseBinaryOperator,
    'FunctionDecl': parseFunctionDecl,
    'DeclStmt': parseDeclStmt,
    'VarDecl': parseVarDecl,
    'CallExpr': parseCallExpr,
    'IfStmt': parseIfStmt,
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
        parsed_dict = icst('unknown', depth, fallback_c, fallback_st)
        debug(_debug_node(node, depth))
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
    ctype = 'b'
    strings_decl = []
    vars_decl = []
    for si, string in enumerate(strings):
        strings_decl.extend(icst('strinit', 0, '', f'{string_prefix}{si}^{string}>'))
    for var in vars:
        typest = var['type']
        if typest == ctype:
            typest = ''
        else:
            ctype = typest
            vars_decl.extend(icst('type', 0, '', f'{typest}'))
        vars_decl.extend(var['decl'])
        vars_decl.extend(icst('next', 0, '', '>'))
    dict_list = strings_decl + vars_decl + dict_list
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