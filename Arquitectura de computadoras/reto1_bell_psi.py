"""Reto 1: crear |Psi+> = (|01> + |10>)/sqrt(2)."""
from qiskit import QuantumCircuit
from qiskit_aer import AerSimulator

qc = QuantumCircuit(2, 2)
qc.h(0)
qc.cx(0, 1)
qc.x(1)  # Pauli-X sobre q1: transforma Phi+ en Psi+
qc.measure([0, 1], [0, 1])
print(qc.draw(output="text"))
counts = AerSimulator().run(qc, shots=10_000).result().get_counts()
print("RESULTADO RETO 1:", counts)
assert set(counts).issubset({"01", "10"})
print("CORRECTO: únicamente se obtienen |01> y |10>.")
