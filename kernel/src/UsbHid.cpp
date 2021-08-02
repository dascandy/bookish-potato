#include "UsbHid.h"
#include "debug.h"

enum ReportEntry {
  Input = 0x80,
  Output = 0x90,
  Collection = 0xA0,
  CollectionEnd = 0xC0,

  UsagePage = 0x04,
  LogicalMinimum = 0x14,
  LogicalMaximum = 0x24,
  PhysicalMinimum = 0x34,
  PhysicalMaximum = 0x44,
  UnitExponent = 0x54,
  Unit = 0x64,
  ReportSize = 0x74,
  ReportId = 0x84,
  ReportCount = 0x94,
  Push = 0xA4,
  Pop = 0xB4,

  Usage = 0x08,
  UsageMinimum = 0x18,
  UsageMaximum = 0x28,
  DesignatorIndex = 0x38,
  DesignatorMinimum = 0x48,
  DesignatorMaximum = 0x58,
  StringIndex = 0x78,
  StringMinimum = 0x88,
  StringMaximum = 0x98,
  Delimiter = 0xA8,
};

struct ReportIterator {
  const uint8_t* p;
  const uint8_t* e;
  struct sentinel {};
  auto& begin() { return *this; }
  auto end() { return sentinel{}; }
  bool operator==(const sentinel&) const {
    return p >= e; 
  }
  void operator++() {
    uint8_t v = *p;
    switch(v & 0x3) {
      case 0:
        p++; break;
      case 1:
        p += 2; break;
      case 2:
        p += 3; break;
      case 3:
        p += 5; break;
    }
  }
  s2::pair<uint8_t, uint32_t> operator*() {
    uint8_t v = *p, r = v & 0xFC;
    if (v == 0xFE) assert(false);
    switch(v & 0x3) {
      case 0:
        return { r, 0 };
      case 1:
        return { r, p[1] };
      case 2:
        return { r, (uint32_t)((p[1] << 8) | (p[2])) };
      case 3:
        return { r, (uint32_t)((p[1] << 24) | (p[2] << 16) | (p[3] << 8) | (p[4])) };
    }
    __builtin_unreachable();
  }
};

void UsbHid::Initialize() {
  UsbDriver* driver = new UsbHid();
  UsbCore::Instance().RegisterClassDriver(0x030000, *driver);
}

UsbHidDevice::UsbHidDevice(UsbInterface& in) 
: in(in)
{
}

struct ReportState {
  uint8_t usagePage = 0;
  uint32_t logicalMinimum = 0, logicalMaximum = 0;
  uint32_t physicalMinimum = 0, physicalMaximum = 0;
  uint32_t unitExponent = 0;
  uint32_t unit = 0;
  uint8_t reportSize = 0;
  uint32_t reportId = 0;
  uint32_t reportCount = 0;
};

void UsbHidDevice::ParseReport(s2::span<const uint8_t> data) {
  ReportState state;
  s2::vector<uint32_t> usages;
  uint16_t usageMin = 0, usageMax = 0;
  uint16_t inputOffset = 0, outputOffset = 0;
  ReportIterator it{data.data(), data.data() + data.size()};

  for (auto [k, v] : it) {
    switch(k) {
      case Input: 
      case Output: 
      {
        s2::vector<HidField>& t = (k == Input ? input : output);
        uint16_t& offset = (k == Input ? inputOffset : outputOffset);
        bool isConstant = (v & 1), isVariable = (v & 2), isRelative = (v & 4);
        for (size_t n = 0; n < state.reportCount; n++) {
          if (not isConstant) { // if it's a constant we don't care
            uint32_t currentUsage;
            if (isVariable) {
              if (usages.empty()) {
                currentUsage = usageMin;
                usageMin++;
              } else {
                currentUsage = usages[0];
                for (size_t n = 0; n < usages.size() - 1; n++) {
                  usages[n] = usages[n+1];
                }
                usages.pop_back();
              }
            } else {
              currentUsage = usageMin;
            }
            currentUsage |= (state.usagePage << 16);
            t.push_back(HidField{currentUsage, offset, state.reportSize, not isVariable});
          }
          offset += state.reportSize;
        }
      }
        break;
      case Collection: usages.clear(); break;
      case CollectionEnd: break;

      case UsagePage: state.usagePage = v; break;
      case LogicalMinimum: state.logicalMinimum = v; break;
      case LogicalMaximum: state.logicalMaximum = v; break;
      case PhysicalMinimum: state.physicalMinimum = v; break;
      case PhysicalMaximum: state.physicalMaximum = v; break;
      case UnitExponent: state.unitExponent = v; break;
      case Unit: state.unit = v; break;
      case ReportSize: state.reportSize = v; break;
      case ReportId: state.reportId = v; break;
      case ReportCount: state.reportCount = v; break;
      case Push: debug("Push {x}\n", v); break;
      case Pop: debug("Pop {x}\n", v); break;

      case Usage: usages.push_back(v); break;
      case UsageMinimum: usageMin = v; break;
      case UsageMaximum: usageMax = v; break;

      case DesignatorIndex: debug("DesignatorIndex {x}\n", v); break;
      case DesignatorMinimum: debug("DesignatorMinimum {x}\n", v); break;
      case DesignatorMaximum: debug("DesignatorMaximum {x}\n", v); break;
      case StringIndex: debug("StringIndex {x}\n", v); break;
      case StringMinimum: debug("StringMinimum {x}\n", v); break;
      case StringMaximum: debug("StringMaximum {x}\n", v); break;
      case Delimiter: debug("Delimiter {x}\n", v); break;
      default: debug("Unknown {x}\n", v); break;
    }
  }
  if (false) {
    debug("Input:\n");
    for (auto& f : input) {
      debug("  {} {} {} {x}\n", f.bitOffset, f.bitLength, f.isIndex, f.usage);
    }
    debug("Output:\n");
    for (auto& f : output) {
      debug("  {} {} {} {x}\n", f.bitOffset, f.bitLength, f.isIndex, f.usage);
    }
  }
}

s2::future<void> UsbHidDevice::start() {
  HIDDescriptor* desc = nullptr;
  for (auto usbd : in.GetInterfaceDescriptors()) {
    if (usbd->type == (int)DescriptorType::HID) {
      desc = (HIDDescriptor*)usbd;
      break;
    }
  }
  uint8_t* reportDesc = nullptr;
  size_t len = 0;
  assert(desc != nullptr);
  for (size_t n = 0; n < desc->numDescriptors; n++) {
    if (desc->entries[n].descriptorType == (int)DescriptorType::Report) {
      len = desc->entries[n].descriptorLength;
      reportDesc = new uint8_t[len];
      // fetch descriptor
      auto data = co_await in.GetDescriptor(DescriptorType::Report, 0, desc->entries[n].descriptorLength);
      memcpy(reportDesc, data.data(), data.size());
    }
  }
  assert(reportDesc != nullptr);

  ParseReport(s2::span<const uint8_t>(reportDesc, reportDesc + len));
}

void UsbHid::AddDevice(UsbDevice& dev) {}

void UsbHid::AddInterface(UsbInterface& interface) {
  (new UsbHidDevice(interface))->start();
}

