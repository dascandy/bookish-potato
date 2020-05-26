#include "mailbox.h"
#include "debug.h"

void print_properties() {
  uint32_t vcversion, boardmodel, boardrev;
  uint64_t serial;
  uint8_t mac[6];
  struct {
    uint32_t baseaddr, size;
  } armmemory, vcmemory;
  char commandline[256];
  property_read(MailboxProperty::VideocoreVersion, (uint8_t*)&vcversion, sizeof(vcversion));
  property_read(MailboxProperty::HardwareBoardModel, (uint8_t*)&boardmodel, sizeof(boardmodel));
  property_read(MailboxProperty::HardwareBoardRevision, (uint8_t*)&boardrev, sizeof(boardrev));
  property_read(MailboxProperty::HardwareBoardSerial, (uint8_t*)&serial, sizeof(serial));
  property_read(MailboxProperty::HardwareBoardMacAddress, (uint8_t*)&mac, sizeof(mac));
  property_read(MailboxProperty::HardwareArmMemory, (uint8_t*)&armmemory, sizeof(armmemory));
  property_read(MailboxProperty::HardwareVcMemory, (uint8_t*)&vcmemory, sizeof(vcmemory));
  uint32_t cmdlinelength = property_read(MailboxProperty::CommandLine, (uint8_t*)&commandline, sizeof(commandline) - 1);
  commandline[cmdlinelength] = 0;
  debug("VC version = {x}\n", vcversion);
  debug("Hardware version = {x} / {x}\n", boardmodel, boardrev);
  debug("Hardware serial = {x}\n", serial);
  debug("Mac address = {x}:{x}:{x}:{x}:{x}:{x}\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  debug("Arm memory = {x} -> {x}\n", armmemory.baseaddr, armmemory.size);
  debug("VC  memory = {x} -> {x}\n", vcmemory.baseaddr, vcmemory.size);
  debug("Commandline = {s}\n", commandline);
//  VideocoreVersion = 1,
//  HardwareBoardModel = 0x10001,
//  HardwareBoardRevision = 0x10002,
//  HardwareBoardMacAddress = 0x10003,
//  HardwareBoardSerial = 0x10004,
//  HardwareArmMemory = 0x10005,
//  HardwareVcMemory = 0x10006,
//  CommandLine = 0x50001,
}

