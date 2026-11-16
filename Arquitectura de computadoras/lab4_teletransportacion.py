"""Práctica 4: teletransportación con correcciones CLÁSICAS de Bob."""
import math
from qiskit import QuantumCircuit
from qiskit_aer import AerSimulator


def construir_circuito(medida: str = "Z") -> QuantumCircuit:
    # q0 = estado de Alice, q1 = mitad de Alice del par de Bell, q2 = mitad de Bob
    # c0 y c1 = 2 bits clásicos enviados por Alice; c2 = medida final de Bob
    qc = QuantumCircuit(3, 3)
    qc.ry(math.pi / 3, 0)  # |psi> = sqrt(3)/2 |0> + 1/2 |1>
    qc.barrier()
    qc.h(1)
    qc.cx(1, 2)  # Par de Bell compartido
    qc.barrier()
    qc.cx(0, 1)
    qc.h(0)
    qc.measure(0, 0)
    qc.measure(1, 1)
    qc.barrier()

    # Correcciones de Bob: dependen de bits CLÁSICOS medidos por Alice.
    # Es equivalente al principio del circuito del PDF, pero usa feed-forward real.
    with qc.if_test((qc.clbits[1], 1)):
        qc.x(2)
    with qc.if_test((qc.clbits[0], 1)):
        qc.z(2)

    if medida == "X":
        qc.h(2)  # Medición en base X, para comprobar también la coherencia de fase
    elif medida != "Z":
        raise ValueError("La base de medición debe ser X o Z")
    qc.barrier()
    qc.measure(2, 2)
    return qc


def medir_bob(circuito, shots=10_000):
    counts = AerSimulator().run(circuito, shots=shots).result().get_counts()
    bob = {"0": 0, "1": 0}
    # Para qc(3,3), Qiskit presenta los bits como c2c1c0.
    for cadena, n in counts.items():
        bob[cadena.replace(" ", "")[0]] += n
    return bob


def main():
    qc = construir_circuito("Z")
    print("CIRCUITO DEL PROTOCOLO DE TELETRANSPORTACIÓN:")
    print(qc.draw(output="text"))
    bob = medir_bob(qc)
    print("\nVERIFICACIÓN EN EL CÚBIT DE BOB (base Z):")
    print(f"P(|0>): {bob['0']/100:.2f}% (Teórico: 75.00%)")
    print(f"P(|1>): {bob['1']/100:.2f}% (Teórico: 25.00%)")
    assert 0.72 <= bob['0'] / 10_000 <= 0.78, "La probabilidad Z está fuera del intervalo esperado"

    # Prueba adicional: la base Z por sí sola no demuestra la fase del estado.
    bob_x = medir_bob(construir_circuito("X"))
    teorico_plus = (1 + math.sin(math.pi / 3)) / 2 * 100
    print("\nCOMPROBACIÓN ADICIONAL (base X, coherencia del estado):")
    print(f"P(|+>): {bob_x['0']/100:.2f}% (Teórico: {teorico_plus:.2f}%)")
    assert abs(bob_x['0'] / 100 - teorico_plus) < 3.0, "La comprobación en base X se alejó del valor esperado"
    print("VERIFICACIÓN: el estado recuperado coincide con las probabilidades esperadas en Z y X.")


if __name__ == "__main__":
    main()
