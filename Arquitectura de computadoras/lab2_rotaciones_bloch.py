"""Práctica 2: rotaciones Ry y probabilidades en la esfera de Bloch."""
import math
from qiskit import QuantumCircuit
from qiskit_aer import AerSimulator


def simular_angulo(theta: float, shots: int = 10_000):
    qc = QuantumCircuit(1, 1)
    qc.ry(theta, 0)
    qc.measure(0, 0)
    counts = AerSimulator().run(qc, shots=shots).result().get_counts()
    return counts, qc


def main():
    angulos = [
        (0.0, "Polo Norte (theta = 0)"),
        (math.pi / 4, "45 grados (theta = pi/4)"),
        (math.pi / 2, "Ecuador (theta = pi/2)"),
        (math.pi, "Polo Sur (theta = pi)"),
    ]
    for theta, desc in angulos:
        counts, _ = simular_angulo(theta)
        p0 = 100 * counts.get("0", 0) / 10_000
        p1 = 100 * counts.get("1", 0) / 10_000
        teorico_p0 = 100 * math.cos(theta / 2) ** 2
        teorico_p1 = 100 * math.sin(theta / 2) ** 2
        print(f"{desc:32s} -> Medido P(0): {p0:5.1f}% | P(1): {p1:5.1f}%")
        print(f"{'':32s}    Teoría P(0): {teorico_p0:5.1f}% | P(1): {teorico_p1:5.1f}%")


if __name__ == "__main__":
    main()
