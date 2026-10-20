#pragma once

namespace hud6 {

class Menu {
public:
    static void Init();
    static void Shutdown();
    static void Process();
    static void Render();
    static bool IsOpen();
    static bool IsLayoutPreview();
};

} // namespace hud6
