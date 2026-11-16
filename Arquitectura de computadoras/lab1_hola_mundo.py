"""Práctica 1: superposición y entrelazamiento, estado de Bell Phi+."""
from qiskit import QuantumCircuit
from qiskit_aer import AerSimulator


def main():
    qc = QuantumCircuit(2, 2)
    qc.h(0)  # Hadamard: |0> -> (|0> + |1>)/sqrt(2)
    qc.cx(0, 1)  # Entrelaza los cúbits en (|00> + |11>)/sqrt(2)
    qc.measure([0, 1], [0, 1])

    print("CIRCUITO CUÁNTICO - ESTADO DE BELL |Phi+>:")
    print(qc.draw(output="text"))
    shots = 10_000
    counts = AerSimulator().run(qc, shots=shots).result().get_counts()
    print(f"\nRESULTADOS DE LA MEDICIÓN ({shots} disparos):")
    for estado in ("00", "01", "10", "11"):
        n = counts.get(estado, 0)
        print(f"Estado |{estado}>: {n:5d} ocurrencias ({100*n/shots:5.2f}%)")
    assert counts.get("01", 0) == 0 and counts.get("10", 0) == 0, "Falló la correlación del estado de Bell"
    print("VERIFICACIÓN: el estado de Bell genera exclusivamente |00> y |11>.")


if __name__ == "__main__":
    main()
