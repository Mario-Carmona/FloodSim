
#pragma once
#include "UI/UserInterface.h"

class CLI_Interface : public UserInterface {
protected:
    void renderLoop() override; // Implementación ASCII
};
