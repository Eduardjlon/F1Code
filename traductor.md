# 1. Ir al proyecto
cd /c/Users/

# 2. Compilar el traductor (la primera vez, o después de cambiar código)
make

# 3. Traducir
./f1code ejemplos/F1Code.f1                 # genera ejemplos/F1Code.and
./f1code ejemplos/F1Code.f1 -o salida.and   # o con el nombre que quieras

# 4. Ver el resultado
cat ejemplos/F1Code.and

# 5. Para la demo, con AST y tabla de símbolos:
./f1code ejemplos/F1Code.f1 --ast --tabla

# 6. Para comprobar que todo sigue funcionando:
make test        # debe dar: 21 pasaron, 0 fallaron