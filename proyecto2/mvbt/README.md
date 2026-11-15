# MVBT (Becker et al., 1996)

Implementacion en C++17 del Multiversion B-Tree sobre un disco simulado (un bloque = un nodo), mas un B+-tree de una
sola version sobre el mismo disco para comparar.

## Archivos

- `disk.h`: disco simulado y conteo de bloques tocados.
- `mvbt.h`: el MVBT.
- `bptree.h`: B+-tree de una version (insert, erase, find, range) con hojas enlazadas y el mismo conteo de E/S.
- `test.cpp`: pruebas de correctitud y reproduccion de las Figs. 1 a 6 del paper.
- `bench.cpp`, `plots.py`: experimentos y graficos en `results/`.

## Compilar y correr

```
make check      # compila y corre las pruebas
make results    # corre el benchmark (results/*.csv, unos segundos) y genera results/*.png
```

Sin make: `g++ -std=c++17 -O2 -Wall test.cpp -o test && ./test`, lo mismo con `bench.cpp`, y `python3 plots.py`.
Cada experimento fija su propia semilla, asi que los CSV son reproducibles.

Parametros por defecto: `b = 25, d = 5, ed = 4` (rango fuerte [9, 21]). Las pruebas pequenas usan `b = 6, d = 2, ed = 1`.
El constructor exige `d >= 2`, `ed >= 1`, `b >= 2d + 3ed - 1` (3.1) y `ed <= d - 1` (3.2). La condicion `ed > 2` (d > 2/epsilon) solo
hace falta para la cota de espacio y no se exige: se cumple con b = 25, 50, 100 pero no con b = 10 (d = 2, ed = 1).

## Complejidades

| Operacion | Cota | Funcion |
|---|---|---|
| Busqueda en la version v | O(log_d m_v) E/S | `MVBT::find` |
| Rango en la version v | O(log_d m_v + r/d) E/S | `MVBT::range` |
| Insercion / borrado | O(log_d m) E/S, a lo mas 5 ceil(log_d m) | `MVBT::insert`, `MVBT::erase` (con `fix`) |
| Espacio | O(N/b) bloques | `MVBT::fix` (version split, merge, key split) |
| Buscar la raiz de la version v (root*) | O(log p) en memoria | `MVBT::root_at` |
| B+-tree: busqueda, insercion, borrado | O(log_d n) E/S | `BPT::find`, `BPT::insert`, `BPT::erase` |
| B+-tree: rango | O(log_d n + r/d) E/S | `BPT::range` (punteros entre hojas) |

## Decisiones que el paper deja abiertas

- Router como cota inferior del rango del hijo, con `INT_MIN` en el hijo mas a la izquierda: `insercion_secuencial`, `aleatorio_mixto` (`check` valida los rangos).
- Merge con el hermano izquierdo y, si no existe, con el derecho: `merge_hermano`, `fig3`.
- Casos de la raiz (Figs. 4, 5 y 6) y encogimiento repetido (`set_root` baja mientras la raiz sea un directorio con una entrada viva): `raiz_casos`, `fig4`, `fig5`, `fig6`, `borrar_todo`.
- Reestructuraciones encadenadas resueltas subiendo nivel por nivel con `fix`: `aleatorio_mixto`, `borrar_todo`, `fig5`, `fig6`.
- Insertar una clave existente o borrar una inexistente no hace nada y no crea version: `aleatorio_mixto` (el oraculo solo registra operaciones exitosas).
- root* como `vector<pair<int,int>>` con busqueda binaria: `insercion_secuencial` (varias raices) y `raiz_casos`.
- Costo = numero de bloques distintos tocados en la operacion (`Disk::touched`), sin distinguir lecturas de escrituras: `costos` comprueba las cotas exactas del paper, busqueda <= ceil(log_d m_v) y actualizacion <= 5 ceil(log_d m), sin holgura.
- Condicion fuerte: todo bloque nuevo que no es raiz nace con entre `d + ed` y `b - ed` entradas vivas: `condicion_fuerte`.
- El rango del MVBT recorre el subarbol de la version (el MVBT no tiene punteros entre hojas, porque una hoja pertenece a varias versiones); el del B+-tree avanza por `next`.
- Redundancia = entradas almacenadas en todas las hojas (vivas y muertas) / numero de inserts. El paper reporta "registros redundantes por clave" (1.31 a 1.70), que equivale a `redundancia - 1`. Con b = 25 salen 1.40 (mixta) y 1.48 (insercion aleatoria), dentro del rango; la secuencial (2.00) y insertar_y_borrar (2.18) quedan por encima, son cargas que el paper no describe.
- `ed > 2` no se exige (ver arriba).

