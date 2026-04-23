#!/usr/bin/env python3
"""
Conversor de C para Start (*T)
Suporta: declarações int, for (count-up), while(1), printf("%d"), printf("str")
"""

import re
import sys

# Mapeamento de tipos C → Start
TYPE_MAP = {
    'int': 'i',
    'uint8_t': 'b', 'int8_t': 'b',
    'uint16_t': 's', 'int16_t': 's',
    'uint32_t': 'i', 'int32_t': 'i',
    'float': 'f', 'double': 'f',
}

# Tamanho em bytes por tipo Start (para avanço de memória)
TYPE_SIZE = {'b': 1, 's': 2, 'i': 4, 'f': 4}

# Operadores C → Start (comparação: mem OP reg)
CMP_MAP = {
    '<': '?<', '>': '?>', '<=': '?l', '>=': '?g', '==': '?=', '!=': '?!',
}
ARITH_MAP = {'+': '+', '-': '-', '*': '*', '/': '/', '%': '%'}


def tokenize(expr):
    """Tokeniza uma expressão C em lista de tokens."""
    tokens = []
    i = 0
    expr = expr.strip()
    while i < len(expr):
        c = expr[i]
        if c.isspace():
            i += 1
            continue
        if c.isdigit():
            num = ''
            while i < len(expr) and (expr[i].isdigit() or expr[i] == '.'):
                num += expr[i]; i += 1
            tokens.append(('NUM', num))
            continue
        if c.isalpha() or c == '_':
            ident = ''
            while i < len(expr) and (expr[i].isalnum() or expr[i] == '_'):
                ident += expr[i]; i += 1
            tokens.append(('ID', ident))
            continue
        two = expr[i:i+2]
        if two in ('>=', '<=', '==', '!=', '&&', '||', '++', '--'):
            tokens.append(('OP', two)); i += 2
            continue
        if c in '+-*/%<>()!':
            tokens.append(('OP', c)); i += 1
            continue
        i += 1
    return tokens


def to_rpn(tokens):
    """Shunting-yard: infix tokens → RPN."""
    PREC = {'*': 5, '/': 5, '%': 5, '+': 4, '-': 4,
            '<': 3, '>': 3, '<=': 3, '>=': 3, '==': 3, '!=': 3,
            '&&': 2, '||': 1}
    out, ops = [], []
    for ttype, tval in tokens:
        if ttype in ('NUM', 'ID'):
            out.append((ttype, tval))
        elif ttype == 'OP' and tval == '(':
            ops.append(tval)
        elif ttype == 'OP' and tval == ')':
            while ops and ops[-1] != '(':
                out.append(('OP', ops.pop()))
            if ops: ops.pop()
        elif ttype == 'OP' and tval in PREC:
            while ops and ops[-1] != '(' and ops[-1] in PREC and PREC[ops[-1]] >= PREC[tval]:
                out.append(('OP', ops.pop()))
            ops.append(tval)
    while ops:
        out.append(('OP', ops.pop()))
    return out


