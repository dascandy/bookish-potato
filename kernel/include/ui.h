#pragma once

#include "Screen.h"
#include <flatmap>

struct HidDevice {
  enum class Type {
    Mouse = 0x10002,
    Joystick = 0x10004,
    Gamepad = 0x10005,
    Keyboard = 0x10006,
    Keypad = 0x10007,
    MultiAxisController = 0x10008,
    Touchscreen = 0xd0004,
  };
  virtual Type getType() = 0;
  struct Listener {
    virtual void HandleReport(HidDevice& dev, s2::flatmap<uint32_t, int16_t> report) = 0;
  };
  void RegisterListener(Listener* l) {
    listeners.push_back(l);
  }
  void UnregisterListener(Listener* l) {
    for (size_t n = 0; n < listeners.size(); n++) {
      if (listeners[n] == l) {
        listeners[n] = listeners.back(); 
        listeners.pop_back();
      }
    }
  }
  s2::vector<Listener*> listeners;
};

namespace Ui {
  struct Compositor : HidDevice::Listener {
    void CreateSplashScreen(Screen* screen);
    void AddScreen(Screen* screen);
    void RemoveScreen(Screen* screen);    
    static Compositor& Instance();
    s2::vector<Screen*> screens;

    void RegisterHidDevice(HidDevice& dev) {
      dev.RegisterListener(this);
    }
    void UnregisterHidDevice(HidDevice& dev) {
      dev.UnregisterListener(this);
    }
    void HandleReport(HidDevice& dev, s2::flatmap<uint32_t, int16_t> report) override;

    int32_t pointerX = 0, pointerY = 0;
    bool mouseLeft = false, mouseRight = false, mouseMiddle = false;
  };
};

