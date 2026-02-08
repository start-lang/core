# Relatório: Otimização de Binário e Performance para ATmega328

## Sumário Executivo

Este relatório analisa o interpretador *T (start-lang/core) quanto à viabilidade de execução em ATmega328 (32KB Flash, 2KB SRAM, 16MHz) e oportunidades de melhoria de performance. A conclusão principal é que **o core do interpretador é arquiteturalmente compatível** com microcontroladores, mas existem **barreiras específicas** que precisam ser endereçadas — a maioria resolvível com mudanças não-invasivas via `#ifdef` e compile-time flags.

---

## 1. Características Fundamentais da Linguagem

### 1.1 Modelo de Execução

O interpretador opera como uma **máquina de fita** (tape machine) com:

- **Registrador** (`Register`): union de 4 bytes (i8[4], i16[2], i32, f32)
- **Fita de memória** (`_m`): buffer de bytes com ponteiro móvel (`<`, `>`)
- **Execução sequencial**: tokens processados um a um via `st_step()` → `st_op()`
- **Execução em blocos** (`_ignend`): permite ler código em chunks — essencial para dispositivos sem RAM para o código completo
- **Sub-estados** (`sub`): lista encadeada para chamadas de função e `RUN`
- **Leitura stream**: o teste `blockrun` em `lang_assertions.c:478-517` demonstra que o código pode ser alimentado em blocos arbitrários, permitindo leitura de EEPROM/UART/SD card

### 1.2 State Saving

O estado completo está contido na struct `State` (~42 bytes no AVR):
- Posição no código fonte (`src`, `_src0`)
- Posição na memória (`_m`, `_m0`, `_mlen`)
- Registrador, tipo, flags (bitfields compactos em 2 bytes)
- Buffer de identificador (`_id[16]`)
- Stack height, matching state

Isso permite **pausar e resumir** a execução a qualquer momento via `st_step()`.

---

## 2. Análise de Tamanho do Binário

### 2.1 ATmega328 — Restrições

| Recurso | Capacidade | Uso estimado (core) |
|---------|-----------|-------------------|
| Flash (código) | 32 KB | ~5-7 KB (sem float/debug) |
| SRAM (dados) | 2 KB | ~525-690 bytes base |
| EEPROM | 1 KB | Disponível para programas |
| Clock | 16 MHz | Suficiente para interpretação |

### 2.2 Estimativa de Flash por Componente

| Componente | Flash estimado (AVR) | Notas |
|-----------|---------------------|-------|
| `st_op()` — dispatch principal | ~3000-4000 bytes | O maior componente |
| `st_step()` + `st_run()` | ~300 bytes | Loop principal |
| `jump()` | ~150 bytes | Scan de brackets |
| `new_sub()` + `st_state_init/free` | ~320 bytes | Gestão de sub-estados |
| `mload()` | ~60 bytes | Helper de load |
| **Subtotal core** | **~3800-4800 bytes** | |

| Dependência | Flash estimado | Eliminável? |
|------------|---------------|-------------|
| `malloc`/`realloc`/`free` | ~800-1200 bytes | Sim, com pools estáticos |
| Soft-float library (AVR) | ~1500-2500 bytes | Sim, com `#ifdef` |
| `printf` (full) | ~2000-3000 bytes | Sim, substituir por UART puts |
| `printf` (minimal, sem float) | ~800-1200 bytes | — |
| `strcmp`/`strlen`/`strcpy`/`memset` | ~170 bytes | Necessário |
| 32-bit multiply/divide/mod | ~500 bytes | Necessário para INT32 |
| **debug_utils.c** | ~3000-5000 bytes | Sim, excluir no target AVR |
| **cli.c** | ~2000-3000 bytes | Sim, substituir por target AVR |

### 2.3 Cenários de Build

| Configuração | Flash Total | Cabe em 32KB? |
|-------------|------------|---------------|
| Full (float + printf + debug) | ~12-15 KB | Sim, mas apertado |
| Sem debug_utils | ~8-10 KB | Sim |
| Sem float | ~7-9 KB | Sim |
| Sem float + sem debug + UART minimal | **~5-7 KB** | **Sim, com ~25KB livres** |
| Apenas INT8 (modo BF-like) | ~3-4 KB | Sim, ultra-minimal |

