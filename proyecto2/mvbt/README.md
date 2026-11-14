# MVBT (Becker et al., 1996)

Implementacion en C++17 del Multiversion B-Tree sobre un disco simulado (un bloque = un nodo), mas un B+-tree de una
sola version sobre el mismo disco para comparar.

## Archivos

- `disk.h`: disco simulado y conteo de bloques tocados.
- `mvbt.h`: el MVBT.
- `bptree.h`: B+-tree de una version (insert, erase, find, range) con el mismo conteo de E/S.
- `test.cpp`: pruebas de correctitud y reproduccion de las Figs. 1 a 6 del paper.
- `bench.cpp`, `plots.py`: experimentos y graficos en `results/`.

## Compilar y correr

```
g++ -std=c++17 -O2 -Wall test.cpp -o test && ./test
g++ -std=c++17 -O2 -Wall bench.cpp -o bench && ./bench    # escribe results/*.csv (unos segundos)
python plots.py                                           # escribe results/*.png
```

Parametros por defecto: `b = 25, d = 5, ed = 4` (rango fuerte [9, 21]). Las pruebas pequenas usan `b = 6, d = 2, ed = 1`.
El constructor exige `b >= 2d + 3ed - 1` (3.1) y `ed <= d - 1` (3.2). La condicion `ed > 2` (d > 2/epsilon) solo
hace falta para la cota de espacio y no se exige: se cumple con b = 25, 50, 100 pero no con b = 10 (d = 2, ed = 1).

## Complejidades

| Operacion | Cota | Funcion |
|---|---|---|
| Busqueda en la version v | O(log_d m_v) E/S | `MVBT::find` |
| Rango en la version v | O(log_d m_v + r/d) E/S | `MVBT::range` |
| Insercion / borrado | O(log_d m) E/S, a lo mas 5 ceil(log_d m) | `MVBT::insert`, `MVBT::erase` (con `fix`) |
| Espacio | O(N/b) bloques | `MVBT::fix` (version split, merge, key split) |
| Buscar la raiz de la version v (root*) | O(log p) en memoria | `MVBT::root_at` |

## Decisiones que el paper deja abiertas

- Router como cota inferior del rango del hijo, con `INT_MIN` en el hijo mas a la izquierda: `insercion_secuencial`, `aleatorio_mixto` (`check` valida los rangos).
- Merge con el hermano izquierdo y, si no existe, con el derecho: `merge_hermano`, `fig3`.
- Casos de la raiz (Figs. 4, 5 y 6) y encogimiento repetido (`set_root` baja mientras la raiz sea un directorio con una entrada viva): `raiz_casos`, `fig4`, `fig5`, `fig6`, `borrar_todo`.
- Reestructuraciones encadenadas resueltas subiendo nivel por nivel con `fix`: `aleatorio_mixto`, `borrar_todo`, `fig5`, `fig6`.
- Insertar una clave existente o borrar una inexistente no hace nada y no crea version: `aleatorio_mixto` (el oraculo solo registra operaciones exitosas).
- root* como `vector<pair<int,int>>` con busqueda binaria: `insercion_secuencial` (varias raices) y `raiz_casos`.
- Costo = numero de bloques distintos tocados en la operacion (`Disk::touched`): `costos`.
- Redundancia = entradas almacenadas en todas las hojas (vivas y muertas) / numero de inserts. El paper reporta "registros redundantes por clave" (1.31 a 1.70), que equivale a `redundancia - 1`. Con b = 25 salen 1.41 (mixta) y 1.48 (insercion aleatoria), dentro del rango; la secuencial (2.00) y insertar_y_borrar (2.18) quedan por encima, son cargas que el paper no describe.
- `ed > 2` no se exige (ver arriba).

## Figuras del paper

`fig1` a `fig6` reconstruyen el ejemplo del paper (`b = 6, d = 2, ed = 1`) y comprueban `(clave, in, del)` de las entradas de cada bloque, sin importar el orden ni los ids de hijo. `fig1` a `fig3` parten de la Fig. 1a y aplican las operaciones del paper (`fig3` incluye la busqueda de 25 en la version 5, que toca R y A). `fig4` a `fig6` parten de un estado armado a mano con el contenido de la figura (no se llega a ellos con secuencias cortas) y aplican la operacion que dispara el cambio. Diferencias con el dibujo:

- El router del hijo mas a la izquierda es `INT_MIN` y no la clave minima (10 en las figuras; 5 para A* en la Fig. 2b).
- La version 1 parte con 11 registros ya cargados, asi que la Fig. 1a se arma a mano.
- Fig. 4: G se agrega a R1 antes del version split, asi que R1 conserva la entrada de G con `in = 18`, que nunca esta viva en las versiones 1 a 17.
- Fig. 6: R2 conserva una entrada hacia R5 con `in = del = 32`, que nunca esta viva.

## Comparacion con el B+-tree

`espacio.csv` trae `bloques_bptree`: un B+-tree con los mismos `b` y `d` que recibe solo las operaciones con efecto en el MVBT. `bptree.csv` compara costos y bloques para b = 10, 25, 50 y 100 con cargas aleatoria y mixta. Los bloques vivos del MVBT quedan entre 0.5% y 10% por encima del B+-tree (b = 25, mixta: 852 contra 774) y los costos de busqueda y actualizacion son casi iguales (b = 25, mixta: 4.00 y 3.98 contra 4.00 y 3.88).

## Resultados

`results/costos.csv`, `rango.csv`, `espacio.csv`, `parametros.csv`, `bptree.csv` y un PNG por cada uno. El costo de busqueda
(3 bloques para m = 1000, 4 para m = 10000) no crece con N y ninguna actualizacion supera su cota.
