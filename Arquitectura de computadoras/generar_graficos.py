"""Gráficas opcionales para ilustrar las mediciones del laboratorio."""
from pathlib import Path
import math
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from qiskit import QuantumCircuit
from qiskit_aer import AerSimulator

out = Path(__file__).resolve().parent / "resultados"
out.mkdir(exist_ok=True)
backend = AerSimulator()

qc = QuantumCircuit(2, 2)
qc.h(0)
qc.cx(0, 1)
qc.measure([0, 1], [0, 1])
counts = backend.run(qc, shots=10_000).result().get_counts()
fig, ax = plt.subplots(figsize=(7, 4))
keys = ["00", "01", "10", "11"]
ax.bar(keys, [counts.get(key, 0) / 100 for key in keys])
ax.set(title="Estado de Bell: resultados de 10 000 mediciones", xlabel="Estado medido", ylabel="Probabilidad (%)", ylim=(0, 100))
fig.tight_layout()
fig.savefig(out / "grafica_bell.png", dpi=160)
plt.close(fig)

angles = [0, math.pi/4, math.pi/2, math.pi]
real = []
for theta in angles:
    qc = QuantumCircuit(1, 1)
    qc.ry(theta, 0)
    qc.measure(0, 0)
    counts = backend.run(qc, shots=10_000).result().get_counts()
    real.append(100 * counts.get("0", 0) / 10_000)
fig, ax = plt.subplots(figsize=(7, 4))
ax.plot([0, 45, 90, 180], real, "o-", label="Simulación P(0)")
ax.plot([0, 45, 90, 180], [100 * math.cos(theta / 2) ** 2 for theta in angles], "--", label="Teoría P(0)")
ax.set(title="Rotaciones Ry en la esfera de Bloch", xlabel="Ángulo θ (grados)", ylabel="Probabilidad (%)", xticks=[0, 45, 90, 180], ylim=(0, 105))
ax.legend()
fig.tight_layout()
fig.savefig(out / "grafica_rotaciones.png", dpi=160)
plt.close(fig)
print("Gráficas generadas en resultados/:")
print("- grafica_bell.png")
print("- grafica_rotaciones.png")