### 2.4 Budget de SRAM (2048 bytes)

| Item | Bytes | Notas |
|------|-------|-------|
| State struct (root) | 42 | Alocação estática possível |
| Buffer de memória (MEM_SIZE=64) | 64 | Configurável |
| Globals (`_vars`, `_funcs`, counters) | 6 | Estáticos |
| `ext[]` table (15 entries) | 60 | Mover strings para PROGMEM |
| String literals das ext functions | ~70 | Mover para PROGMEM |
| Globals de stdf (`stop`, `output_len`, `seed`) | 7 | |
| Stack do AVR | ~256-400 | Reserva para call stack |
| Heap overhead (se usar malloc) | ~20-40 | |
| **Base total** | **~525-690** | |
| **Disponível para dados do programa** | **~1358-1523** | |

**Limites práticos do programa interpretado:**
- ~10-15 variáveis nomeadas
- ~5-8 funções definidas em runtime
- ~3-5 níveis de aninhamento de chamadas
- MEM_SIZE=64 bytes de fita (expansível até ~512 com `m`)

---

## 3. Análise de Performance

### 3.1 Bottleneck #1: `jump()` — Scan Linear de Brackets

**Impacto: CRÍTICO em programas com loops**

`src/start_lang.c:14-55`

Cada vez que o interpretador encontra um `[`, `]`, `(`, `)`, `break` ou `continue` que requer salto, a função `jump()` faz scan linear caractere por caractere até encontrar o matching bracket. Em um loop `[body]` que executa N iterações com corpo de tamanho L:

- **Custo atual**: O(N × L) — cada iteração refaz o scan
- **Custo ideal**: O(N) — com tabela pré-computada, o salto seria O(1)

Para o Mandelbrot (`tests/mandelbrot/m.st`) com loops aninhados pesados, isto domina o tempo de execução.

**Mudança sugerida**: Pré-computar tabela `jump_target[i]` em um pass O(n) antes da execução. Isto é a técnica padrão em interpretadores Brainfuck otimizados.

**Avaliação de invasividade**: MÉDIA. Requer um array adicional proporcional ao tamanho do código fonte. No desktop, trivial. No ATmega328, pode ser proibitivo se o programa for grande — mas compatível com o modelo de execução em blocos se limitado ao bloco atual.

### 3.2 Bottleneck #2: Lookup de Identificadores — strcmp Linear

**Impacto: ALTO em programas com variáveis/funções**

`src/start_lang.c:261-316`

Cada referência a uma variável ou função faz:
1. Scan linear em `_vars[]` com `strcmp` — O(V)
2. Scan linear em `_funcs[]` com `strcmp` — O(F)
3. Scan linear em `ext[]` com `strcmp` — O(E)

Com V variáveis, F funções e E=15 extensões, cada acesso custa O(V + F + 15).

**Mudança sugerida**: Hash table simples com ~32-64 slots. Com `MAX_IDLEN=16` e contadores `uint8_t`, uma hash lightweight caberia em ~128-256 bytes.

**Avaliação de invasividade**: MÉDIA-ALTA. Mudaria a estrutura de dados interna de `_vars`/`_funcs`. Entretanto, como estas são globais internas ao interpretador (não expostas na API), a mudança é contida.

### 3.3 Bottleneck #3: `strlen` Desnecessário no Backward Jump

**Impacto: MÉDIO**

`src/start_lang.c:120`

```c
if (! s->_forward) {
    s->src += strlen((char*) s->src) - 1;
```

Quando `jump()` precisa escanear para trás, `st_step()` primeiro chama `strlen()` para ir ao fim do source. Isso é O(n) no tamanho do código e acontece **em cada backward jump** (ex: `ENDWHILE` que volta para `WHILE`).

**Mudança sugerida**: Armazenar o comprimento do source em `State._lensrc` (campo já existente mas usado para outro propósito durante definição de funções). Ou adicionar um campo `_srclen` dedicado.

**Avaliação de invasividade**: BAIXA. Adicionar 2 bytes ao State e setar durante `st_state_init()`.

### 3.4 Bottleneck #4: `step_callback` em Cada Step

