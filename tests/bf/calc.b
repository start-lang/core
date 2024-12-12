# calc.b -- compute sum AA+BB (c) 2019 Antonio Ferreras https://github.com/AntonioFerreras
# init: 200m
, ------------------------------------------------ First digit of first number on 0
>>, ------------------------------------------------ Second Digit of second number on 2
build whole first number
< Go to 1 (Holds ten)
++++++++++
< go to 0
[>[>+>+<<-]>>[<<+>>-]<<<-]>[-]< multiply first digit by ten and add two to make number on cell 2
, ------------------------------------------ get plus symbol and then remove it
>>>, ------------------------------------------------ Store first digit of second number on 3
>>, ------------------------------------------------ Store second digit of second number on 5
build 2nd number
< Go to 4 (Holds ten)
++++++++++
< go to 3
[>[>+>+<<-]>>[<<+>>-]<<<-]>[-]< multiply first digit by ten and add two to make number on cell 5
< go to 2
[->>>+<<<]Add together the two numbers in 2 and 5
>>> go to 5
print number
[>>+>+<<<-]>>>[<<<+>>>-]<<+>[<->[>++++++++++<[->-[>+>>]>[+[-<+>]>+>>]<<<<<
]>[-]++++++++[<++++++>-]>[<<+>>-]>[<<+>>-]<<]>]<[->>++++++++[<++++++>-]]<[.[-]<]<