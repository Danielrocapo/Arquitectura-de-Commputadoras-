"""Práctica 3: algoritmo de Deutsch para 4 funciones booleanas."""
from qiskit import QuantumCircuit
from qiskit_aer import AerSimulator


def construir_oraculo(tipo: str) -> QuantumCircuit:
    oraculo = QuantumCircuit(2, name=f"U_f ({tipo})")
    if tipo == "constante_0":
        pass
    elif tipo == "constante_1":
        oraculo.x(1)
    elif tipo == "balanceada_identidad":
        oraculo.cx(0, 1)
    elif tipo == "balanceada_inversa":
        oraculo.x(0)
        oraculo.cx(0, 1)
        oraculo.x(0)
    else:
        raise ValueError(f"Oráculo desconocido: {tipo}")
    return oraculo


def ejecutar_deutsch(tipo: str):
    qc = QuantumCircuit(2, 1)
    qc.x(1)  # Ancilla inicial: |1>
    qc.h(0)  # q0: |+>
    qc.h(1)  # q1: |-> (importante para el phase kickback)
    qc.compose(construir_oraculo(tipo), inplace=True)
    qc.h(0)
    qc.measure(0, 0)
    counts = AerSimulator().run(qc, shots=1_000).result().get_counts()
    return qc, counts


def main():
    casos = ["constante_0", "constante_1", "balanceada_identidad", "balanceada_inversa"]
    for caso in casos:
        _, counts = ejecutar_deutsch(caso)
        bit = next(iter(counts))
        conclusion = "CONSTANTE" if bit == "0" else "BALANCEADA"
        esperado = "0" if caso.startswith("constante") else "1"
        assert counts == {esperado: 1000}, f"Resultado inesperado de {caso}: {counts}"
        print(f"Oráculo: {caso:22s} | Medición q0: '{bit}' -> Función {conclusion}")
    print("VERIFICACIÓN: los cuatro oráculos fueron clasificados correctamente.")


if __name__ == "__main__":
    main()