**Impacto: MÉDIO**

`src/start_lang.c:157` + `lib/tools/debug_utils.c:262`

A cada passo, `st_run()`:
1. Percorre a chain de sub-estados para achar o mais profundo — O(D)
2. Chama `step_callback()` que chama `debug_state()`
3. `debug_state()` incrementa contadores, checa limites, checa timeout (com `clock()` a cada 10000 steps)

**Mudança sugerida**:
- Function pointer para callback: `NULL` = sem overhead
- No ATmega328: callback mínimo apenas com watchdog reset
- Compilação condicional: `#ifdef DEBUG_ENABLED`

**Avaliação de invasividade**: BAIXA. O `step_callback` já é `extern` e definido por cada target.

### 3.5 Bottleneck #5: `mload()` Não Inlined

**Impacto: BAIXO-MÉDIO**

`src/start_lang.c:184-196`

Chamada em cada operação aritmética/comparação para tipos 16/32-bit. Faz cópia byte a byte. Não é `static inline`.

**Mudança sugerida**: Marcar como `static inline`. Substituir cópias byte-a-byte por `memcpy` com tamanho constante (o compilador otimiza para load/store nativo).

**Avaliação de invasividade**: MUITO BAIXA. Apenas adicionar `static inline` e opcionalmente usar `memcpy`.

### 3.6 Bottleneck #6: Traversal Duplo de Sub-estados

**Impacto: BAIXO**

`st_run()` (linhas 155-156) e `st_step()` (linhas 97-98) ambos percorrem a chain `sub->sub->...` a cada step. Redundância O(D) por step.

**Mudança sugerida**: Manter ponteiro para o sub-estado mais profundo diretamente no State root.

**Avaliação de invasividade**: BAIXA. Adicionar um campo `State * _deepest` e atualizar em `new_sub()` e cleanup.

### 3.7 Repetição do Dispatch de Tipo

**Impacto: CODE SIZE (não performance direta)**

O padrão `if INT8 / else if INT16 / else if INT32 / else if FLOAT` aparece ~25+ vezes em `st_op()`. Cada instância gera branches e código separado.

**Mudança sugerida**: Macros para encapsular o padrão (reduz tamanho do código e melhora I-cache). Exemplo de `mstore()` inline complementando `mload()`.

**Avaliação de invasividade**: BAIXA. Refactoring puramente mecânico com macros.

---

## 4. Classificação: O que Mudar vs. O que Preservar

### 4.1 DEVE ser preservado (características fundamentais)

| Característica | Razão |
|---------------|-------|
| Execução token-a-token (`st_step`) | Core do modelo — permite execução em stream/blocos |
| State como struct autônoma | Permite pause/resume, save/restore |
| Register union de 4 bytes | Eficiente e AVR-friendly |
| Bitfields compactos nos flags | Já otimizado para memória |
| Tokens single-byte ASCII | Perfeito para 8-bit |
| Tipos `uint8_t` para contadores | Já limitados a 255, adequado |
| Tabela `ext[]` de funções externas | Interface limpa para diferentes targets |
| `step_callback` como hook externo | Flexibilidade para cada plataforma |
| `MEM_SIZE = 64` default | Conservador, adequado para ATmega328 |
| Compatibilidade Brainfuck | Sem custo adicional, usa os mesmos operadores |

### 4.2 PODE mudar com baixo impacto

| Mudança | Ganho | Risco |
|---------|-------|-------|
| `mload()`/`mstore()` → `static inline` | Performance +5-10% em aritmética | Nenhum |
| Eliminar `strlen` no backward jump | Performance em loops | Mínimo — 2 bytes no State |
| `#ifdef` para FLOAT support | -2-3KB Flash no AVR | Nenhum se condicional |
| `#ifdef` para debug_utils | -3-5KB Flash no AVR | Já esperado por target |
| Reduzir `MAX_IDLEN` 16→8 (AVR only) | -8 bytes por State | Limita nomes a 7 chars |
| `PROGMEM` para strings de `ext[]` (AVR) | -130 bytes SRAM | Específico AVR, `#ifdef` |
| Macros para dispatch de tipo | -500-1000 bytes code size | Refactoring mecânico |

