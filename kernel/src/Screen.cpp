#include "Screen.h"
#include "ui.h"
#include "debug.h"

namespace {

consteval uint16_t brandToId(const char* brand) {
  return ((brand[0] & 0x1F) << 10) | ((brand[1] & 0x1F) << 5) | ((brand[2] & 0x1F) << 0);
}

s2::string translate(uint16_t brandId) {
  switch(brandId) {
  case brandToId("ACI"): return "Asus";
  case brandToId("ACR"): return "Acer";
  case brandToId("ACT"): return "Targa";
  case brandToId("ADI"): return "ADI Corp";
  case brandToId("AOC"): return "AOC";
  case brandToId("API"): return "Acer";
  case brandToId("APP"): return "Apple";
  case brandToId("ART"): return "ArtMedia";
  case brandToId("AST"): return "AST Research";
  case brandToId("AUO"): return "AU Optronics";
  case brandToId("BNQ"): return "BenQ";
  case brandToId("BOE"): return "BOE";
  case brandToId("CPL"): return "Compal / ALFA";
  case brandToId("CPQ"): return "COMPAQ";
  case brandToId("CTX"): return "Chuntex";
  case brandToId("DEC"): return "Digital Equipment Corporation";
  case brandToId("DEL"): return "Dell";
  case brandToId("DPC"): return "Delta";
  case brandToId("DWE"): return "Daewoo";
  case brandToId("ECS"): return "ELITEGROUP";
  case brandToId("EIZ"): return "EIZO";
  case brandToId("EPI"): return "Envision";
  case brandToId("FCM"): return "Funai";
  case brandToId("FUS"): return "Fujitsu-Siemens";
  case brandToId("GSM"): return "LG Electronics (GoldStar)";
  case brandToId("GWY"): return "Gateway";
  case brandToId("HEI"): return "Hyundai Electronics";
  case brandToId("HIQ"): return "Hyundai ImageQuest";
  case brandToId("HIT"): return "Hitachi";
  case brandToId("HSD"): return "Hannspree";
  case brandToId("HSL"): return "Hansol";
  case brandToId("HTC"): return "Hitachi";
  case brandToId("HWP"): return "Hewlett Packard";
  case brandToId("HPN"): return "Hewlett Packard";
  case brandToId("IBM"): return "IBM";
  case brandToId("ICL"): return "Fujitsu ICL";
  case brandToId("IFS"): return "InFocus";
  case brandToId("IQT"): return "Hyundai";
  case brandToId("IVM"): return "Iiyama";
  case brandToId("KDS"): return "KDS USA";
  case brandToId("KFC"): return "KFC Computek";
  case brandToId("LEN"): return "Lenovo";
  case brandToId("LGD"): return "LG Display";
  case brandToId("LKM"): return "ADLAS / AZALEA";
  case brandToId("LNK"): return "LINK Technologies";
  case brandToId("LPL"): return "LG Philips";
  case brandToId("LTN"): return "Lite-On";
  case brandToId("MAG"): return "MAG InnoVision";
  case brandToId("MAX"): return "Maxdata";
  case brandToId("MEI"): return "Panasonic";
  case brandToId("MEL"): return "Mitsubishi";
  case brandToId("MIR"): return "Miro";
  case brandToId("MTC"): return "MITAC";
  case brandToId("NAN"): return "NANAO";
  case brandToId("NEC"): return "NEC";
  case brandToId("NOK"): return "Nokia";
  case brandToId("NVD"): return "Nvidia";
  case brandToId("OQI"): return "OPTIQUEST";
  case brandToId("PBN"): return "Packard Bell";
  case brandToId("PCK"): return "Daewoo";
  case brandToId("PDC"): return "Polaroid";
  case brandToId("PGS"): return "Princeton Graphic Systems";
  case brandToId("PHL"): return "Philips Consumer";
  case brandToId("PRT"): return "Princeton";
  case brandToId("REL"): return "Relisys";
  case brandToId("RHT"): return "Qemu";
  case brandToId("SAM"): return "Samsung";
  case brandToId("SEC"): return "Seiko Epson";
  case brandToId("SMC"): return "Samtron";
  case brandToId("SMI"): return "Smile";
  case brandToId("SNI"): return "Siemens Nixdorf";
  case brandToId("SNY"): return "Sony";
  case brandToId("SPT"): return "Sceptre";
  case brandToId("SRC"): return "Shamrock";
  case brandToId("STN"): return "Samtron";
  case brandToId("STP"): return "Sceptre";
  case brandToId("TAT"): return "Tatung";
  case brandToId("TRL"): return "Royal Information Company";
  case brandToId("TSB"): return "Toshiba";
  case brandToId("UNM"): return "Unisys";
  case brandToId("VSC"): return "ViewSonic";
  case brandToId("WTC"): return "Wen";
  case brandToId("ZCM"): return "Zenith";
  default:
    {
      char unk[] = "Unknown (@@@)";
      unk[9] += (brandId >> 10) & 0x1F;
      unk[10] += (brandId >> 5) & 0x1F;
      unk[11] += (brandId >> 0) & 0x1F;
      return unk;
    }
  }
}

}

Screen::Screen(s2::span<const uint8_t> edid) {
  // TODO: validate
  connection = "unknown";
  manufacturer = translate((edid[8] << 8) | edid[9]);
  widthMm = edid[66] + ((edid[68] & 0xF0) << 4);
  heightMm = edid[67] + ((edid[68] & 0xF) << 8);
  uint16_t width = edid[56] + ((edid[58] & 0xF0) << 4);
  uint16_t height = edid[59] + ((edid[61] & 0xF0) << 4);
  for (size_t n = 0; n < 4; n++) {
    if (edid[54 + 18 * n] == 0 &&
        edid[55 + 18 * n] == 0 &&
        edid[56 + 18 * n] == 0) {
      switch(edid[57 + 18 * n]) {
      case 0xFF:
      {
        uint8_t* b = (uint8_t*)edid.data() + 58 + 18*n;
        uint8_t* e = b;
        while (e - b != 13 && *e != 10) e++;
        // TODO: transcode from CP437
        serialno = s2::string::from_cp437(s2::span<const uint8_t>(b, e));
      }
        break;
      case 0xFC:
      {
        uint8_t* b = (uint8_t*)edid.data() + 58 + 18*n;
        uint8_t* e = b;
        while (e - b != 13 && *e != 10) e++;
        // TODO: transcode from CP437
        name = s2::string::from_cp437(s2::span<const uint8_t>(b, e));
      }
        break;
      }
    }
  }

  supportedResolutions.push_back({width, height, PixelFormat::RGBX_8888});
  dotsPerInch = uint32_t(25.4 * width / widthMm);
  debug("[SCR] Found screen {} {} #{} ({} dpi) of {}x{} mm with {}x{} pixels\n", manufacturer, name, serialno, dotsPerInch, widthMm, heightMm, width, height);
}

// Pretend to be a 23" 1080p screen, a 46" 4K screen or a 15.3" 720p screen
// these all are basically 96DPI.
Screen::Screen(uint16_t xres, uint16_t yres) 
: widthMm(xres/4)
, heightMm(yres/4)
, dotsPerInch(96)
, name("VM Screen")
, connection("VM")
, manufacturer("VM")
, serialno("1")
, displayBpp(8)
{
  supportedResolutions.push_back({xres, yres, PixelFormat::RGBX_8888});
}

void Screen::Register() {
  Ui::Compositor::Instance().AddScreen(this);
}

Screen::~Screen() {
  Ui::Compositor::Instance().RemoveScreen(this);
}


