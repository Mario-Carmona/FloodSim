
#pragma once
#include "UI/UserInterface.h"

class CLI_Interface : public UserInterface {
protected:
    void userPressStart(SimConfig& cfg);

    void renderLoop() override; // Implementación ASCII
};
