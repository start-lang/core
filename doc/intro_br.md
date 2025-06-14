
\*T é uma linguagem de programação esotérica, estruturada e interpretada que busca oferecer um desafio criativo e um paradigma alternativo para quem deseja explorar os limites da programação. A maior parte dos operadores possui apenas um caractere, permitindo uma sintaxe compacta. No \*T, o foco está em criar códigos quase cifrados e brincar com novas formas de resolver problemas. A ideia é que a linguagem valorize mais o processo e os desafios encontrados ao programar, onde o lema é "se funcionar já está bom". A legibilidade é parte da forma de exploração, que envolve um jogo de decifrar para seguir em frente.

Um dos principais diferenciais do  \*T é sua portabilidade, o interpretador foi escrito em C com pouquíssimas dependências permitindo experimentações em ambientes com recursos limitados. Apesar das limitações de linguagens interpretadas, o \*T oferece um ambiente funcional interessante para desafios computacionais em plataformas diversas. A biblioteca de funções padrão da linguagem ainda está sendo padronizada e pode variar de acordo com o ambiente ou dispositivo, um conjunto de funções que sejam simples de implementar em diferentes plataformas está sendo pensado de forma que permitam a criação de programas interessantes, `.` (`PC` ou `PRINT`) e `,` (`IN` ou `INPUT`) permitem entrada e saída na forma textual e devem estar presentes em todos os ambientes.

No final dessa página, um interpretador permite experimentar com as capacidades da linguagem e explorar sua sintaxe compacta. Fora o interpretador online o código pode ser compilado em um executável em diversas plataformas. A ideia é portar o projeto para gadgets portáteis que possam ser programados diretamente no dispositivo. Cada ambiente de desenvolvimento apresenta um desafio diferente de usabilidade para o interpretador, principalmente quando consideramos que nem todos os dispositivos vão possuir um teclado completo ou uma tela com boa resolução, é nessas limitações que torna o desafio interessante.

Este projeto foi desenvolvido tanto para quem quer um desafio ao programar e experimentar os limites do que se pode fazer dentro de um ambiente limitado, quanto para quem quer criar dispositivos ou portar a linguagem para novos ambientes, tentando otimizar a usabilidade e performance em cada contexto.

```st
"Hello, World!" PS
```

> Os exemplos dessa página são executáveis - ao passar o mouse por cima deles eles são selecionados e basta expandir o terminal oculto no final da página e clicar na opção de execução desejada: `Run` apenas executa e `Debug` realiza as operações passo a passo mostrado informações sobre o estado do interpretador. Para passar para a próxima instrução deve-se pressionar `espaço` ou `enter`.

```st
9!>0!>1!?=[2<1-?!2>;<@>+] ;PN // Calcula o fibonacci de 9
```

Recentemente, a linguagem foi adaptada para ser compatível com Brainfuck, e por ser uma das linguagens esotéricas mais populares os conceitos e técnicas básicas podem ser reaproveitados ao programar em \*T. Códigos em bf podem ser executados da mesma forma sem adaptação. Se é a primeira vez que você explora uma esolang similar ao bf, abaixo se encontra uma breve introdução.

```st
>,[>,]<[.<] // Inverte a string de entrada
```

# Características

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

# Conceitos básicos

O principal elemento em comum com o bf é a ideia de representar a memória como uma fita/array constituída de "casas"/bytes, a leitura e manipulação dos valores dessa fita é feita por meio de uma cabeça de leitura/ponteiro, que recebe instruções do código fonte e realiza ações de acordo. Existem instruções para deslocar a fita para frente ou para trás, selecionando novas regiões, realizar operações matemáticas e controlar o fluxo do programa.

Todo programa começa na posição zero da memória e pode selecionar outras posições usando os operadores `>` e `<`. Constantes numéricas podem ser usadas precedendo os operadores, caso não exista uma constante em uso o valor 1 é assumido.

**Exemplo: andar 3 bytes para direita**
  - em bf: ```>>>```
  - em \*T: ```3>```