### 4.3 PODE mudar com impacto médio

| Mudança | Ganho | Risco | Análise |
|---------|-------|-------|---------|
| Tabela de jump pré-computada | **Performance de loops: 10-100x** | Requer array extra; incompatível com execução em blocos se não adaptada | A maior otimização possível. Para o modo desktop, trivial. Para ATmega328, pode ser limitada ao bloco atual |
| Hash table para vars/funcs | **Lookup: O(1) vs O(n)** | Muda estrutura interna | Contido ao interpretador. ~128-256 bytes extra |
| Pools estáticos em vez de malloc | **Elimina fragmentação, -800B Flash** | Limita número de vars/funcs/sub-estados | Necessário para ATmega328. Pode coexistir via `#ifdef` |
| Ponteiro para sub-estado mais profundo | Elimina 2× traversal O(D) | Manutenção do ponteiro | Baixo risco real |

### 4.4 NÃO deve mudar (quebraria a linguagem)

| Aspecto | Razão para preservar |
|---------|---------------------|
| Semântica de operadores | Programas existentes quebrariam |
| Ordem de avaliação (prev_token / lookahead) | Gramática da linguagem depende disso |
| Modelo de memória (fita linear) | Fundamental da linguagem |
| Namespace global de vars/funcs | Semântica da linguagem |
| Comportamento do `#` (RUN) | Feature core da linguagem |
| Execução em blocos com `_ignend` | Feature diferenciadora |

---

## 5. Plano de Ação Recomendado

### Fase 1: Quick Wins (baixo risco, sem mudança de API)

1. **`static inline` em `mload()`** — 1 linha mudada
2. **Adicionar `mstore()` inline** — função nova, ~5 linhas
3. **Eliminar `strlen` no backward jump** — armazenar `_srclen` no State
4. **`#ifdef ENABLE_FLOAT` no dispatch de tipos** — compile-time flag

### Fase 2: Otimização de Performance (médio risco)

5. **Tabela de jump pré-computada** — pass O(n) no init, array `uint16_t[]`
6. **Hash simples para lookup de identificadores** — FNV-1a hash, 64 slots

### Fase 3: Target ATmega328 (mudanças confinadas ao target)

7. **`targets/arduino/` com entry point próprio** — UART I/O
8. **Pools estáticos via `#ifdef AVR`** — `State _states[4]`, `Variable _vars[16]`
9. **`PROGMEM` para strings constantes** — `ext[]` names
10. **Callback mínimo** — watchdog reset only

### Ganhos Estimados

| Métrica | Antes | Depois (Fase 1+2) | Depois (Fase 3, AVR) |
|---------|-------|--------------------|---------------------|
| Loop performance | O(N×L) | **O(N)** | O(N) |
| Identifier lookup | O(V+F+E) | **O(1)** | O(V+F+E)* |
| Flash (AVR, sem float) | ~9KB | ~8KB | **~5-6KB** |
| SRAM base (AVR) | ~690 | ~690 | **~500** |
| Backward jump | O(src_len) | **O(1)** | O(1) |

*No ATmega328, com V≤16 e F≤8, o scan linear é aceitável — hash table seria over-engineering.

---

## 6. Conclusão

O interpretador *T já foi projetado com mentalidade embarcada: tipos `uint8_t` para contadores, bitfields compactos, tokens single-byte, MEM_SIZE=64, e a capacidade única de execução em blocos (`blockrun`). A arquitetura está fundamentalmente correta para ATmega328.

As principais barreiras são:
1. **Alocação dinâmica** (`malloc`/`realloc`) — resolvível com pools estáticos via `#ifdef`
2. **Dependência de `printf`** — resolvível com UART direto
3. **Soft-float library** — resolvível com `#ifdef ENABLE_FLOAT`
4. **Debug/CLI code** — já separado em targets diferentes

Para **performance no desktop**, a otimização mais impactante é a **tabela de jump pré-computada**, que transformaria loops de O(N×L) para O(N) — ganho potencial de 10-100x em programas loop-heavy como Mandelbrot e BF programs.

Todas as mudanças propostas podem coexistir com o código atual via compilação condicional, sem impactar os targets existentes (desktop, WASM).
