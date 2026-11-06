# Contrato de traducción F1Code → Andante

Grupo 1 · Cadena: Grupo 2 (genera `.f1`) → **Grupo 1 (`.f1` → `.and`)** → Grupo 8 (procesa `.and`)

El contrato es el acordado en la Semana 9, más tres categorías agregadas el 2026-11-06 para que
todos los lenguajes de la cadena tengan las mismas categorías (tabla comparativa de lenguajes):
declaración genérica (`garage chassis`), ciclo para (`stint`) y retorno (`podium`). Todo lo que
Andante no tiene se resuelve con reglas semánticas (los "Pendientes" del contrato) y con
reescrituras en el generador, así que Andante recibe solo instrucciones que ya existen.

## 1. Tabla de traducción

| F1Code | Andante |
|---|---|
| `garage engine x := e;` | `numero x = e;` |
| `garage fuel x := e;` | `decimal x = e;` |
| `garage chassis x := e;` | `numero x = e;` si `e` es entera · `decimal x = e;` si `e` es real |
| `x := e;` | `avanzar x = e;` |
| `x boost e;` | `avanzar x = x + (e);` |
| `pitstop x;` | `leer x;` |
| `radio e;` | `caminar e;` |
| `race (c) { A }` | `virar (c) inicio A final` |
| `race (c) { A } overtake { B }` | `virar (c) inicio A final sino inicio B final` |
| `lap (c) { A }` | `historia (c) inicio A final` |
| `stint (I; c; P) { A }` | `I` traducido + `historia (c) inicio A P final` (ver sección 3) |
| `finish;` | ver sección 3 |
| `podium e;` | `avanzar f1_podio = e;` y luego igual que `finish` (ver sección 3) |
| `-e` | `-e` (entre paréntesis si es parte de otra expresión) |
| `+ - * /`, `< > <= >= == !=`, `( )` | iguales |
| `// ...`, `/* ... */` | se descartan |

## 2. Reglas semánticas de F1Code (resuelven los pendientes del contrato)

1. Toda variable se declara antes de usarse y una sola vez (tabla de símbolos global).
2. `engine` op `engine` → `engine`, y la división es entera. Si interviene un `fuel`, el resultado es `fuel`.
3. `engine` → `fuel` se permite. **`fuel` → `engine` es error**, porque Andante no permite asignar decimal a número.
   Aplica a declaración, `:=` y `boost`.
4. División entre la constante `0` es error.
5. `radio` imprime con salto de línea; un `fuel` se imprime con 2 decimales (igual que `caminar`).
6. `finish` es opcional y válido en cualquier parte. Lo que sigue en el mismo bloque es inalcanzable (advertencia).
7. **Declaración genérica `garage chassis`:** el chasis no trae motor ni combustible fijo; toma el
   tipo de su valor inicial (`engine` si la expresión es entera, `fuel` si es real). Desde ahí el
   tipo queda fijo y aplican las reglas 2 y 3. El valor inicial es obligatorio, porque sin él no
   hay de dónde inferir el tipo. Andante no tiene tipo genérico, así que el generador emite el tipo
   ya inferido (`numero` o `decimal`): **el Grupo 8 no tiene que cambiar nada**.
8. **Ciclo para `stint (I; c; P) { A }`:** `I` es una declaración o asignación (`garage engine|fuel|chassis i := e`
   o `i := e`) y `P` es un `boost` o una asignación (`i boost e` o `i := e`), ambos sin `;`. Se validan como
   sentencias normales. La variable de control va a la tabla global: sigue visible después del ciclo y no se
   puede volver a declarar.
9. **Retorno `podium e;`:** termina el programa entregando el valor de `e`, como `return e;` en el `main` de C
   (F1Code no tiene funciones: el programa completo es la carrera y `podium` entrega el resultado). El valor es
   obligatorio; para terminar sin valor está `finish`. En el control de flujo se comporta igual que `finish`
   (regla 6), incluida la advertencia de código inalcanzable.
10. El menos unario (`-10`, `-x`) se acepta porque el contrato lo usa en el ejemplo válido 6 y en el
   programa demostrativo (`hamilton boost -10;`); el token `-` ya existe.

## 3. Reescrituras del generador

