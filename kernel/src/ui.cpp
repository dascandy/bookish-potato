#include "ui.h"
#include "debug.h"

void Ui::Compositor::CreateSplashScreen(Screen* screen) {
  uint8_t* buffer = (uint8_t*)screen->GetFreeBuffer();
  debug("buffer at {x} with res {}x{}\n", buffer, screen->currentResolution.width,screen->currentResolution.height);
  for (size_t n = 0; n < screen->currentResolution.height; n++) {
    for (size_t m = 0; m < screen->currentResolution.width; m++) {
      buffer[4*m+4*screen->currentResolution.width*n+0] = n % 256;
      buffer[4*m+4*screen->currentResolution.width*n+1] = m % 256;
      buffer[4*m+4*screen->currentResolution.width*n+2] = 128;
      buffer[4*m+4*screen->currentResolution.width*n+3] = 128;
    }
  }

  screen->QueueBuffer(buffer);
}

void Ui::Compositor::AddScreen(Screen* screen) {
  screens.push_back(screen);
//  if (screens.size() == 1) {
    screens.back()->SetActiveResolution(screens.back()->supportedResolutions.front(), 1);
    CreateSplashScreen(screens.back());
//  }
}

void Ui::Compositor::RemoveScreen(Screen* screen) {
  for (size_t n = 0; n < screens.size(); n++) {
    if (screens[n] == screen) {
      screens[n] = screens.back();
      screens.pop_back();
    }
  }
}

Ui::Compositor& Ui::Compositor::Instance() 
{ 
  static Ui::Compositor compositor; 
  return compositor; 
}