A cabeça de leitura possui algumas características próprias em \*T, ela guarda um registrador capaz de armazenar valores, tamanho do tipo em uso e resultados de operações lógicas. Esses valores podem ser usados para simplificar algumas operações. Quando uma constante numérica é processada, ela fica armazenada no registrador, podendo ou não ser movida para a fita de memória usando o operador `!`. Da mesma forma valores podem ser carregados no registrador usando `;`, ou mesmo trocados ente registrador e memória usando `@`.

Tipos numéricos diferentes são suportados em \*T, os tipos inteiros podem ter 3 tamanhos: 8, 16 e 32 bits, todos sem sinal. Além dos tipos inteiros o tipo float de 32 bits também pode ser usado. Por padrão o tipo inteiro de 8 bits é usado, para alterar o tipo pode-se usar os operadores: `b` (byte - uint8), `s` (short - uint16), `i` (int - uint32) e `f` (float).

Ao alterar o tipo numérico em uso os operadores de deslocamento vão movimentar a fita pelo tamanho em bytes de cada tipo: `b` - 1 byte, `s` - 2 bytes e 4 bytes para `i` e `f`.

**Exemplo: tipos e deslocamento**
  - ```32!>``` ou ```b32!>``` - guarda 32 no primeiro byte e se move para o segundo
  - ```s256!>``` - guarda 256 usando os dois primeiros bytes e se move para o terceiro
  - ```i20000!>``` - guarda o valor usando os 4 primeiros bytes e se move para o quinto
  - ```f3.1415!>``` - guarda o valor usando os 4 primeiros bytes e se move para o quinto

Para entrada e saída são usados os operadores `,` e `.`, respectivamente. `,` permite ler um caractere e guardar na posição atual da fita. `.` imprime o valor da posição atual como um caractere ASCII. Além disso no \*T a função `PN`/`PRINTNUM` pode ser útil para imprimir o valor em decimal.

**Exemplo: imprimir o número 7**
  - em bf: ``-[----->+<]>++++.`` ou (55 repetições de `+` com `.` ao final)
  - em \*T: ``7+ PN``

Enquanto no bf apenas os operadores `+` e `-` estão presentes, o \*T possui também `*`, `/` e `%`, e permitem usar constantes numéricas. Dessa forma algumas operações são simplificadas.

**Exemplo: "3 + 4"**
  - em bf: ```+++ ++++```
  - em \*T: ```3+ 4+```

**Exemplo: "2 * 3"**
  - em bf: ```++[>+++<-]```
    - veja que é necessário usar um laço de repetição para realizar uma multiplicação, divisões são ainda mais trabalhosas de "emular"
  - em \*T: ```2+ 3*``` ou ```2@3*``` (usando switch) ou ```2p3o*``` (usando operadores de pilha)

Um detalhe que vale a pena mostrar é o uso de comentários. Quando `//` é encontrado o programa pula para a próxima quebra de linha, e quando `/*` é encontrado ele pula para o próximo `*/`, permitindo ter comentários com mais de uma linha ou comentários dentro da mesma linha.

Strings podem ser carregadas na fita usando `"`, ao escrever `"Hello World"`, para cada caractere, o valor correspondente da tabela ASCII é carregado, ao final a fita é movimentada para o início da string, permitindo usar a função `PS`/`PRINTSTRING` para imprimir os valores em ASCII até chegar em um valor igual a zero. Opcionalmente é possível usar o operador `>` ao final da string na forma `"Hello World">` para evitar retornar ao início da string e continuar a usar valores válidos da fita.

**Exemplo: Comentários e Strings**
  - Comenta até o final da linha:
```st
// Meu primeiro programa
"Hello World!" PS
// Fim
```
  - Comentário com inicio e fim:
```st
/*
   Meu primeiro programa
*/
"Hello World!" /* String */ PS /* Função PRINTSTRING */
/* Fim */
```
 - Concatena duas Strings usando identificadores:
