# Contrato de traducción F1Code → Andante

Grupo 1 · Cadena: Grupo 2 (genera `.f1`) → **Grupo 1 (`.f1` → `.and`)** → Grupo 8 (procesa `.and`)

El lenguaje F1Code no se modifica: es el contrato acordado en la Semana 9. Todo lo que
Andante no tiene se resuelve con reglas semánticas (los "Pendientes" del contrato) y con
reescrituras en el generador.

## 1. Tabla de traducción

| F1Code | Andante |
|---|---|
| `garage engine x := e;` | `numero x = e;` |
| `garage fuel x := e;` | `decimal x = e;` |
| `x := e;` | `avanzar x = e;` |
| `x boost e;` | `avanzar x = x + (e);` |
| `pitstop x;` | `leer x;` |
| `radio e;` | `caminar e;` |
| `race (c) { A }` | `virar (c) inicio A final` |
| `race (c) { A } overtake { B }` | `virar (c) inicio A final sino inicio B final` |
| `lap (c) { A }` | `historia (c) inicio A final` |
| `finish;` | ver sección 3 |
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
7. El menos unario (`-10`, `-x`) se acepta porque el contrato lo usa en el ejemplo válido 6 y en el
   programa demostrativo (`hamilton boost -10;`); el token `-` ya existe.

## 3. Reescrituras del generador

### finish
- **Nivel principal:** se deja de generar; lo que sigue no se ejecutaría.
- **Dentro de `race` o `lap`:** se usa una bandera `numero f1_fin = 0;`.
  - `finish;` → `avanzar f1_fin = 1;`
  - Después de una sentencia que contiene un `finish`, el resto del bloque va dentro de `virar (f1_fin == 0) inicio ... final`.
  - Un `lap` que contiene un `finish` genera `historia (f1_fin == 0 && c)`.
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

## 4. Pendiente de confirmar con el Grupo 8
- Que aceptan `&&` en la condición de `historia` (lo usa la bandera de `finish`).
- Que aceptan literales como `0.0` y paréntesis alrededor del menos unario: `x + (-10)`.
- Si una variable declarada dentro de `inicio ... final` es visible después (el generador ya no depende de esto).
