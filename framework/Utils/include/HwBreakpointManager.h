#ifndef HW_BREAKPOINT_MANAGER_H
#define HW_BREAKPOINT_MANAGER_H
#include <signal.h>
#include <stddef.h>

typedef struct HardwareBpStruct
{
	/*正式断点*/
	size_t hwBpAddr;
	size_t hwBpLen;
	size_t hwBpType;
	size_t hwBpHandler;
	/*用来为备胎断点保存原来正式断点用的。*/
	size_t resetHwBpAddr;
	size_t resetHwBpLen;
	size_t resetHwBpType;
	size_t resetHwBpHandler;
	//其他东西
	size_t hProcess;
	size_t isNextBp;
	size_t hwBpPairType;
	size_t hwBpIsEmu;
} HardwareBpStruct;

enum
{
	HW_BREAKPOINT_LEN_1 = 1,
	HW_BREAKPOINT_LEN_2 = 2,
	HW_BREAKPOINT_LEN_4 = 4,
	HW_BREAKPOINT_LEN_8 = 8,
};

enum
{
	HW_BREAKPOINT_EMPTY = 0,
	HW_BREAKPOINT_R = 1,
	HW_BREAKPOINT_W = 2,
	HW_BREAKPOINT_RW = HW_BREAKPOINT_R | HW_BREAKPOINT_W,
	HW_BREAKPOINT_X = 4,
	HW_BREAKPOINT_INVALID = HW_BREAKPOINT_RW | HW_BREAKPOINT_X,
};

HardwareBpStruct findHardwareBpStruct(size_t addr);

size_t connectDriver();

bool disconnectDriver();

int getNumBRPS();

int getNumWRPS();

size_t addProcessHwBp(HardwareBpStruct *objHardwareBpStruct);

bool delProcessHwBp(HardwareBpStruct *objHardwareBpStruct);

HardwareBpStruct
initHardwareBpStruct(size_t hwBpAddr, size_t hwBpLen, size_t hwBpType, size_t hwBpHandler,
					 size_t resetHwBpAddr, size_t resetHwBpLen, size_t resetHwBpType, size_t resetHwBpHandler, size_t hProcess, size_t isNextBp,
					 size_t hwBpPairType, size_t hwBpIsEmu);

void registerSignal44HwBp();

void sig44HwbpHandler(int signumber, siginfo_t *info, void *uc);

void initHardwareModule();

bool addHardwareBreakpoint(size_t hwBpAddr, size_t hwBpLen, size_t hwBpType, size_t hwBpHandler,
                           size_t resetHwBpAddr, size_t resetHwBpLen, size_t resetHwBpType, size_t resetHwBpHandler,
                           size_t hProcess, size_t isNextBp, size_t hwBpPairType, size_t hwBpIsEmu);
#endif
