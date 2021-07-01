#pragma once

#include "Screen.h"

namespace Ui {
  struct Compositor {
    void CreateSplashScreen(Screen* screen);
    void AddScreen(Screen* screen);
    void RemoveScreen(Screen* screen);    
    static Compositor& Instance();
    std::vector<Screen*> screens;
  };
};

