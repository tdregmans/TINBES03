#define CHAR 1 // 1-byte // plaats op stack (eerst de waarde, dan het type)
#define INT 2 // 2-byte // plaats op stack (eerst de waarde, dan het type)
#define STRING 3 // n-byte (zero-terminated) // plaats op stack (eerst de string, dan de lengte (byte), dan het type)
#define FLOAT 4 // 4-byte // plaats op stack (eerst de waarde, dan het type)
#define SET 5 // name x // zet in geheugen (name is 1 byte, volgt direct op opcode, niet van stack)
#define GET 6 // name // haal uit geheugen en plaats op stack
#define INCREMENT 7 // x
#define DECREMENT 8 // x
#define PLUS 9 // x y
#define MINUS 10 // x y 
#define TIMES 11 // x y
#define DIVIDEDBY 12 // x y
#define MODULUS 13 // x y
#define UNARYMINUS 14 // x
#define EQUALS 15 // x y
#define NOTEQUALS 16 // x y
#define LESSTHAN 17 // x y
#define LESSTHANOREQUALS 18 // x y
#define GREATERTHAN 19 // x y
#define GREATERTHANOREQUALS 20 // x y
#define LOGICALAND 21 // x y
#define LOGICALOR 22 // x y
#define LOGICALXOR 23 // x y
#define LOGICALNOT 24 // x
#define BITWISEAND 25 // x y
#define BITWISEOR 26 // x y
#define BITWISEXOR 27 // x y
#define BITWISENOT 28 // x
#define TOCHAR 29 // x
#define TOINT 30 // x
#define TOFLOAT 31 // x
#define ROUND 32 // x
#define FLOOR 33 // x
#define CEIL 34 // x
#define MIN 35 // x y
#define MAX 36 // x y
#define ABS 37 // x
#define CONSTRAIN 38 // x a b
#define MAP 39 // value fromLow fromHigh toLow toHigh
#define POW 40 // base exponent
#define SQ 41 // x
#define SQRT 42 // x
#define DELAY 43 // millis
#define DELAYUNTIL 44 // millis
#define MILLIS 45
#define PINMODE 46 // x y
#define ANALOGREAD 47 // pin
#define ANALOGWRITE 48 // pin val
#define DIGITALREAD 49 // pin
#define DIGITALWRITE 50 // pin val
#define PRINT 51 // data // output naar console
#define PRINTLN 52 // data // output naar console
#define OPEN 53 // name length // name is zero-terminated string
#define CLOSE 54
#define WRITE 55 // data // output naar file
#define READINT 56 // lees van file
#define READCHAR 57 // lees van file
#define READFLOAT 58 // lees van file
#define READSTRING 59 // lees van file
#define IF 128 // lengthOfTrueCode // springt lengthOfTrueCode verder als 0 op stack. waarde blijft staan op stack.
#define ELSE 129 // lengthOfFalseCode // springt lengthOfFalseCode verder als niet 0 op stack. waarde blijft staan op stack.
#define ENDIF 130 // popt 1 waarde van de stack
#define WHILE 131 // lengthOfConditionCode lengthOfRepeatedCode // popt 1 waarde. springt lengthOfRepeatedCode + 1 verder als waarde 0. zoniet, pusht lengthOfConditionCode + lengthOfRepeatedCode + 4 op stack
#define ENDWHILE 132 // popt 1 waarde en springt zoveel plaatsen terug (naar begin condition code)
#define LOOP 133 // geeft aan dat hier setup() eindigt en loop() start
#define ENDLOOP 134
#define STOP 135
#define FORK 136 // name // start nieuw process, geeft process ID terug
#define WAITUNTILDONE 137 // processID
