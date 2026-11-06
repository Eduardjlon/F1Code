# 1. Ir al proyecto
cd "/c/Users/-INVITADO-/Downloads/F1Code-main/f1code"
ls                                          # debe mostrar: Makefile README.md docs ejemplos include src tests

# 2. Compilar el traductor (la primera vez, o después de cambiar código)
make                                        # si dice "Nothing to be done for 'all'", ya está compilado

# 3. Traducir
./f1code ejemplos/F1Code.f1                 # genera ejemplos/F1Code.and
./f1code ejemplos/F1Code.f1 -o salida.and   # o guarda el resultado con el nombre que quieras

# 4. Ver el resultado
cat ejemplos/F1Code.and                     # muestra el código Andante generado
explorer ejemplos                           # abre la carpeta en Windows

# 5. Para la demo, con AST y tabla de símbolos:
./f1code ejemplos/F1Code.f1 --ast --tabla

# 6. Mostrar la detección de errores
cat tests/invalidos/09_no_declarada.f1      # ver el código con error
./f1code tests/invalidos/09_no_declarada.f1 # ver cómo lo detecta

./f1code tests/invalidos/06_id_inicia_digito.f1   # error léxico
./f1code tests/invalidos/02_falta_punto_coma.f1   # error sintáctico
./f1code tests/invalidos/07_fuel_a_engine.f1      # error semántico

# 7. Para comprobar que todo sigue funcionando:
make test                                   # debe dar: 40 pasaron, 0 fallaron

# 8. Declaración genérica (garage chassis): ver el tipo inferido en el AST y la tabla
./f1code tests/validos/08_chassis_generico.f1 --ast --tabla

# 9. Ciclo para (stint) y retorno (podium)
./f1code tests/validos/09_stint_ciclo_para.f1 --ast -o salida.and && cat salida.and
./f1code tests/validos/10_podium_retorno.f1 --ast -o salida.and && cat salida.and