```st
S^ /* marca o inicio */ "Hello">
// vai ao final da string e ignora o valor 0 retornando uma posição
<" World!" S /* pula para a posição S */ PS /* imprime a string */
```

Laços de repetição são definidos por `[` e `]` sendo que a repetição apenas acontece quando foi realizada uma operação lógica com resultado positivo. As operações lógicas são realizadas pelo operador `?` seguido da operação. As operações lógicas são sempre realizadas entre o valor da fita e o valor do registrador.

**Operadores lógicos**
  - `t` - verdade
  - `?>` - maior que
  - `?<` - menor que
  - `?=` - igual
  - `?!` - diferente
  - `?l` - menor ou igual
  - `?g` - maior ou igual
  - `??` - diferente de zero
  - `?z` - igual a zero
  - `h` - verdadeiro caso a pilha seja maior que zero

> Caso um operador lógico não seja usado antes do inicio ou final de um laço de repetição o valor da posição da memória atual é usada, sendo que se for diferente de zero o laço deve repetir. Possuindo o mesmo comportamento que o bf.

Condicionais podem ser utilizadas para executar blocos de código caso a operação lógica seja verdadeira ou falsa (if/else) utilizando `(` `se_verdade` `)` ou `(` `se_verdade` `:` `se_falso` `)` .

**Exemplo: Condicional e função RAND**
  - Gera um número aleatório e compara com o número 5, imprimindo se é maior ou menor/igual
  - ``RAND!10% ;PN 5?>(" > 5":" <= 5") PS``

Para simplificar o gerenciamento de memória, *T oferece uma forma de mapear posições da fita a um identificador específico, emulando o conceito de variáveis. Os identificadores consistem em uma ou mais letras maiúsculas, podendo ser opcionalmente intercalados com underlines e números. A declaração é feita usando `ID^` onde ID é o identificador. Após a declaração, basta usar o identificador sozinho sempre que quiser acessar a posição correspondente.

Uma vantagem ao debugar o código é que o tipo em uso também é registrado (apenas no debugger), permitindo visualizar o valor da variável de forma mais intuitiva, especialmente quando são usados tipos diferentes do padrão (byte).

Identificadores também podem ser usados para chamar funções da stdlib da linguagem, como:
	•	PN/PRINTNUM — imprime um número.
	•	PS/PRINTSTRING — imprime uma string.
	•	RAND — gera um número aleatório.


# Resumo

- NOP - Nenhuma operação
  - Qualquer caractere em branco (espaço, tab e quebra de linha) é ignorado
- Constante numérica
  - Uma sequência de números é sempre guardada no registrador
  - Existem 4 comandos usados para definir o tipo em uso:
    - `b` 8bits sem sinal - padrão
    - `s` 16bits sem sinal
    - `i` 32bits sem sinal
    - `f` float 32bits
  - Exemplos:
    - ```123``` ```b255``` ```s350``` ```f8.76```
- Operações de memória
  - `<` - Move para a esquerda pelo tamanho do tipo
  - `>` - Move para a direita pelo tamanho do tipo
  - `!` - Copia o valor do registrador para a fita
  - `;` - Copia o valor da fita para o registrador
  - `@` - Troca o valor da fita pelo valor do registrador e vice-versa
  - `z` - Move 2 byte para a direita até encontrar um zero
  - `#` - Executa valores na fita como se fossem comandos até `0`
  - `m` - Realoca o tamanho da fita com base no registrador atual
- Operações matemáticas
  - `+` - Soma o valor do registrador com o valor na fita
  - `-` - Subtrai o valor do registrador do valor na fita
  - `*` - Multiplica o valor da fita pelo valor do registrador
  - `/` - Divide o valor da fita pelo valor do registrador
  - `%` - Guarda o resto da divisão do valor da fita pelo valor do registrador na fita
  - `w` - Introduz operações bitwise (&, |, ^, \~, <<, >>) com base no tipo ativo
