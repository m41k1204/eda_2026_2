import csv
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt


def leer(nombre):
    with open("results/" + nombre + ".csv") as f:
        return list(csv.DictReader(f))


def col(filas, c):
    return [float(x[c]) for x in filas]


# costos.csv: costo vs N para cada m, con la cota punteada
filas = leer("costos")
fig, ax = plt.subplots(1, 2, figsize=(10, 4))
for m, color in (("1000", "C0"), ("10000", "C1")):
    f = [x for x in filas if x["m"] == m]
    n = col(f, "N")
    ax[0].plot(n, col(f, "busq_prom"), color=color, label="actual m=" + m)
    ax[0].plot(n, col(f, "busq_vieja_prom"), color=color, linestyle="--", marker="o", markersize=3, label="pasadas m=" + m)
    ax[0].plot(n, col(f, "cota_busq"), color=color, linestyle=":", label="cota m=" + m)
    ax[1].plot(n, col(f, "act_prom"), color=color, label="promedio m=" + m)
    ax[1].plot(n, col(f, "act_max"), color=color, linestyle="--", label="maximo m=" + m)
    ax[1].plot(n, col(f, "cota_act"), color=color, linestyle=":", label="cota m=" + m)
ax[0].set_xlabel("N")
ax[0].set_ylabel("bloques por busqueda")
ax[1].set_xlabel("N")
ax[1].set_ylabel("bloques por actualizacion")
for a in ax:
    a.set_ylim(bottom=0)
    a.legend(fontsize=7)
fig.tight_layout()
fig.savefig("results/costos.png", dpi=120)
plt.close(fig)

# rango.csv: E/S vs r para cada b, en la version actual, en versiones pasadas y en el B+-tree
filas = leer("rango")
valores_b = sorted({x["b"] for x in filas}, key=int)
fig, ax = plt.subplots(1, len(valores_b), figsize=(5 * len(valores_b), 4))
for eje, valor_b in zip(ax, valores_b):
    f = [x for x in filas if x["b"] == valor_b]
    r = col(f, "r_prom")
    eje.plot(r, col(f, "mvbt_actual"), marker="o", label="MVBT version actual")
    eje.plot(r, col(f, "mvbt_pasada"), marker="s", linestyle="--", label="MVBT versiones pasadas")
    eje.plot(r, col(f, "bptree"), marker="^", label="B+-tree")
    eje.plot(r, col(f, "cota"), linestyle=":", color="gray", label="log_d m + r/d")
    eje.set_xscale("log")
    eje.set_yscale("log")
    eje.set_xlabel("r (resultados)")
    eje.set_ylabel("bloques por consulta")
    eje.set_title("b = " + valor_b + ", d = " + f[0]["d"])
    eje.legend(fontsize=8)
fig.tight_layout()
fig.savefig("results/rango.png", dpi=120)
plt.close(fig)

# espacio.csv: bloques vs N
filas = leer("espacio")
n = col(filas, "N")
fig, ax = plt.subplots(figsize=(5, 4))
ax.plot(n, col(filas, "bloques"), label="bloques MVBT (total = pico)")
ax.plot(n, col(filas, "bloques_vivos"), label="bloques vivos MVBT")
ax.plot(n, col(filas, "bloques_bptree"), label="B+-tree")
ax.plot(n, col(filas, "referencia_N_b"), linestyle=":", label="N/b (referencia, no es cota)")
ax.set_xlabel("N")
ax.set_ylabel("bloques")
ax.legend()
fig.tight_layout()
fig.savefig("results/espacio.png", dpi=120)
plt.close(fig)

# parametros.csv: redundancia por b y carga
filas = leer("parametros")
bs = sorted({int(x["b"]) for x in filas})
cargas = []
for x in filas:
    if x["carga"] not in cargas:
        cargas.append(x["carga"])
fig, ax = plt.subplots(figsize=(7, 4))
ancho = 0.2
for j, carga in enumerate(cargas):
    vals = [float(x["redundancia"]) for b in bs for x in filas if int(x["b"]) == b and x["carga"] == carga]
    ax.bar([i + j * ancho for i in range(len(bs))], vals, ancho, label=carga)
ax.set_xticks([i + 1.5 * ancho for i in range(len(bs))])
ax.set_xticklabels([str(b) for b in bs])
ax.set_xlabel("b")
ax.set_ylabel("redundancia")
ax.legend(fontsize=8)
fig.tight_layout()
fig.savefig("results/parametros.png", dpi=120)
plt.close(fig)

# bptree.csv: MVBT vs B+-tree por b (continuo = aleatoria, punteado = mixta)
filas = leer("bptree")
fig, ax = plt.subplots(1, 3, figsize=(12, 4))
for k, (m, bp, ylabel) in enumerate((("mvbt_busq", "bptree_busq", "bloques por busqueda"),
                                     ("mvbt_act", "bptree_act", "bloques por actualizacion"),
                                     ("mvbt_bloques_vivos", "bptree_bloques", "bloques"))):
    for carga, estilo in (("insercion_aleatoria", "-"), ("mixta", "--")):
        f = [x for x in filas if x["carga"] == carga]
        ax[k].plot(col(f, "b"), col(f, m), color="C0", linestyle=estilo, marker="o", label="MVBT " + carga)
        ax[k].plot(col(f, "b"), col(f, bp), color="C1", linestyle=estilo, marker="o", label="B+-tree " + carga)
    ax[k].set_xscale("log")
    ax[k].set_xlabel("b")
    ax[k].set_ylabel(ylabel)
ax[2].set_yscale("log")
ax[0].legend(fontsize=7)
fig.tight_layout()
fig.savefig("results/bptree.png", dpi=120)
plt.close(fig)

# epsilon.csv: redundancia y bloques por N/b contra epsilon, una linea por d (b = 25)
filas = leer("epsilon")
valores_d = sorted({int(x["d"]) for x in filas})
fig, ax = plt.subplots(1, 2, figsize=(10, 4))
for eje, carga in zip(ax, ("insercion_aleatoria", "mixta")):
    for d in valores_d:
        f = [x for x in filas if int(x["d"]) == d and x["carga"] == carga]
        eje.plot(col(f, "epsilon"), col(f, "redundancia"), marker="o", label="d = " + str(d))
    eje.set_xlabel("epsilon")
    eje.set_ylabel("redundancia")
    eje.set_title(carga + " (b = 25)")
    eje.set_ylim(bottom=0)
    eje.legend(fontsize=8, loc="lower right")
fig.tight_layout()
fig.savefig("results/epsilon.png", dpi=120)
plt.close(fig)
