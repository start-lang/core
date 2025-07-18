![Version](https://start-lang.github.io/core/img/version.svg)
![GitHub Actions Workflow Status](https://img.shields.io/github/actions/workflow/status/start-lang/core/ci.yaml)
![Tests](https://start-lang.github.io/core/img/tests.svg)
![Coverage](https://start-lang.github.io/core/img/coverage.svg)
![GitHub commit activity](https://img.shields.io/github/commit-activity/t/start-lang/core/main?label=commits)

## 1. Introdução

\*T (lê-se "start") é uma linguagem esotérica estruturada, interpretada e compacta, pensada para funcionar mesmo em ambientes com recursos extremamente limitados. O projeto começou a partir de uma pergunta simples: qual seria a forma mais enxuta possível de interagir com um dispositivo digital para programá-lo? A ideia original envolvia um cenário quase absurdo — uma matriz de LEDs, um potenciômetro e um botão — mas ela serviu como base para imaginar uma linguagem que pudesse operar com o mínimo de entrada humana.

Durante os testes iniciais, ficou claro que linguagens como o Brainfuck, apesar de minimalistas, não eram práticas para esse tipo de interface. Elas exigem comandos demais para tarefas simples. A \*T nasceu da tentativa de resolver isso: uma linguagem sinteticamente expressiva, com operadores de um caractere e uma sintaxe projetada para ser densa, mas interpretável.

Apesar do caráter experimental, o projeto inclui um interpretador escrito em C com pouquíssimas dependências, uma suite de testes própria, compatibilidade com Brainfuck, suporte a execução via WebAssembly, e um sistema de build e testes automatizados. O interpretador pode rodar tanto no terminal quanto no navegador, e foi projetado para permitir execução por etapas — o que abre espaço para rodar programas em dispositivos que não comportam o código inteiro na memória.

A \*T não é uma linguagem feita para facilitar. Ela é feita para explorar. A ideia é oferecer uma base mínima que possa ser levada a lugares estranhos e usados incomuns, onde as limitações se tornam parte do desafio.

### 1.1. Características

- Constantes numéricas e carregamento de strings
- Operadores matemáticos, lógicos e bitwise
- Comentários multi-linha e de linha única
- Suporte a inteiros sem sinal de 8, 16, 32 bits e floats de 32 bits
- Estruturas como if-else e while com suporte a continue e break
- Operadores de pilha
- Declaração e execução de funções com escopo reutilizável
- Projetada para rodar em ambientes limitados, como gadgets e microcontroladores
- Execução dinâmica de código armazenado em memória
- Sistema de tipos numéricos flexível com troca dinâmica
- Ferramentas de depuração para análise e execução em ambientes restritos
- Extensibilidade do interpretador com funções externas
- Compatível com geração e execução de código em tempo de execução
- Compatível com códigos em Brainfuck

```st
9!>0!>1!?=[2<1-?!2>;<@>+] ;PN // Fibonacci de 9
```

Você pode experimentar a linguagem diretamente no navegador:

* [https://start-lang.github.io/core/](https://start-lang.github.io/core/)  — com editor e terminal integrados
* [https://esolangs.org/wiki/\\*T](https://esolangs.org/wiki/\*T)  — entrada na wiki da comunidade esolang

Este repositório é um ponto de partida para explorar a linguagem, compilar o interpretador, rodar exemplos e contribuir com novas ideias. A intenção é manter o projeto acessível, funcional e curioso o suficiente para inspirar experimentações com linguagens e dispositivos alternativos.

## 2. Requisitos

Para compilar e executar o interpretador da linguagem \*T localmente, é necessário ter um ambiente com ferramentas mínimas de desenvolvimento. Além da execução direta no terminal, também é possível rodar o projeto dentro de um container Docker configurado com todas as dependências, ou compilar para WebAssembly e executar via Node.js ou navegador.

### 2.1 Compilação local

O interpretador pode ser compilado em sistemas Linux ou macOS. Os requisitos básicos são:

- `make`
- `clang` (recomendado) ou `gcc`
- `python3` (opcional, para geração de SVGs)
- `valgrind` ou `leaks` (opcional, para testes de memória)

A compilação local pode ser feita diretamente com os comandos `make build-cli` ou `make build-wasm`, dependendo do ambiente alvo.

### 2.2 Execução via Docker

Para isolar o ambiente e evitar diferenças de comportamento entre plataformas, o projeto oferece suporte a execução via Docker. A imagem usada é: `aantunes/clang-wasm:latest`

Ela contém todas as ferramentas necessárias para compilar o projeto para WebAssembly, rodar testes, gerar assets e executar os principais alvos do `Makefile`. O uso do Docker é particularmente importante quando se deseja garantir reprodutibilidade nos builds, ou quando o sistema operacional local não possui os toolchains necessários.

A execução via Docker é feita com os targets que começam com `docker-run-`, por exemplo:

```bash
make docker-run-test
make docker-run-build-wasm
```

Importante: evite alternar entre compilações locais e dockerizadas sem rodar make clean entre elas. Isso evita conflitos causados por diferenças de plataforma no binário gerado.

### 2.3 Execução com Node.js (WebAssembly)

Para rodar os binários .wasm do interpretador localmente, é necessário ter o Node.js instalado. O runtime wasm_runtime.js é compatível tanto com Node quanto com navegadores, e pode ser usado para testar a versão WebAssembly da linguagem:

```bash
node targets/web/wasm_runtime.js build/start.wasm '"Hello, World!" PS'
```

O uso do Node é útil apenas para validar a compatibilidade da versão WebAssembly com a versão nativa em C, já que ambas compartilham a mesma base de código.

## 3. Como compilar e executar

A linguagem \*T pode ser compilada tanto para ambientes de linha de comando quanto para WebAssembly. O projeto foi organizado para permitir o uso direto via terminal, a execução em navegadores modernos ou até em dispositivos embarcados. Essa seção cobre as formas principais de compilar e executar o interpretador, tanto localmente quanto via Docker, explicando os cuidados necessários e as decisões de design que motivaram essa estrutura.

### 3.1. Compilação local

Para compilar localmente, o projeto exige um compilador compatível com C99. A princípio, tanto `gcc` quanto `clang` funcionam, mas o suporte ao WebAssembly depende do uso do `clang`. Por padrão, o Makefile usa `clang`, e em sistemas onde ele não estiver disponível, você pode forçar o uso do `gcc` com:

```bash
make build-cli-gcc
```

Os principais targets de build são:

- `build-cli`: compila o interpretador de linha de comando
- `build-test`: compila os testes automatizados
- `build-benchmark`: compila o interpretador com parâmetros específicos para medir performance
- `build-wasm`: gera o binário WebAssembly compatível com navegadores e Node.js

Se você alternar entre builds feitos localmente e builds feitos via Docker, é importante rodar `make clean` antes de recompilar. Isso evita conflitos de arquitetura ou inconsistências entre toolchains. Compilar parte do projeto com bibliotecas do sistema e outra parte dentro do container pode resultar em erros difíceis de diagnosticar, especialmente no caso do WebAssembly.

### 3.2. Execução do interpretador CLI

Depois de compilar com `make build-cli`, o interpretador estará disponível em `build/start`.

A linha de comando aceita código como argumento direto, leitura de arquivos `.st` e também entrada via `stdin`. Alguns exemplos:

```bash
./build/start '"Hello, World!" PS'
./build/start -f tests/quine.st
echo '>,[>,]<[.<]' | ./build/start -
```

Você pode passar opções adicionais para ativar modos de debug, limitar o número de passos ou restringir a saída. Os principais parâmetros estão descritos na sessão 3.3..

## 3.3. Parâmetros do interpretador CLI

O interpretador de linha de comando aceita diversos parâmetros que controlam o modo de execução, ativam depuração, limitam recursos ou alteram o comportamento padrão. A tabela abaixo resume os principais:

| Parâmetro           | Descrição                                                                 |
|---------------------|---------------------------------------------------------------------------|
| `-h`, `--help`       | Exibe a ajuda com a descrição dos parâmetros e exemplos de uso           |
| `-f FILE`           | Lê o código a ser executado a partir de um arquivo `.st`                  |
| `-` ou `--stdin`    | Lê o código diretamente da entrada padrão                                 |
| `-d`, `--debug`     | Ativa o modo de depuração por passos, com saída textual a cada operação   |
| `-i`, `--interactive` | Entra no modo interativo com interface visual no terminal                |
| `-e`, `--exec-info` | Exibe informações de execução ao final (tempo, memória, número de passos) |
| `-S N`              | Define um limite de passos, em milhares (ex: `-S 10` = 10.000 passos)     |
| `-O N`              | Define um limite de saída textual, em caracteres                         |
| `-t N`              | Define um tempo máximo de execução, em segundos (ex: `-t 5` = 5s)         |
| `-s`                | Mostra o código parseado antes de executar                               |
| `-v`, `--version`   | Exibe a versão do interpretador                                          |

Esses parâmetros podem ser combinados livremente. Alguns exemplos práticos:

- Executar código com visualização interativa:

  ```bash
  ./build/start -i '"Hello, World!" PS'
  ```

- Rodar um arquivo com limite de passos e saída:

  ```bash
  ./build/start -f tests/quine.st -S 50 -O 100
  ```

- Enviar código via `stdin` com debug:

  ```bash
  echo '8!>0!>1!?=[2<1-?!2>;<@>+] ;PN' | ./build/start -d -
  ```

Os valores padrão, caso não especificados, são:

- `max_steps`: 2.500.000 passos
- `max_output`: ilimitado
- `timeout`: 3 segundos (exceto em debug)

## 4. Testes

A linguagem \*T inclui uma suíte de testes própria, desenvolvida com o objetivo de validar tanto o comportamento da linguagem quanto a robustez da implementação do interpretador. O sistema foi pensado para garantir portabilidade entre diferentes ambientes de execução, incluindo versões compiladas com `clang`, `gcc` e WebAssembly, e para facilitar a adição de novos testes conforme a linguagem evolui.

Os testes cobrem desde operações simples e operadores individuais até casos mais complexos como recursão, uso de memória, execução em chunks e entrada/saída via arquivos e `stdin`.

### 4.1. Visão geral da suíte de testes

A base dos testes unitários está na biblioteca MicroCuts (Micro C Unit Test Suite), incluída no projeto como submódulo. Ela fornece macros de asserção e utilitários de verificação, usados principalmente no arquivo `tests/unit/lang_assertions.c`.

Esse arquivo centraliza os testes de semântica da linguagem. Nele são validados:

- Todos os operadores principais da linguagem
- Tipos numéricos (inteiros e float)
- Comportamento do interpretador frente a erros
- Execução por step (um operador por vez)
- Alocação e liberação de memória
- Definição e escopo de variáveis
- Criação e chamada de funções
- Leitura de arquivos e execução por chunk

Além dos testes unitários, há diretórios específicos para outros tipos de teste:

- `tests/cli/`: testes da interface de linha de comando com diferentes combinações de entrada/saída
- `tests/bf/`: programas em Brainfuck usados para validar compatibilidade e performance
- `tests/mandelbrot/`: comparação entre versões em C e \*T de um gerador de fractais
- `tests/unit/`: assertions e validações internas, usando MicroCuts

### 4.2. Targets de testes

A seguir estão os principais comandos de teste disponíveis via Makefile:

- `make test`
  Executa os testes principais da linguagem em modo rápido. Esse target é usado pelo CI e inclui cobertura, execução da CLI e verificação de execução via WebAssembly (Node.js).

- `make test-long`
  Executa o mesmo conjunto de testes do target `test`, seguido de um benchmark completo com os interpretadores compilados com `gcc`, `clang` e WebAssembly. O resultado do benchmark é salvo em `build/benchmark.out`.

- `make test-full`
  Executa o target `test-long` seguido de uma análise de memória com `valgrind` (Linux) ou `leaks` (macOS). Ideal para detectar vazamentos, comportamento indefinido ou operações inválidas de memória.

- `make test-cli`
  Executa todos os testes localizados em `tests/cli/`. Cada teste possui um arquivo `.st` com o código fonte, e pode ter arquivos `.in` (entrada) e `.out` (saída esperada). O interpretador é executado e o resultado é comparado linha a linha. Qualquer divergência invalida o teste.

- `make test-wasm-cli`
  Executa um teste simples em WebAssembly usando o interpretador compilado como `.wasm`, rodando com Node.js. O objetivo é validar se a versão WebAssembly gera o mesmo resultado de saída que a versão em C.

- `make test-wasm-example`
  Roda um teste mais completo em WebAssembly, com execução de um código de exemplo no ambiente web simulado (`targets/web/example.js`).

- `make test-quick`
  Executa rapidamente o binário de teste previamente compilado. Útil para reexecutar após pequenas alterações no código sem recompilar.

### 4.3. Testes com entrada, arquivos e STDIN

Os testes de linha de comando cobrem diferentes formas de entrada:

- Código passado como argumento direto:

  ```bash
  ./build/start '"Hello, World!" PS'
  ```

- Código lido de arquivo `.st` com `-f`:

  ```bash
  ./build/start -f tests/quine.st
  ```

- Código lido via entrada padrão (`stdin`) com `-`:

  ```bash
  echo '>,[>,]<[.<]' | ./build/start -
  ```

Além disso, alguns testes envolvem entrada interativa (simulada) para validar códigos que dependem do input do usuário, como:

- `tests/bf/pi.st`: calcula dígitos de pi a partir de um número lido da entrada
- `tests/bf/calc.st`: realiza operações com números lidos da entrada
- `tests/cli/*.in`: fornecem entradas específicas para diferentes códigos de teste

Esse conjunto de testes garante que o interpretador funcione corretamente tanto com entrada inline quanto com arquivos, que interprete corretamente os operadores da linguagem, e que mantenha a compatibilidade com códigos legados em Brainfuck.

## 5. Docker e isolamento do ambiente

Para garantir reprodutibilidade e isolamento do ambiente de compilação, o projeto inclui suporte a Docker. A imagem utilizada encapsula ferramentas necessárias para compilar, testar e gerar os binários da linguagem \*T, incluindo toolchains específicos como `clang`, utilitários para WebAssembly, ferramentas de benchmarking e scripts auxiliares.

Essa abordagem tem dois objetivos principais:

1. Evitar discrepâncias entre diferentes sistemas operacionais, versões de compiladores ou dependências nativas.
2. Permitir que todo o processo de build e teste rode de forma previsível, seja em máquinas locais ou no CI/CD.

### 5.1. Targets disponíveis via Docker

Os principais comandos usando Docker são:

- `make docker-run-test`
  Executa os testes principais da linguagem, usando o ambiente isolado.

- `make docker-run-build-cli`
  Compila o interpretador.

- `make docker-run-build-wasm`
  Compila o interpretador para WebAssembly dentro do container.

- `make docker-run-test-mandelbrot`
  Executa o examplo que desenha o fractal de mandelbrot.

> É importante evitar misturar builds feitos localmente com builds feitos via Docker. Como os binários gerados podem ser diferentes (por conta do compilador, flags ou ambiente), o ideal é sempre rodar `make clean` antes de mudar entre os dois modos.

## 6. Estrutura do repositório

O repositório é organizado da seguinte forma:

- **`.github/workflows/`**
  Contém os arquivos do GitHub Actions responsáveis por rodar testes automatizados, builds e publicação do site. São utilizados principalmente nos processos de CI/CD.

- **`assets/`**
  Guarda arquivos estáticos utilizados na interface web e durante o deploy.

- **`build/`**
  Diretório temporário gerado durante a execução de comandos do Makefile. É onde ficam os binários compilados, arquivos intermediários, resultados de testes e os assets prontos para publicação. Pode ser limpo com o target `make clean`.

- **`doc/`**
  Contém a documentação do projeto em formato Markdown, escrita em português e inglês. Esses arquivos são usados no site para oferecer referências sobre a linguagem, exemplos, explicações dos comandos e estruturas da \*T.

- **`lib/`**
  Contém bibliotecas externas e internas utilizadas pelo projeto. Há três principais:
  - `microcuts/` (Micro C Unit Test Suite): biblioteca de testes baseada em assertions.
  - `wunstd/` (Web Unstandard): biblioteca com implementações simplificadas de funções padrão do C (`stdlib`, `string`, etc) voltada para uso em WebAssembly.
  - `tools/`: biblioteca interna com funções auxiliares utilizadas no interpretador e nos testes.

- **`src/`**
  Diretório principal da linguagem. Contém:
  - `start_lang.c`: implementação do interpretador e das estruturas principais.
  - `start_lang.h`: definição das estruturas, constantes, macros e tipos usados no interpretador.
  - `start_tokens.h`: lista de operadores reconhecidos pela linguagem.

- **`targets/`**
  Contém pontos de entrada específicos para diferentes plataformas:
  - `desktop/cli.c`: CLI principal usada para compilar e rodar a linguagem em sistemas Linux/macOS.
  - `web/`: arquivos utilizados na versão WebAssembly. Inclui o `wasm_runtime.js`, usado tanto em navegadores quanto no Node.js.
  - `gfx/`: (futuro) implementação gráfica com SDL, voltada para execução visual em dispositivos como o RG35XX.
  - `arduino/`: (futuro) entrada para compilar a linguagem usando o Arduino CLI em microcontroladores como ESP32, RP2040, Teensy e outros.

- **`tests/`**
  Diretório com os testes da linguagem, organizados por tipo:
  - `bf/`: testes de compatibilidade com Brainfuck, incluindo um conversor legado que hoje é redundante.
  - `cli/`: testes automatizados da CLI. Incluem arquivos `.st`, `.in` e `.out` usados para validar comportamento.
  - `mandelbrot/`: código em \*T e em C para comparação direta. O código em C foi manualmente transpilado para \*T mantendo a estrutura.
  - `unit/`: testes de unidade da linguagem, a função `validate` do arquivo `lang_assertions.c` cobre operadores, códigos de erro, execução por chunks, alocação dinâmica, criação de variáveis e funções.

## 7. Particularidades do interpretador

Uma das principais características do interpretador da linguagem \*T é sua capacidade de executar instruções de forma incremental, por meio de um sistema de execução por etapas (step). Isso significa que, em vez de processar o código inteiro de uma vez, o interpretador pode analisar e executar um único operador por vez, e retornar imediatamente após essa operação. Essa abordagem foi pensada desde o início como uma forma de permitir que a linguagem pudesse rodar em dispositivos com recursos extremamente restritos.

A ideia é que o código fonte possa ser mantido fora da memória principal — por exemplo, armazenado em um sistema externo de arquivos, EEPROM, ou mesmo transferido por comunicação serial — e carregado em pequenos blocos (chunks) conforme a necessidade. O interpretador mantém seu estado interno entre os passos e é capaz de indicar se precisa continuar lendo o chunk atual, avançar para o próximo, ou retroceder para o anterior. Essa forma de controle permite que o sistema use uma quantidade mínima de RAM.

Esse mecanismo foi pensado durante o desenvolvimento com foco em ambientes como ATmega328, ATmega8 e microcontroladores similares. O código final do interpretador ainda não foi portado com sucesso para todos esses dispositivos, mas a arquitetura foi desenhada levando isso em conta. Toda a lógica de execução, os testes de alocação dinâmica e os limites de comportamento foram estruturados com a ideia de que a execução parcial do código deve sempre ser possível.

## 8. Contribuindo com o projeto

Este projeto está aberto a contribuições. O objetivo é que qualquer pessoa interessada em linguagens, interpretadores ou dispositivos com recursos limitados possa experimentar, modificar, propor mudanças e compartilhar ideias.

Se você quiser contribuir, aqui vão algumas orientações para manter o repositório organizado e facilitar o processo de revisão:

- **Faça commits pequenos e objetivos**
  Evite agrupar várias mudanças em um único commit. Prefira enviar alterações de forma gradual, sempre que possível com uma descrição clara do que foi feito.

- **Inclua testes ao alterar o comportamento da linguagem**
  Se você estiver modificando um operador, adicionando uma nova função ou ajustando a sintaxe, crie testes cobrindo o novo comportamento. A pasta `tests/` já está estruturada para isso, com exemplos em `unit/`, `cli/` e `bf/`.

- **Rode os testes antes de enviar**
  Use os targets `make test` e `make memcheck` (ou `make test-full`) para verificar se a linguagem continua funcionando corretamente.

O projeto está aberto a sugestões, correções, experimentações e contribuições de diferentes níveis de complexidade. Mesmo pequenas melhorias no código, testes ou documentação são bem-vindas. Se tiver dúvidas sobre por onde começar, entre em contato por meio das issues ou abra um pull request com o que você estiver experimentando.

## 9. Agradecimentos

Alguns projetos e pessoas foram fundamentais para que a linguagem \*T tomasse forma:

- [**esolangs.org**](https://esolangs.org/), que foi uma porta de entrada para o universo das linguagens esotéricas.
- [**Urban Müller**](https://en.wikipedia.org/wiki/Brainfuck), criador do Brainfuck.
- [**Andy Wingo**](https://github.com/wingo/walloc), pelo `walloc` usado na `wunstd`.
- [**Nathan Friedly**](https://github.com/nfriedly/miyoo-toolchain), pela publicação da imagem do Docker que tornou possível portar a linguagem para o RG35XX (no `main` em breve).
