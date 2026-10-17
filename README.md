# F1Code

Lenguaje de programación didáctico con temática de Fórmula 1, y su traductor construido con **Flex + Bison**.

En la cadena de traducción del curso, el **Grupo 1** recibe programas **F1Code** (`.f1`, generados por el Grupo 2) y los traduce a **Andante** (`.and`, que procesa el Grupo 8).

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

El traductor se construye por etapas.

| Etapa | Estado |
|---|---|
| Análisis léxico (Flex) | Completado |
| Análisis sintáctico (Bison), con recuperación de errores | Completado |
| Árbol de Sintaxis Abstracta (AST) | Completado |
| Tabla de símbolos | Completado |
| Validaciones semánticas | Completado |
| Generación de código Andante (`.and`) | Completado |
| Representación intermedia (IR) | Pendiente |

Hoy el traductor acepta un archivo `.f1`, lo analiza, valida declaraciones y tipos, y genera el `.and` equivalente. Los errores léxicos, sintácticos y semánticos se reportan con número de línea y, si hay alguno, **no se genera** el `.and`.

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
./f1code <archivo.f1> [-o salida.and] [--ast] [--tabla]
```

| Opción | Efecto |
|---|---|
| `-o archivo` | Nombre del `.and` de salida (por defecto, el mismo nombre con extensión `.and`) |
| `--ast` | Imprime el árbol de sintaxis abstracta |
| `--tabla` | Imprime la tabla de símbolos |

Ejemplo:

```bash
./f1code tests/valido.f1 --ast --tabla
```

Salida en consola:

```
--- Arbol de Sintaxis Abstracta (AST) ---
Program
  Decl (engine) hamilton := 300   [linea 3]
  Decl (fuel) leclerc := 50.5   [linea 4]
  Input hamilton   [linea 6]
  If (hamilton > 250)   [linea 8]
    Then:
      Output hamilton   [linea 9]
    Overtake:
      Output leclerc   [linea 11]
  While (hamilton > 200)   [linea 16]
    Boost hamilton += -(10)   [linea 17]
    Output hamilton   [linea 18]
  Finish   [linea 21]

--- Tabla de simbolos ---
Nombre          Tipo       Linea
------          ----       -----
hamilton        engine     3
leclerc         fuel       4

>> Traduccion completada: tests/valido.f1 -> tests/valido.and
```

Archivo generado `tests/valido.and`:

```
// Codigo Andante generado por f1code (Grupo 1) desde valido.f1

numero hamilton = 300;
decimal leclerc = 50.5;
leer hamilton;
virar (hamilton > 250) inicio
    caminar hamilton;
final
sino inicio
    caminar leclerc;
final
historia (hamilton > 200) inicio
    avanzar hamilton = hamilton + (-10);
    caminar hamilton;
final
```

Ante un error, el traductor reporta la línea, lo que encontró y lo que esperaba, y no genera el `.and`:

```
Error sintactico en linea 2: error de sintaxis: se encontro identificador, se esperaba engine o fuel

>> Compilacion ABORTADA: 0 error(es) lexico(s), 1 error(es) sintactico(s). No se genero tests/invalido.and
```

Códigos de salida: `0` traducción correcta · `1` el programa tiene errores · `2` error de uso o de archivo.

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
factor        -> ENTERO | REAL | IDENT | '(' expr ')' | '-' factor
```

> `'-' factor` (menos unario) no aparece en la sección 5 del contrato, pero el contrato lo usa en el ejemplo válido 6 y en el programa demostrativo (`hamilton boost -10;`), y el token `-` ya existe. El parser lo acepta para que esos ejemplos sean válidos.

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

```bash
make test
```

Corre la batería completa (20 pruebas): cada programa de `tests/validos/` debe traducirse y su `.and` debe ser idéntico al de `tests/esperado/`; cada programa de `tests/invalidos/` debe rechazarse sin generar `.and`.

| Válidas | Qué cubren |
|---|---|
| 01_demo_contrato | Programa demostrativo del contrato |
| 02_ejemplos_contrato | Los 6 ejemplos válidos del contrato |
| 03_aritmetica_precedencia | Precedencia, paréntesis, división entera, mezcla de tipos, menos unario |
| 04_comparaciones_ciclo | Las 6 comparaciones, ciclo con contador |
| 05_finish_anidado | `finish` dentro de un `race` dentro de un `lap` |
| 06_bloques_renombrado | Declaración dentro de un ciclo, nombres reservados de Andante, código inalcanzable |

