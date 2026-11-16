"""Reto 2: comprobar por qué la ancilla necesita X antes de H."""
from qiskit import QuantumCircuit
from qiskit_aer import AerSimulator


def simular(incluir_x):
    qc = QuantumCircuit(2, 1)
    if incluir_x:
        qc.x(1)
    qc.h(0)
    qc.h(1)
    qc.cx(0, 1)  # f(x) = x, una función BALANCEADA
    qc.h(0)
    qc.measure(0, 0)
    return AerSimulator().run(qc, shots=1000).result().get_counts()

correcto = simular(True)
incorrecto = simular(False)
print("Función real: f(x) = x (BALANCEADA)")
print("Con X en la ancilla: ", correcto, "-> se mide 1, balanceada")
print("Sin X en la ancilla:", incorrecto, "-> se mide 0, falsa conclusión constante")
assert correcto == {"1": 1000} and incorrecto == {"0": 1000}
