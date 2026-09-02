#include <iostream>
#include <fstream>
#include <cstring>

using namespace std;

int main()
{
    int arreglo1[1000];
    int arreglo2[100];

    // =========================================
    // 1. LEER LOS 1000 ELEMENTOS DEL ARCHIVO
    //    (memoria secundaria)
    // =========================================

    ifstream archivo("datos.txt");

    if (!archivo)
    {
        cout << "Error: No se pudo abrir datos.txt" << endl;
        return 1;
    }

    for (int i = 0; i < 1000; i++)
    {
        archivo >> arreglo1[i];
    }

    archivo.close();

    cout << "Datos cargados correctamente desde memoria secundaria."
         << endl;

    // =========================================
    // 2. MOSTRAR PRIMEROS 5 ELEMENTOS
    // =========================================

    cout << "\nPrimeros 5 elementos:" << endl;

    for (int i = 0; i < 5; i++)
    {
        cout << arreglo1[i] << " ";
    }

    // =========================================
    // 3. MOSTRAR ULTIMOS 5 ELEMENTOS
    // =========================================

    cout << "\n\nUltimos 5 elementos:" << endl;

    for (int i = 995; i < 1000; i++)
    {
        cout << arreglo1[i] << " ";
    }

    // =========================================
    // 4. COPIAR POSICIONES 500 - 599
    // =========================================

    memcpy(
        arreglo2,
        &arreglo1[500],
        100 * sizeof(int)
    );

    // =========================================
    // 5. LIMPIAR POSICIONES ORIGINALES
    // =========================================

    memset(
        &arreglo1[500],
        0,
        100 * sizeof(int)
    );

    // =========================================
    // 6. MOSTRAR LOS 100 ELEMENTOS COPIADOS
    // =========================================

    cout << "\n\nElementos transferidos al segundo arreglo:" << endl;

    for (int i = 0; i < 100; i++)
    {
        cout << arreglo2[i] << " ";

        if ((i + 1) % 10 == 0)
        {
            cout << endl;
        }
    }

    // =========================================
    // 7. VALIDAR QUE 500 - 599 SEAN CERO
    // =========================================

    bool correcto = true;

    for (int i = 500; i <= 599; i++)
    {
        if (arreglo1[i] != 0)
        {
            correcto = false;
            break;
        }
    }

    cout << "\nValidacion: ";

    if (correcto)
    {
        cout << "Los indices 500 al 599 contienen ceros." << endl;
    }
    else
    {
        cout << "Error: algunos valores no fueron limpiados." << endl;
    }

    return 0;
}