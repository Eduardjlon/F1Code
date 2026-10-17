# F1Code — Traductor F1Code → Andante

Traductor del lenguaje **F1Code** (`.f1`) a **Andante** (`.and`), construido con Flex y Bison.
Grupo 1 · Cadena: Grupo 2 genera el `.f1` → **Grupo 1 lo traduce** → Grupo 8 procesa el `.and`.

```
.f1 → Flex (tokens) → Bison (AST) → semantico.c (validaciones) → codegen_andante.c → .and
```

## Requisitos

`gcc`, `make`, `flex` (>= 2.6) y `bison` (>= 3.0).

```bash
sudo apt-get install -y flex bison gcc make                    # Ubuntu / WSL
pacman -S mingw-w64-ucrt-x86_64-gcc flex bison make            # Windows (MSYS2 UCRT64)
```

## Compilar, usar y probar

```bash
make                                   # genera ./f1code
./f1code tests/valido.f1               # genera tests/valido.and
./f1code programa.f1 -o salida.and     # nombre de salida propio
./f1code programa.f1 --ast --tabla     # además imprime el AST y la tabla de símbolos
make test                              # 20 pruebas: válidas, inválidas y .and esperado
make clean
```

Si hay errores léxicos, sintácticos o semánticos, se reportan con número de línea y
**no se genera** el `.and`. Códigos de salida: `0` correcto · `1` el programa tiene errores ·
`2` error de uso o de archivo.

## Estructura

```
include/            ast.h, symtab.h, semantico.h, codegen_andante.h
src/
  f1code.l          analizador léxico (Flex)
  f1code.y          gramática del contrato + AST, con recuperación de errores (Bison)
  ast.c             constructores e impresión del AST
  symtab.c          tabla de símbolos global
  semantico.c       validaciones: declaración, tipos, división entre cero, finish
  codegen_andante.c generador .and (único módulo que conoce Andante)
  main.c            línea de comandos
tests/
  validos/ esperado/ invalidos/ run_tests.sh
  valido.f1 invalido.f1    (programas de la Entrega 2)
docs/
  traduccion_andante.md    tabla de traducción y decisiones de diseño
```

## Reglas semánticas

1. Toda variable se declara antes de usarse y una sola vez.
2. `engine` op `engine` → `engine` (la división es entera); si interviene un `fuel`, el resultado es `fuel`.
3. `engine` → `fuel` se permite; **`fuel` → `engine` es error**, porque Andante no tiene esa conversión.
4. División entre la constante `0` es error.
5. `finish` es válido en cualquier parte; lo que sigue en el mismo bloque es inalcanzable (advertencia).

La tabla de traducción completa, cómo se traduce `finish` dentro de bloques, las declaraciones
dentro de bloques y el renombrado de identificadores están en
[`docs/traduccion_andante.md`](docs/traduccion_andante.md).