class CToStart:
    def __init__(self):
        self.vars = {}       # name → st_type ('i','b','s','f')
        self.loop_stack = [] # stack of loop variable names (for increment/close)

    def var_name(self, c_name):
        """Converte nome de variável C para identificador Start (uppercase)."""
        return c_name.upper()

    def declare_vars(self, names_types):
        """Gera código para declarar variáveis (com avanço de memória entre elas)."""
        out = []
        for name, st_type in names_types:
            vn = self.var_name(name)
            self.vars[name] = st_type
            out.append(f'{st_type}{vn}^')
            out.append(f'{st_type}>')
        return out

    def _cond_to_start(self, cond_str):
        """Converte condição C para Start, deixando _ans configurado.
        Padrão: carrega operando direito no reg, aponta mem para operando esquerdo.
        ?< testa mem[left] < reg(right).
        """
        tokens = tokenize(cond_str)
        rpn = to_rpn(tokens)
        if len(rpn) == 3:
            (at, av), (bt, bv), (ot, ov) = rpn
            if ov in CMP_MAP:
                out = []
                lst = self.vars.get(av, 'i') if at == 'ID' else 'i'
                rst = self.vars.get(bv, 'i') if bt == 'ID' else lst
                # Carrega operando direito no reg
                if bt == 'NUM':
                    out.append(f'{lst}{bv}')
                else:
                    bvn = self.var_name(bv)
                    out.append(f'{rst}{bvn};')
                # Move pointer para operando esquerdo (sem carregar no reg)
                if at == 'NUM':
                    # Literal não tem var: coloca no reg e compara com mem atual
                    out.append(f'{lst}{av}')
                else:
                    avn = self.var_name(av)
                    out.append(f'{lst}{avn}')  # muda pointer sem ;
                out.append(CMP_MAP[ov])
                return out
        elif len(rpn) == 1:
            ttype, tval = rpn[0]
            if ttype == 'ID':
                vn = self.var_name(tval)
                st = self.vars.get(tval, 'i')
                return [f'{st}{vn};??']
        return [f'/* unsupported cond: {cond_str} */']

    def expr_to_start(self, expr_str, dest_var=None):
        """
        Converte uma expressão C para operações Start.
        Resultado fica em mem[dest_var] se dest_var fornecido,
        senão fica no registrador.

        Estratégia para expressão binária 'a OP b':
          - Se a é var: iA; iDEST! (copia a para dest)
          - Se a é literal: iLIT iDEST!
          - Então aplica OP b sobre dest

        Para expressões mais complexas usa stack (p/o).
        """
        tokens = tokenize(expr_str)
        rpn = to_rpn(tokens)

        # Caso simples: expressão com apenas um termo
        if len(rpn) == 1:
            ttype, tval = rpn[0]
            if dest_var:
                vn = self.var_name(dest_var)
                st = self.vars.get(dest_var, 'i')
                if ttype == 'NUM':
                    return [f'{st}{tval}', f'{vn}!']
                else:
                    svn = self.var_name(tval)
                    sst = self.vars.get(tval, 'i')
                    return [f'{sst}{svn};', f'{vn}!']
            else:
                ttype, tval = rpn[0]
                if ttype == 'NUM':
                    return [f'i{tval}']
                else:
                    vn = self.var_name(tval)
                    st = self.vars.get(tval, 'i')
                    return [f'{st}{vn};']

        # Caso binário: a OP b  (RPN: a b OP)
        if len(rpn) == 3:
            (at, av), (bt, bv), (ot, ov) = rpn
            st = self.vars.get(dest_var, 'i') if dest_var else 'i'
            dvn = self.var_name(dest_var) if dest_var else None

            if ov in CMP_MAP:
                # Comparação — deixa _cond/_ans configurados
                # Lado esquerdo em mem, lado direito em reg
                out = []
                # Carrega lado esquerdo para dest (ou variável temp via reg)
                if at == 'NUM':
                    out.append(f'{st}{av}')
                    if dvn:
                        out.append(f'{dvn}!')
                else:
                    svn = self.var_name(av)
                    sst = self.vars.get(av, 'i')
                    out.append(f'{sst}{svn};')
                    if dvn:
                        out.append(f'{dvn}!')
                # Carrega lado direito em reg
                if bt == 'NUM':
                    out.append(f'{st}{bv}')
                else:
                    bvn = self.var_name(bv)
                    bst = self.vars.get(bv, 'i')
                    out.append(f'{bst}{bvn};')
                out.append(CMP_MAP[ov])
                return out

            if ov in ARITH_MAP:
                # Aritmética: dest = a OP b
                out = []
                if dvn:
                    # Optimização: se 'a' é o próprio dest, opera directamente
                    if at == 'ID' and av == dest_var:
                        # dest OP= b
                        if bt == 'NUM':
                            out.append(f'{st}{bv}')
                        else:
                            bvn = self.var_name(bv)
                            bst = self.vars.get(bv, 'i')
                            out.append(f'{bst}{bvn};')
                        out.append(f'{dvn}{ARITH_MAP[ov]}')
                    else:
                        # Copia 'a' em dest, depois aplica OP 'b'
                        if at == 'NUM':
                            out.append(f'{st}{av}')
                            out.append(f'{dvn}!')
                        else:
                            svn = self.var_name(av)
                            sst = self.vars.get(av, 'i')
                            out.append(f'{sst}{svn};')
                            out.append(f'{dvn}!')
                        if bt == 'NUM':
                            out.append(f'{st}{bv}')
                        else:
                            bvn = self.var_name(bv)
                            bst = self.vars.get(bv, 'i')
                            out.append(f'{bst}{bvn};')
                        out.append(f'{dvn}{ARITH_MAP[ov]}')
                    return out

        # Fallback: não suportado, retorna comentário
        return [f'/* unsupported: {expr_str} */']

    def convert(self, c_code):
        lines = c_code.split('\n')
        out = []
        # Fase 1: pré-declara todas as variáveis (para alocação de memória)
        # Coleta declarações em ordem de aparecimento
        all_decls = []
        seen = set()
        for line in lines:
            line = line.strip()
            # declaração normal: tipo nome = ...;  ou  tipo nome;
            m = re.match(r'^(int|float|double|uint\w+|int\w+)\s+(\w+)\s*(?:=.*)?;', line)
            if m:
                c_type = m.group(1)
                vname = m.group(2)
                st_type = TYPE_MAP.get(c_type, 'i')
                if vname not in seen:
                    seen.add(vname)
                    all_decls.append((vname, st_type))
                    self.vars[vname] = st_type
            # declaração inline no for: for (int i = ...)
            m2 = re.match(r'^for\s*\(\s*(int|float|uint\w+|int\w+)\s+(\w+)\s*=', line)
            if m2:
                c_type = m2.group(1)
                vname = m2.group(2)
                st_type = TYPE_MAP.get(c_type, 'i')
                if vname not in seen:
                    seen.add(vname)
                    all_decls.append((vname, st_type))
                    self.vars[vname] = st_type

        # Gera bloco de declarações no início
        if all_decls:
            for vname, st_type in all_decls:
                vn = self.var_name(vname)
                out.append(f'{st_type}{vn}^')
                out.append(f'{st_type}>')
        out.append('i_STR_^')

        # Fase 2: converte corpo linha por linha
        for line in lines:
            line = line.strip()
            if not line or line.startswith('//') or line.startswith('#'):
                continue
            if re.match(r'^(int|void)\s+main', line) or line in ('{', 'return 0;'):
                continue

            # Declaração com inicialização
            m = re.match(r'^(int|float|double|uint\w+|int\w+)\s+(\w+)\s*=\s*(.+);', line)
            if m:
                c_type, vname, expr = m.group(1), m.group(2), m.group(3)
                vn = self.var_name(vname)
                st = self.vars.get(vname, 'i')
                ops = self.expr_to_start(expr, vname)
                out.extend(ops)
                continue

            # for (int i = INIT; i CMP LIMIT; i++ ou i--)
            m = re.match(r'^for\s*\(\s*int\s+(\w+)\s*=\s*(\d+)\s*;\s*\w+\s*(<|>|<=|>=)\s*(\w+)\s*;\s*\w+(\+\+|--)\s*\)', line)
            if m:
                ivar, init, cmp_op, limit, step = m.group(1), m.group(2), m.group(3), m.group(4), m.group(5)
                st = self.vars.get(ivar, 'i')
                vn = self.var_name(ivar)
                self.loop_stack.append((ivar, step))
                # Inicializa
                out.append(f'{st}{init}')
                out.append(f'{vn}!')
                # Abre loop com t[ e verifica condição no início
                out.append('t[')
                # Condição: se NOT (i CMP LIMIT) → break
                if limit.isdigit():
                    limit_code = f'{st}{limit}'
                else:
                    lvn = self.var_name(limit)
                    lst = self.vars.get(limit, 'i')
                    limit_code = f'{lst}{lvn};'
                out.append(f'{st}{vn};')
                out.append(limit_code)
                out.append(f'{CMP_MAP[cmp_op]}~(x:)')
                continue

            # if (cond) {
            m = re.match(r'^if\s*\((.+)\)\s*\{?$', line)
            if m and not line.startswith('for'):
                cond = m.group(1).strip()
                ops = self._cond_to_start(cond)
                out.extend(ops)
                self.loop_stack.append(('__if__', None))
                out.append('(')
                continue

            # while (1) {
            m = re.match(r'^while\s*\(\s*1\s*\)\s*\{?$', line)
            if m:
                self.loop_stack.append(('__while__', None))
                out.append('t[')
                continue

            # break;
            if re.match(r'^break\s*;$', line):
                out.append('x:')
                continue

            # } else {
            if re.match(r'^\}\s*else\s*\{', line):
                out.append(':')
                continue

            # printf("%d\\n", var_or_expr)
            m = re.match(r'^printf\s*\(\s*"%d\\n"\s*,\s*(.+)\s*\);', line)
            if m:
                expr = m.group(1).strip()
                vname = expr if expr.isidentifier() else None
                if vname and vname in self.vars:
                    st = self.vars[vname]
                    vn = self.var_name(vname)
                    out.append(f'{st}{vn};')
                    out.append('PN')
                else:
                    # expressão geral
                    ops = self.expr_to_start(expr)
                    out.extend(ops)
                    out.append('PN')
                out.append('i_STR_;')
                out.append('b10@.@')
                continue

            # printf("%d", var_or_expr)
            m = re.match(r'^printf\s*\(\s*"%d"\s*,\s*(.+)\s*\);', line)
            if m:
                expr = m.group(1).strip()
                vname = expr if expr.isidentifier() else None
                if vname and vname in self.vars:
                    st = self.vars[vname]
                    vn = self.var_name(vname)
                    out.append(f'{st}{vn};')
                    out.append('PN')
                else:
                    ops = self.expr_to_start(expr)
                    out.extend(ops)
                    out.append('PN')
                continue

            # printf("string\n")  ou  printf("string")
            m = re.match(r'^printf\s*\(\s*"(.*)"\s*\);', line)
            if m:
                s = m.group(1)
                has_newline = '\\n' in s
                s = s.replace('\\n', '')
                out.append('i_STR_;')
                if s:
                    out.append(f'"{s}" PS')
                if has_newline:
                    out.append('b10@.@')
                continue

            # Atribuição simples: var = expr;
            m = re.match(r'^(\w+)\s*=\s*(.+);', line)
            if m:
                vname, expr = m.group(1), m.group(2)
                if vname in self.vars:
                    ops = self.expr_to_start(expr, vname)
                    out.extend(ops)
                    continue

            # Fecha bloco }
            if line == '}':
                if self.loop_stack:
                    top = self.loop_stack.pop()
                    if isinstance(top, tuple) and top[0] == '__if__':
                        out.append(')')
                    elif isinstance(top, tuple) and top[0] == '__while__':
                        out.append('t]')
                    else:
                        loop_var, step = top
                        vn = self.var_name(loop_var)
                        st = self.vars.get(loop_var, 'i')
                        if step == '--':
                            out.append(f'{st}{vn} 1-')
                        else:
                            out.append(f'{st}{vn} 1+')
                        out.append('t')
                        out.append(']')

        return '\n'.join(out)


def main():
    if len(sys.argv) < 2:
        print("Usage: c_to_start.py <c_file>", file=sys.stderr)
        sys.exit(1)
    with open(sys.argv[1]) as f:
        c_code = f.read()
    converter = CToStart()
    print(converter.convert(c_code))


if __name__ == '__main__':
    main()