| Inválidas | Error esperado |
|---|---|
| 01–06 | Los 6 ejemplos inválidos del contrato |
| 07, 08 | `fuel` asignado a `engine` (declaración y `boost`) |
| 09, 10 | Variable no declarada / redeclarada |
| 11 | División entre cero |
| 12 | Comentario de bloque sin cerrar |
| 13 | Comparaciones encadenadas |
| 14 | `=` en lugar de `:=` |

`tests/valido.f1` y `tests/invalido.f1` son los programas de la Entrega 2.

---

## Estructura del repositorio

```
f1code/
├── Makefile           # Build de un solo comando
├── README.md
├── include/           # Headers (ast.h, symtab.h, semantico.h, codegen_andante.h)
├── src/
│   ├── f1code.l           # Especificación Flex (analizador léxico)
│   ├── f1code.y           # Gramática Bison (analizador sintáctico)
│   ├── ast.c              # Construcción e impresión del AST
│   ├── symtab.c           # Tabla de símbolos
│   ├── semantico.c        # Validaciones: declaración, tipos, división entre cero, finish
│   ├── codegen_andante.c  # Generador .and (único módulo que conoce Andante)
│   └── main.c             # Punto de entrada
├── tests/
│   ├── validos/           # Deben traducirse
│   ├── esperado/          # .and esperado de cada válido
│   ├── invalidos/         # Deben rechazarse
│   ├── run_tests.sh
│   ├── valido.f1
│   └── invalido.f1
├── docs/
│   └── traduccion_andante.md  # Contrato de traducción F1Code -> Andante
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
┌───────────────────┐
│   Validaciones    │  semantico.c: declaración, tipos, finish
└───────┬───────────┘
        ▼
┌───────────────────┐
│ Generador Andante │  codegen_andante.c: AST → .and
└───────┬───────────┘
        ▼
  Programa Andante (.and) → Grupo 8
```

Cada etapa tiene una responsabilidad única y se prueba por separado. El generador es el único módulo que conoce Andante: si cambia el lenguaje destino, solo se reemplaza `codegen_andante.c`.

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

**Traducción a Andante sin modificar F1Code.**
Lo que Andante no tiene se resuelve con reglas semánticas y en el generador: `boost` se reescribe como `avanzar x = x + (e)`, un `finish` dentro de un bloque usa una variable bandera, las declaraciones dentro de bloques se suben al inicio y los identificadores que chocan con palabras de Andante se renombran. Ver [`f1code/docs/traduccion_andante.md`](f1code/docs/traduccion_andante.md).

**Variables globales, tabla de símbolos plana.**
F1Code no tiene funciones ni bloques con ámbito propio, así que no se requiere una pila de ámbitos. Si el lenguaje creciera con funciones, esa sería la primera estructura a cambiar.

---

## Roadmap

**Semanas 13–14**
- [x] Parser y AST más completos, con anotación de tipos en los nodos
- [x] Manejo de errores con recuperación (regla `error` de Bison) para reportar varios errores en una sola corrida
- [x] Validaciones semánticas: variable no declarada, redeclaración, compatibilidad de tipos en operaciones mixtas `engine`/`fuel`
- [ ] Representación intermedia

**Entrega final**
- [x] Generación de código Andante (`.and`) a partir del AST
- [x] Batería de pruebas ampliada
- [ ] Confirmar con el Grupo 8: `&&` en la condición de `historia`, literales `0.0` y `x + (-10)`

---

## Integrantes del Grupo 1

| Nombre | Carné |
|---|---|
| Juan Andrés Pirir Palma | 2400479 |
| Jonathan Eduardo Jolón García | 2400483 |
| Andrea Alejandra López Jiménez | 2400040 |

**Curso:** Compiladores — Ing. Luis Alberto Pérez Estrada
**Facultad de Ingeniería, Ingeniería en Sistemas y Ciencias de la Computación**
Universidad San Pablo de Guatemala
