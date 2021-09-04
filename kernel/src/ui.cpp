#include "ui.h"
#include "debug.h"

void Ui::Compositor::CreateSplashScreen(Screen* screen) {
  uint8_t* buffer = (uint8_t*)screen->GetFreeBuffer();
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
  debug("[UI] Added screen {x}\n", screen);
  screens.push_back(screen);
  if (screens.size() == 1) {
    screens.back()->SetActiveResolution(screens.back()->supportedResolutions.front(), 1);
    CreateSplashScreen(screens.back());
  }
}

void Ui::Compositor::RemoveScreen(Screen* screen) {
  debug("[UI] Added screen {x}\n", screen);
  for (size_t n = 0; n < screens.size(); n++) {
    if (screens[n] == screen) {
      screens[n] = screens.back();
      screens.pop_back();
    }
  }
}

void Ui::Compositor::HandleReport(HidDevice& dev, s2::flatmap<uint32_t, int16_t> report) {
  debug("[UI] Device {x} type {} report\n", &dev, (uint32_t)dev.getType());
  for (auto& [k,v] : report) {
    debug("[UI]   {}={}\n", k, v);
  }

  if (dev.getType() == HidDevice::Type::Mouse) {
    pointerX += report[0x10030];
    if (pointerX < 0) pointerX = 0;
    if (pointerX >= screens[0]->currentResolution.width) pointerX = screens[0]->currentResolution.width - 1;
    pointerY += report[0x10031];
    if (pointerY < 0) pointerY = 0;
    if (pointerY >= screens[0]->currentResolution.height) pointerY = screens[0]->currentResolution.height - 1;
    debug("[UI] Mouse now at {}/{}\n", pointerX, pointerY);
    // TODO: click detection
  }
}

Ui::Compositor& Ui::Compositor::Instance() 
{ 
  static Ui::Compositor compositor; 
  return compositor; 
}