- String
  - Sequência de dígitos cercados por aspas duplas `"`, aspas duplas internas devem ser precedidas de `\`
  - Sempre são salvas na fita em sequência e terminadas com o valor 0
  - Essa operação não move a fita, porém adicionando o comando `>` em sequência move a fita até o fim da string
- Identificadores
  - Qualquer sequência de letras maiúsculas representa um identificador
  - Identificadores podem conter `_` após a primeira letra
  - Caso o id seja de uma variável, então a fita é movida até ela
  - Caso o id seja de uma função, então a mesma é executada
  - No interpretador online existem 3 funções implementadas:
    - PC ou PRINT:
      - Imprime o valor do registrador como caractere
    - PS ou PRINTSTR:
      - Imprime a fita atual como string (da posição atual até o primeiro `0`)
    - PN:
      - Imprime o valor do registrador como número (depende do tipo usado)
- Variáveis
  - Posições da fita podem ser mapeadas para ids usando o operador `^` seguido do id
  - Exemplo:
    - `X^1!>2!>3!X;` - salva a primeira posição da fita como o id `X`, salva `1` na primeira casa, `2` na segunda e `3` na terceira, volta até a primeira casa usando o nome do id e carrega o valor da posição no registrador, que é `1`
- Comparações
  - Existe um registrador exclusivo para comparações que é usado pelas operações if e while
  - A comparação é feita entre a fita (esquerda) e o registrador (direita)
    - `t` - define o valor do registrador exclusivo como verdadeiro
    - `?>` - maior que
    - `?<` - menor que
    - `?=` - igual
    - `?!` - diferente
    - `?l` - menor ou igual
    - `?g` - maior ou igual
    - `??` - diferente de zero
    - `?z` - igual a zero
    - `?h` - verdadeiro caso a pilha seja maior que zero
    - `~` - inverte o valor do registrador exclusivo
  - Exemplos:
    - `0!1?<` - `0` é menor que `1`? Verdadeiro
    - `1!1?<` - `1` é menor que `1`? Falso
    - `2!1?<` - `2` é menor que `1`? Falso
- If
  - Sintaxe: `(` `se_verdade` `)` ou `(` `se_verdade` `:` `se_falso` `)`
  - Usa o valor do registrador de comparações para definir se o primeiro ou segundo bloco deve ser executado
  - O primeiro bloco é sempre executado caso o valor seja `1`
  - O segundo bloco é executado caso o valor seja `0` e o bloco não tenha sido omitido
  - Exemplo:
    - `2!1?>(2:3)!` - salva `2` caso `2` seja maior que `1` caso contrario salve `3`, portanto `2` é salvo
- While
  - Sintaxe: `[` `bloco` `]`
  - `[` - entra no loop caso o valor do registrador de comparações seja `1`
  - `]` - repete caso o valor do registrador de comparações seja `1`
  - `c` - *continue* - volta ao início do loop
  - `x` - *break* - encerra o loop
- Pilha
  - `p` e `o` são equivalentes a `!>` e `;<` respectivamente, porém a altura da pilha pode ser consultada usado `h` e a condição `?h` também está disponível.
  - `p` - insere um item na pilha, movendo para a direita pelo tamanho do tipo
  - `o` - remove um item da pilha, movendo para a esquerda pelo tamanho do tipo
  - `h` - guarda o tamanho da pilha no registrador
- Funções
  - Além de mapear posições da fita, ids podem ser usados para declarar funções
  - Sintaxe: `NOME` `{` `código` `}`
  - `r` - retorna a função
- Comentários
  - Podem ser usados comentários multi-linha ou de apenas uma linha
  - Sintaxe: `/*` `conteúdo de várias linhas` `*/` ou `//` `resto da linha` `\n`

# Como usar

O interpretador oficial pode ser compilado para Linux ou Mac OS, uma versão compilada para WASM roda nesta página. Outros *targets* devem ser publicados nas próximas atualizações da linguagem, uma biblioteca compatível com Arduino/ESP32 (M5stack Cardputer) está em estágios iniciais e um demo simples usando SDL1.2/2 pode ser compilado para desktop e também o console portátil RG35xx.


