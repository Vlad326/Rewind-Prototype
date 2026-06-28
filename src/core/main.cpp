#include "core/Engine.h"

int main() {
    Engine engine;
    if (!engine.init()) {
        return -1;
    }
    return engine.run();
}