### finish
- **Nivel principal:** se deja de generar; lo que sigue no se ejecutaría.
- **Dentro de `race`, `lap` o `stint`:** se usa una bandera `numero f1_fin = 0;`.
  - `finish;` → `avanzar f1_fin = 1;`
  - Después de una sentencia que contiene un `finish`, el resto del bloque va dentro de `virar (f1_fin == 0) inicio ... final`.
  - Un `lap` o `stint` que contiene un `finish` genera `historia (f1_fin == 0 && c)`.
  - La bandera solo se genera si hay un `finish` anidado.

```
lap (v < 10) {                    historia (f1_fin == 0 && v < 10) inicio
    race (v == 7) { finish; }         virar (v == 7) inicio
    v boost 1;              →              avanzar f1_fin = 1;
}                                     final
radio v;                              virar (f1_fin == 0) inicio
                                          avanzar v = v + 1;
                                      final
                                  final
                                  virar (f1_fin == 0) inicio
                                      caminar v;
                                  final
```

### stint (ciclo para)
Andante no tiene ciclo para, así que se reescribe como un `lap`:
`stint (I; c; P) { A }` ≡ `I; lap (c) { A P; }`.

```
stint (garage engine v := 1; v <= 5; v boost 1) {      numero v = 1;
    radio v;                                     →     historia (v <= 5) inicio
}                                                          caminar v;
                                                           avanzar v = v + 1;
                                                       final
```
- El paso `P` queda al final del cuerpo, así que si hay un `finish` antes, también queda dentro del
  `virar (f1_fin == 0)`.
- Si el `stint` está dentro de un bloque, su declaración se sube al inicio como cualquier otra.

### podium (retorno)
Andante no tiene retorno. El valor se guarda en una variable reservada `f1_podio` (declarada al
inicio: `numero`, o `decimal` si algún `podium` devuelve un `fuel`) y la ejecución se detiene con
las mismas reglas de `finish`:

```
race (v == 7) {                   virar (v == 7) inicio
    podium t * 2;         →           avanzar f1_podio = t * 2;
}                                     avanzar f1_fin = 1;
                                  final
```
- En el nivel principal, `podium e;` → `avanzar f1_podio = e;` y se deja de generar.
- `f1_podio` solo se genera si el programa tiene algún `podium`, y se renombra si choca con una variable.
- Es el único punto que depende de que Andante no tenga retorno: cuando el Grupo 8 publique su
  palabra de retorno, solo cambia `emitStop()` en `codegen_andante.c`.

### Declaraciones dentro de bloques
F1Code tiene tabla global, pero no sabemos si Andante tiene alcance por bloque. Toda
declaración que quedaría dentro de un `inicio ... final` se sube al inicio del programa con
valor neutro (`0` o `0.0`) y en su lugar queda un `avanzar`.

### Nombres reservados de Andante
Si un identificador de F1Code es una de las 14 palabras reservadas de Andante (`caminar`, `leer`,
`avanzar`, `virar`, `sino`, `historia`, `inicio`, `final`, `detener`, `numero`, `decimal`, `texto`,
`verdadero`, `falso`), se renombra agregando `_f1` hasta que el nombre esté libre
(`final` → `final_f1`, o `final_f1_f1` si `final_f1` ya existe). Lo mismo para `f1_fin`.

### Paréntesis
Toda subexpresión compuesta se genera entre paréntesis (`a + b * c` → `a + (b * c)`), para no
depender de la precedencia de Andante ni de los siguientes eslabones de la cadena.

## 4. Pendiente de confirmar

**Con el Grupo 2:** `chassis`, `stint` y `podium` ya son palabras reservadas de F1Code (no se pueden usar
como identificadores). Sintaxis: `garage chassis x := valor;`, `stint (init; cond; paso) { ... }` y
`podium valor;`.

**Con el Grupo 8:** cuál es su palabra para el ciclo para y el retorno, si los agregan. Mientras tanto,
el generador usa solo `historia`, `avanzar` y `virar`, que ya existen.

- Que aceptan `&&` en la condición de `historia` (lo usa la bandera de `finish`).
- Que aceptan literales como `0.0` y paréntesis alrededor del menos unario: `x + (-10)`.
- Si una variable declarada dentro de `inicio ... final` es visible después (el generador ya no depende de esto).