### Exemplo: Desenhando fractal em ASCII
O exemplo abaixo foi convertido do C em *T e permite desenhar o fractal de Mandelbrot usando caracteres ASCII. Em alguns dispositivos a página pode ficar irresponsiva por alguns segundos enquanto é executado.

```st
s
LL^24!>L^!>
CC^70!>C^!>
f
CX^>
CY^>
TX^>
TY^>
TZ^>
X^>
Y^>
bI^
sL 1- [
  sCC;C! 1- [
    b0I!
    f0X!Y!
    i0sCC;efCX!3/ i0sC;efCX@/ 2-
    i0sLL;efCY!2/ i0sL;efCY@/ 1-
    bI 10! [
      f
      X;TX!*
      Y;TY!*
      0TZ! TX;TZ+ TY;TZ+ 4?> (
        x
      )
      Y 2*  X;Y*  CY;Y+
      TX;X! TY;X- CX;X+
      bI 1- ??
    ]
    bI;TZ! 38+ .
    sC 1- ??
  ]
  b10TZ! .
  sL 1- ??
]
```

Resultado esperado:

```
.........-----,,,,,,+*)(&&&()*+++,,,,,,,,-------------------..../////
.......------,,,,++++)&&&&&&&&)*+++,,,,,,,,,------------------...////
......------,,++++**)(&&&&&&&&())***++,,,,,,,,-----------------...///
....-------,+*&())(&&&&&&&&&&&&&&'&&&'*+++,,,,,,-----------------..//
...-------,++&&&&&&&&&&&&&&&&&&&&&&&&')**++++++,,,,---------------.//
..-------,,+*)&&&&&&&&&&&&&&&&&&&&&&&&')***+++++++++,,-------------./
.-------,,++((&&&&&&&&&&&&&&&&&&&&&&&&&'()))*******)&*+,,,,---------/
.-------,,+*&&&&&&&&&&&&&&&&&&&&&&&&&&&&&'((&'&&'&&&&**+++,,,,,,----.
-------,,,+*&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&')*+++++,,,,,,,--
-------,,,++)&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&'((*+++++,,,,,,-
-------,,,++*)&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&'))****+,,++,
-------,,,++*)(&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
-------,,,++*)&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&'))****+,,++,
-------,,,++)&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&'((*+++++,,,,,,-
-------,,,+*&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&')*+++++,,,,,,,--
.-------,,+*&&&&&&&&&&&&&&&&&&&&&&&&&&&&&'((&'&&'&&&&**+++,,,,,,----.
.-------,,++((&&&&&&&&&&&&&&&&&&&&&&&&&'()))*******)&*+,,,,---------/
..-------,,+*)&&&&&&&&&&&&&&&&&&&&&&&&')***+++++++++,,-------------./
...-------,++&&&&&&&&&&&&&&&&&&&&&&&&')**++++++,,,,---------------.//
....-------,+*&())(&&&&&&&&&&&&&&'&&&'*+++,,,,,,-----------------..//
......------,,++++**)(&&&&&&&&())***++,,,,,,,,-----------------...///
.......------,,,,++++)&&&&&&&&)*+++,,,,,,,,,------------------...////
.........-----,,,,,,+*)(&&&()*+++,,,,,,,,-------------------..../////
```

# REPL

Use a caixa de texto abaixo para testar o interpretador, basta digitar o código e pressionar `Run` ou `Debug` no final da página para executar.

# Limitações

- Códigos de erros ainda não estão documentados
- A versão online do interpretador as vezes precisa de um pressionamento de tecla a mais para trocar entre o modo de debug e execução

# Próximos passos

> Se você gostou da ideia visite o repositório do projeto no GitHub e dê uma estrelinha, se o projeto estiver tendo boa visibilidade eu posso dar mais atenção pra ele na correção de bugs, documentação e próximos releases <3
