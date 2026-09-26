# F1Code — Front-end (Entrega 2, Semana 11)

Front-end funcional del lenguaje **F1Code** (`.f1`): analizador léxico (Flex),
analizador sintáctico (Bison), construcción de AST y tabla de símbolos básica.

## Estructura del proyecto

```
f1code/
├── include/
│   ├── ast.h          # Definición de nodos del AST
│   └── symtab.h        # Definición de la tabla de símbolos
├── src/
│   ├── f1code.l         # Analizador léxico (Flex)
│   ├── f1code.y         # Analizador sintáctico + construcción de AST (Bison)
│   ├── ast.c            # Constructores e impresión del AST
│   ├── symtab.c         # Implementación de la tabla de símbolos
│   └── main.c           # Programa principal (driver)
├── tests/
│   ├── valido.f1        # Programa de ejemplo válido
│   └── invalido.f1      # Programa de ejemplo con error sintáctico
├── Makefile
└── README.md
```

## Requisitos

- `gcc`
- `flex` (>= 2.6)
- `bison` (>= 3.0)
- `make`

En Ubuntu/Debian, si no los tienes instalados:

```bash
sudo apt-get update
sudo apt-get install -y flex bison gcc make
```

## Compilación

Desde la carpeta `f1code/`:

```bash
make
```

Esto genera el ejecutable `f1code` en la raíz del proyecto. Internamente el
Makefile ejecuta, en orden:

1. `bison -d src/f1code.y` → genera `f1code.tab.c` / `f1code.tab.h`
2. `flex src/f1code.l` → genera `lex.yy.c`
3. Compila y enlaza todo junto con `ast.c`, `symtab.c` y `main.c`

Para limpiar los archivos generados:

```bash
make clean
```

## Ejecución

```bash
./f1code tests/valido.f1
./f1code tests/invalido.f1
```

O bien, para correr ambas pruebas de una vez:

```bash
make test
```

### Salida esperada

- **Programa válido**: imprime el AST completo (con número de línea de cada
  sentencia) y la tabla de símbolos, y termina con código de salida `0`.
- **Programa inválido**: imprime el/los error(es) léxico(s) o sintáctico(s)
  con su línea exacta, aborta la compilación y termina con código de
  salida `1`.

## Cobertura de esta entrega

- [x] Archivo Flex (`.l`) funcional — reconoce las 10 palabras reservadas,
      identificadores, enteros, reales, operadores, delimitadores y
      comentarios de línea y de bloque.
- [x] Archivo Bison (`.y`) funcional — implementa la gramática completa del
      contrato del lenguaje (Entrega 1), incluyendo precedencia de
      operadores mediante la jerarquía `expr` → `term` → `factor`.
- [x] Integración Flex + Bison (vía `f1code.tab.h` generado por Bison).
- [x] AST inicial para las 10 capacidades obligatorias.
- [x] Tabla de símbolos básica (nombre, tipo, línea de declaración),
      con detección de redeclaración y de uso de variables no declaradas.
- [x] Prueba de un programa válido (`tests/valido.f1`) y uno inválido
      (`tests/invalido.f1`).

## Notas de diseño

- La gramática resuelve la precedencia de operadores **estructuralmente**
  (sin necesidad de directivas `%left`/`%right`), por lo que compila sin
  ningún conflicto shift/reduce ni reduce/reduce.
- Los mensajes de error (léxicos, sintácticos y semánticos básicos) incluyen
  siempre el número de línea, usando `%option yylineno` de Flex.
- La validación semántica completa (verificación de tipos, ámbitos, etc.)
  se profundizará en la etapa de "Validaciones" del pipeline (semanas 13–14),
  conforme al cronograma de la guía del proyecto.
