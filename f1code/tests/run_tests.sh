#!/bin/sh
# Bateria de pruebas del traductor F1Code -> Andante
#   validos:   deben traducirse y el .and debe ser identico a tests/esperado/
#   invalidos: deben rechazarse (codigo de salida != 0) y NO generar .and

cd "$(dirname "$0")/.." || exit 2
BIN=./f1code
OUT=tests/salida
mkdir -p "$OUT"
pasaron=0
fallaron=0

# Compara el .and esperado con el generado, ignorando CRLF/LF.
# Usa diff si esta instalado (muestra las diferencias); si no, compara el texto.
iguales() {
    if command -v diff >/dev/null 2>&1; then
        diff -u --strip-trailing-cr "$1" "$2" >"$3"
    else
        [ "$(tr -d '\r' <"$1")" = "$(tr -d '\r' <"$2")" ] || {
            echo "el .and generado no coincide con $1 (instala diffutils para ver las diferencias)" >"$3"
            return 1
        }
    fi
}

for f in tests/validos/*.f1; do
    n=$(basename "$f" .f1)
    rm -f "$OUT/$n.and"
    if $BIN "$f" -o "$OUT/$n.and" >/dev/null 2>"$OUT/$n.err" && iguales "tests/esperado/$n.and" "$OUT/$n.and" "$OUT/$n.diff"; then
        echo "  OK     valido    $n"
        pasaron=$((pasaron + 1))
    else
        echo "  FALLA  valido    $n"
        cat "$OUT/$n.err" "$OUT/$n.diff" 2>/dev/null | sed 's/^/         /'
        fallaron=$((fallaron + 1))
    fi
done

for f in tests/invalidos/*.f1; do
    n=$(basename "$f" .f1)
    rm -f "$OUT/$n.and"
    if $BIN "$f" -o "$OUT/$n.and" >/dev/null 2>"$OUT/$n.err"; then
        echo "  FALLA  invalido  $n (se acepto un programa invalido)"
        fallaron=$((fallaron + 1))
    elif [ -f "$OUT/$n.and" ]; then
        echo "  FALLA  invalido  $n (genero .and a pesar del error)"
        fallaron=$((fallaron + 1))
    else
        echo "  OK     invalido  $n -> $(head -n 1 "$OUT/$n.err")"
        pasaron=$((pasaron + 1))
    fi
done

echo
echo "Resultado: $pasaron pasaron, $fallaron fallaron"
[ "$fallaron" -eq 0 ]