## Figuras del paper

`fig1` a `fig6` reconstruyen el ejemplo del paper (`b = 6, d = 2, ed = 1`) y comprueban `(clave, in, del)` de las entradas de cada bloque, sin importar el orden ni los ids de hijo. `fig1` a `fig3` parten de la Fig. 1a y aplican las operaciones del paper (`fig3` incluye la busqueda de 25 en la version 5, que toca R y A). `fig4` a `fig6` parten de un estado armado a mano con el contenido de la figura (no se llega a ellos con secuencias cortas) y aplican la operacion que dispara el cambio. Diferencias con el dibujo:

- El router del hijo mas a la izquierda es `INT_MIN` y no la clave minima (10 en las figuras; 5 para A* en la Fig. 2b).
- La version 1 parte con 11 registros ya cargados, asi que la Fig. 1a se arma a mano.
- Fig. 4: G se agrega a R1 antes del version split, asi que R1 conserva la entrada de G con `in = 18`, que nunca esta viva en las versiones 1 a 17.
- Fig. 6: R2 conserva una entrada hacia R5 con `in = del = 32`, que nunca esta viva.

## Comparacion con el B+-tree

`espacio.csv` trae `bloques_bptree`: un B+-tree con los mismos `b` y `d` que recibe solo las operaciones con efecto en el MVBT. `bptree.csv` compara costos y bloques para b = 10, 25, 50 y 100 con cargas aleatoria y mixta.

- Bloques vivos del MVBT contra bloques del B+-tree: desde 1% por debajo (b = 10, mixta: 2336 contra 2361) hasta 20% por encima (b = 100, mixta: 200 contra 167); con b = 25 y carga mixta, 873 contra 792 (10%).
- Busqueda en la version actual: el mismo numero de bloques en 7 de las 8 configuraciones. Con b = 25 e insercion aleatoria el MVBT tiene un nivel mas (5.00 contra 4.00), porque sus bloques nacen con a lo mas `b - ed = 21` entradas vivas.
- Actualizacion: el MVBT toca a lo mas 6% mas bloques en promedio (b = 10, aleatoria: 6.12 contra 5.78; b = 25, mixta: 3.98 contra 3.88).

## Resultados

Un CSV y un PNG por experimento en `results/`:

- `costos`: con m fijo, el costo de busqueda (3 bloques para m = 1000, 4 para m = 10000) no crece con N, ni en la version actual ni en versiones pasadas, y ninguna actualizacion supera 5 ceil(log_d m) (maximo observado: 10 contra una cota de 30).
- `rango`: E/S contra el tamano del resultado para b = 25 y b = 100, en la version actual, en versiones pasadas al azar y en el B+-tree, sobre un arbol de 100000 claves con 100000 actualizaciones posteriores. Con b = 25 y r = 1000: 82.5 (actual), 76.6 (pasadas), 69.1 (B+-tree), contra log_d m + r/d = 208.
- `espacio`: bloques totales del MVBT (nunca se liberan, asi que el total es tambien el pico), bloques vivos y bloques del B+-tree. `referencia_N_b` es N/b, una referencia y no una cota: el espacio es O(N/b) con una constante mayor que 1. `bloques_por_N_b` baja de 2.70 (N = 5000) a 1.50 (N = 100000).
- `parametros`: redundancia, espacio y costos para b = 10, 25, 50, 100 (d = b/5, ed = d - 1) y cuatro cargas.
- `epsilon`: con b = 25 fijo, todas las combinaciones validas de d en {3, 4, 5, 6, 8} y ed, con cargas aleatoria y mixta. Con solo inserciones epsilon no influye (redundancia 2.47 a 2.49). Con carga mixta la redundancia baja al crecer epsilon (d = 5: de 2.95 con epsilon = 0.2 a 2.41 con epsilon = 0.8), como predice el paper al recomendar epsilon maximo. `cota_redundancia` es la cota de peor caso 2k/epsilon (12.5 a 50), muy por encima de lo medido.
- `bptree`: la comparacion de la seccion anterior.
