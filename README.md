# F1Code

Lenguaje de programación didáctico con temática de Fórmula 1, y su compilador construido con **Flex + Bison**, con destino final a **C11**.

Proyecto del curso de **Compiladores** — Universidad San Pablo de Guatemala.

---

## Tabla de contenidos

- [Descripción](#descripción)
- [Estado del proyecto](#estado-del-proyecto)
- [Requisitos](#requisitos)
- [Compilación](#compilación)
- [Uso](#uso)
- [El lenguaje F1Code](#el-lenguaje-f1code)
  - [Palabras reservadas](#palabras-reservadas)
  - [Tipos y literales](#tipos-y-literales)
  - [Operadores y precedencia](#operadores-y-precedencia)
  - [Comentarios](#comentarios)
  - [Gramática](#gramática-bnf-simplificado)
- [Ejemplo completo](#ejemplo-completo)
- [Ejemplos inválidos](#ejemplos-inválidos)
- [Pruebas](#pruebas)
- [Estructura del repositorio](#estructura-del-repositorio)
- [Arquitectura del compilador](#arquitectura-del-compilador)
- [Decisiones de diseño](#decisiones-de-diseño)
- [Roadmap](#roadmap)
- [Autores](#autores)

---

## Descripción

F1Code es un lenguaje imperativo mínimo, de sintaxis inspirada en la Fórmula 1, diseñado para poder practicar las etapas clásicas de un compilador: análisis léxico, análisis sintáctico, construcción de AST, tabla de símbolos, validaciones semánticas y generación de código.

En lugar de `int` se escribe `engine`, en lugar de `if` se escribe `race`, en lugar de `while` se escribe `lap`. Los archivos fuente usan la extensión **`.f1`**.

```f1
garage engine hamilton := 300;

race (hamilton > 250) {
    radio hamilton;
} overtake {
    radio 0;
}

finish;
```

El lenguaje cubre 10 capacidades obligatorias: declaración de variables, asignación, expresiones aritméticas, precedencia de operadores, comparaciones, entrada de datos, salida de datos, condicional con alternativa, ciclo `while` y comentarios.

---

## Estado del proyecto

El compilador se construye por etapas. Actualmente el **front-end está funcional**.

| Etapa | Estado |
|---|---|
| Análisis léxico (Flex) | Completado |
| Análisis sintáctico (Bison) | Completado |
| Árbol de Sintaxis Abstracta (AST) | Completado |
| Tabla de símbolos | Completado |
| Validaciones semánticas | Pendiente |
| Representación intermedia (IR) | Pendiente |
| Generación de código C11 | Pendiente |

Hoy el compilador acepta un archivo `.f1`, lo analiza, imprime el AST y la tabla de símbolos, y reporta errores léxicos y sintácticos con número de línea. **Aún no valida semántica** (uso de variables no declaradas, redeclaraciones, compatibilidad de tipos) ni genera código.

---

## Requisitos

| Herramienta | Versión mínima sugerida |
|---|---|
| GCC | 9.x |
| Flex | 2.6 |
| Bison | 3.5 |
| GNU Make | 4.x |

### Instalación de dependencias

**Debian / Ubuntu**
```bash
sudo apt update
sudo apt install build-essential flex bison
```

**Fedora**
```bash
sudo dnf install gcc make flex bison
```

**macOS (Homebrew)**
```bash
brew install flex bison
```

**Windows (MSYS2 / UCRT64)**
```bash
pacman -S mingw-w64-ucrt-x86_64-gcc flex bison make
```

---

## Compilación

Un solo comando desde la raíz del repositorio:

```bash
make
```

Para reconstruir desde cero:

```bash
make clean && make
```

El proceso genera el parser con `bison -d`, el lexer con `flex`, compila las fuentes C y enlaza el ejecutable **`f1code`** en la raíz. Los archivos intermedios (`f1code.tab.c`, `lex.yy.c`, objetos `.o`) quedan en `build/` y no se versionan.

---

## Uso

```bash
./f1code <archivo.f1>
```

Ejemplo:

```bash
./f1code tests/valido.f1
```

Salida:

```
=== F1Code Front-end ===
Archivo: tests/valido.f1

>> Analisis lexico y sintactico completados sin errores.

--- Arbol de Sintaxis Abstracta (AST) ---
Program
  Decl (engine) hamilton := 300    [linea 3]
  Decl (fuel) leclerc := 50.5    [linea 4]
  Input hamilton    [linea 6]
  If (hamilton > 250)    [linea 12]
    Then:
      Output hamilton    [linea 9]
    Overtake:
      Output leclerc    [linea 11]
  While (hamilton > 200)    [linea 19]
    Boost hamilton += -(10)    [linea 17]
    Output hamilton    [linea 18]
  Finish    [linea 21]

--- Tabla de simbolos ---
Nombre          Tipo      Linea
------          ----      -----
hamilton        engine    3
leclerc         fuel      4
```

Ante un error, el compilador reporta la línea y el token donde se detectó el problema, y aborta:

```
Error sintactico en linea 2: syntax error (cerca de 'hamilton')

>> Compilacion ABORTADA: 0 error(es) lexico(s), 1 error(es) sintactico(s).
```

El código de salida es `0` si la compilación fue exitosa y distinto de `0` si hubo errores.

---

## El lenguaje F1Code

### Palabras reservadas

| Token | Significado |
|---|---|
| `garage` | Inicia la declaración de una variable |
| `engine` | Tipo entero |
| `fuel` | Tipo real (decimal) |
| `pitstop` | Entrada de datos |
| `radio` | Salida de datos |
| `race` | Condicional (`if`) |
| `overtake` | Alternativa (`else`) |
| `lap` | Ciclo (`while`) |
| `boost` | Incremento compuesto: `id boost expr` ≡ `id := id + expr` |
| `finish` | Fin explícito del programa |

### Tipos y literales

| Categoría | Patrón | Ejemplo |
|---|---|---|
| Identificador | `[a-zA-Z_][a-zA-Z0-9_]*` | `hamilton`, `_vel2` |
| Entero (`engine`) | `[0-9]+` | `300` |
| Real (`fuel`) | `[0-9]+\.[0-9]+` | `50.5` |

Un identificador **no puede iniciar con dígito**: `1verstappen` produce un error léxico.

### Operadores y precedencia

| Nivel | Operadores | Asociatividad |
|---|---|---|
| 1 (más alta) | `( )` | — |
| 2 | `*` `/` | Izquierda |
| 3 | `+` `-` | Izquierda |
| 4 (más baja) | `<` `>` `<=` `>=` `==` `!=` | No asociativo |

La asignación se escribe con **`:=`**, no con `=`. La igualdad se compara con `==`.

### Comentarios

```f1
// Comentario de línea, hasta el fin de la línea

/* Comentario de bloque,
   puede abarcar varias líneas */
```

Ambos se descartan en el análisis léxico y no llegan al parser.

### Gramática (BNF simplificado)

```
programa      -> lista_sent

lista_sent    -> sentencia lista_sent | ε

sentencia     -> declaracion | asignacion | incremento
               | entrada | salida
               | condicional | ciclo | fin_stmt

declaracion   -> 'garage' tipo IDENT ':=' expr ';'
tipo          -> 'engine' | 'fuel'

asignacion    -> IDENT ':=' expr ';'
incremento    -> IDENT 'boost' expr ';'
entrada       -> 'pitstop' IDENT ';'
salida        -> 'radio' expr ';'

condicional   -> 'race' '(' cond ')' '{' lista_sent '}'
                 ( 'overtake' '{' lista_sent '}' )?

ciclo         -> 'lap' '(' cond ')' '{' lista_sent '}'
fin_stmt      -> 'finish' ';'

cond          -> expr OP_COMP expr
OP_COMP       -> '<' | '>' | '<=' | '>=' | '==' | '!='

expr          -> expr '+' term | expr '-' term | term
term          -> term '*' factor | term '/' factor | factor
factor        -> ENTERO | REAL | IDENT | '(' expr ')'
```

Las **llaves `{ }` son obligatorias** en todos los bloques. No existe cuerpo de una sola sentencia sin llaves.

---

## Ejemplo completo

`tests/valido.f1`:

```f1
// Programa demostrativo de F1Code
// hamilton = velocidad en km/h, leclerc = combustible restante
garage engine hamilton := 300;
garage fuel leclerc := 50.5;

pitstop hamilton;

race (hamilton > 250) {
    radio hamilton;
} overtake {
    radio leclerc;
}

/* Ciclo que reduce la velocidad
   hasta llegar al limite de pit lane */
lap (hamilton > 200) {
    hamilton boost -10;
    radio hamilton;
}

finish;
```

> **Nota sobre `boost`:** siempre suma. Para restar se pasa una expresión negativa, como `hamilton boost -10`, que equivale a `hamilton := hamilton + (-10)`.

---

## Ejemplos inválidos

Casos que el compilador debe rechazar, y por qué:

| Código | Error |
|---|---|
| `garage hamilton := 300;` | Falta el tipo (`engine` o `fuel`) |
| `garage engine hamilton := 300` | Falta el `;` final |
| `hamilton := ;` | Falta la expresión después de `:=` |
| `race hamilton > 250 { ... }` | Faltan los paréntesis en la condición |
| `lap (hamilton > 200) radio hamilton;` | Falta el bloque `{ }` |
| `garage engine 1verstappen := 5;` | Identificador no puede iniciar con dígito (error **léxico**) |

---

## Pruebas

El directorio `tests/` contiene programas de prueba:

| Archivo | Qué verifica |
|---|---|
| `valido.f1` | Programa que ejercita las 10 capacidades del lenguaje |
| `invalido.f1` | Declaración sin tipo — dispara error sintáctico en línea 2 |

Para correrlas:

```bash
./f1code tests/valido.f1     # debe terminar sin errores
./f1code tests/invalido.f1   # debe reportar el error y abortar
```

---

## Estructura del repositorio

```
f1code/
├── Makefile           # Build de un solo comando
├── README.md
├── include/           # Headers públicos (ast.h, symtab.h)
├── src/
│   ├── f1code.l       # Especificación Flex (analizador léxico)
│   ├── f1code.y       # Gramática Bison (analizador sintáctico)
│   ├── ast.c          # Construcción e impresión del AST
│   ├── symtab.c       # Tabla de símbolos
│   └── main.c         # Punto de entrada
├── tests/
│   ├── valido.f1
│   └── invalido.f1
└── build/             # Artefactos generados — ignorado por Git
```

---

## Arquitectura del compilador

```
Código fuente (.f1)
        │
        ▼
┌───────────────┐
│ Flex (Tokens) │  Análisis léxico: texto → tokens
└───────┬───────┘
        ▼
┌───────────────┐
│ Bison (AST)   │  Análisis sintáctico: tokens → árbol
└───────┬───────┘
        ▼
┌───────────────────┐
│ Tabla de símbolos │  Registro de variables: nombre, tipo, línea
└───────┬───────────┘
        ▼
  [ Validaciones ]     ← pendiente
        ▼
  [      IR      ]     ← pendiente
        ▼
  [  Salida C11  ]     ← pendiente
```

Cada etapa tiene una responsabilidad única y se prueba por separado. Esa separación permite cambiar el back-end sin tocar el front-end.

---

## Decisiones de diseño

**`:=` para asignar, `==` para comparar.**
En C, `=` y `==` se confunden con facilidad y el compilador no siempre avisa. Usar `:=` hace ese error imposible de escribir por accidente. Es la convención de Pascal y Ada, y en un lenguaje didáctico la claridad pesa más que la brevedad.

**Llaves obligatorias — se elimina el *dangling else*.**
El `overtake` es opcional, lo que reintroduce la ambigüedad clásica del `else` colgante: con dos condicionales anidados y un solo `else`, la gramática no sabría a cuál pertenece. La solución habitual es imponer precedencias en el parser, pero eso **oculta** la ambigüedad sin eliminarla. Al hacer obligatorias las llaves, el cierre `}` marca explícitamente el final de cada bloque y un `overtake` solo puede pertenecer al `race` recién cerrado. La gramática resulta inambigua por construcción:

```bash
$ bison -d -Wall src/f1code.y
(sin advertencias — 0 conflictos shift/reduce)
```

**Palabras reservadas antes que identificadores en el lexer.**
Flex resuelve empates por longitud del match y, a igual longitud, por el orden de las reglas. Si la regla de `IDENT` fuera primero, `garage` haría match como identificador y el lenguaje perdería sus palabras clave.

**Número de línea en cada nodo del AST.**
Cada nodo guarda la línea donde se reconoció. Es lo que permite dar mensajes de error ubicados, y es la base sobre la que se apoyarán las validaciones semánticas.

**Solo dos tipos numéricos.**
`engine` (entero) y `fuel` (real) bastan para ejercitar las 10 capacidades obligatorias y para plantear el problema de conversión entre ellos, sin desviar el proyecto hacia un sistema de tipos que no es el objetivo del curso.

**Variables globales, tabla de símbolos plana.**
F1Code no tiene funciones ni bloques con ámbito propio, así que no se requiere una pila de ámbitos. Si el lenguaje creciera con funciones, esa sería la primera estructura a cambiar.

---

## Roadmap

**Semanas 13–14**
- [ ] Parser y AST más completos, con anotación de tipos en los nodos
- [ ] Manejo de errores con recuperación (regla `error` de Bison) para reportar varios errores en una sola corrida, en lugar de abortar en el primero
- [ ] Validaciones semánticas: variable no declarada, redeclaración, compatibilidad de tipos en operaciones mixtas `engine`/`fuel`
- [ ] Preparar la representación intermedia

**Entrega final**
- [ ] Generación de código C11 a partir del AST
- [ ] Batería de pruebas ampliada

---

## Autores

| Nombre | Carné |
|---|---|
| Juan Andrés Pirir Palma | 2400479 |
| Jonathan Eduardo Jolón García | 2400483 |
| Andrea Alejandra López Jiménez | 2400040 |

**Curso:** Compiladores — Ing. Luis Alberto Pérez Estrada
**Facultad de Ingeniería, Ingeniería en Sistemas y Ciencias de la Computación**
Universidad San Pablo de Guatemala
