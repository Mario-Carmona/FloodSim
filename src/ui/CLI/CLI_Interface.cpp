
#include "UI/CLI/CLI_Interface.h"

void CLI_Interface::renderLoop() {
    StatePtr currentState = nullptr;
        
    while (true) {
        // Espera activa (o pasiva según implementación) del siguiente frame
        bool active = bridge->waitForState(currentState);
        
        if (!active) break; // Salir si el bridge cierra

        // Renderizar
        //visualizeGridASCII(currentState);

        // Simular carga de renderizado (ej. 100ms)
        // En modo Lossless, esto frenará al Core. En RealTime, el Core saltará frames.
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
