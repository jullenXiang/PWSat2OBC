#ifndef BOOT_COMMANDS_COMMANDS_HPP_
#define BOOT_COMMANDS_COMMANDS_HPP_

void Test();
void TestSRAM();
void TestEEPROM();

void BootUpper();
void SetRunlevel();

void SetBootIndex();
void ShowBootSettings();
void SetBootSlotToSafeMode();
void SetBootSlotToUpper();

void UploadApplication();
void UploadSafeMode();

void PrintBootTable();
void EraseBootTable();

#endif /* BOOT_COMMANDS_COMMANDS_HPP_ */
