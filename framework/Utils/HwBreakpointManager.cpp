#include <Util.h>
#include <NativeSandBox.h>
#include <HwBreakpointManager.h>
#include <dlfcn.h>
#include <signal.h>
#include <sys/ucontext.h>
#include <linux/types.h>
#include <PrintStack.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <malloc.h>
#include <map>
#include <Log.h>
#include <GlobalInfo.h>
#include <asm/sigcontext.h>

//当前驱动版本号
#define SYS_VERSION 01
#define DEV_FILENAME "/dev/myhwbp"
#define MAJOR_NUM 100
#define IOCTL_CLEAR_ALL_HWBPS _IOR(MAJOR_NUM, 1, char *)  //删除所有残留断点
#define IOCTL_GET_NUM_BRPS _IOR(MAJOR_NUM, 3, char *)	  //获取CPU支持硬件执行断点的数量.
#define IOCTL_GET_NUM_WRPS _IOR(MAJOR_NUM, 4, char *)	  //获取CPU支持硬件访问断点的数量.
#define IOCTL_ADD_PROCESS_HWBP _IOR(MAJOR_NUM, 6, char *) //设置进程硬件断点.
#define IOCTL_DEL_PROCESS_HWBP _IOR(MAJOR_NUM, 7, char *) //删除进程硬件断点.

using namespace std;

static std::map<size_t, HardwareBpStruct> hwBpMap;
static size_t driverLink = -1;

bool delProcessHwBp(HardwareBpStruct *objHardwareBpStruct)
{
	if (driverLink < 0)
	{
		return false;
	}
	if (!objHardwareBpStruct->hwBpHandler)
	{
		return false;
	}
	int res = ioctl(driverLink, IOCTL_DEL_PROCESS_HWBP, objHardwareBpStruct->hwBpHandler);
	if (res != 0)
	{
		LOGI("delProcessHwBp ioctl():%s\n", strerror(errno));
		hwBpMap.erase(objHardwareBpStruct->hwBpAddr);
		return false;
	}
	return true;
}

HardwareBpStruct findHardwareBpStruct(size_t addr)
{
	if (hwBpMap.count(addr) > 0)
	{
		return hwBpMap[addr];
	}
	else
	{
		LOGI("没找到map里对应的HardwareBpStruct");
	}
	return hwBpMap[addr];
}

void printHwBpMap()
{
	LOGI("hwBpMap size:%zx", hwBpMap.size());
	for (int i = 0; i < hwBpMap.size(); ++i)
	{
	}
	return;
}

HardwareBpStruct
initHardwareBpStruct(size_t hwBpAddr, size_t hwBpLen, size_t hwBpType, size_t hwBpHandler,
					 size_t resetHwBpAddr, size_t resetHwBpLen, size_t resetHwBpType, size_t resetHwBpHandler, size_t hProcess, size_t isNextBp,
					 size_t hwBpPairType, size_t hwBpIsEmu)
{
	HardwareBpStruct objStruct = {0};

	objStruct.hwBpAddr = hwBpAddr;
	objStruct.hwBpLen = hwBpLen;
	objStruct.hwBpType = hwBpType;
	objStruct.hwBpHandler = hwBpHandler;

	objStruct.resetHwBpAddr = resetHwBpAddr;
	objStruct.resetHwBpLen = resetHwBpLen;
	objStruct.resetHwBpType = resetHwBpType;
	objStruct.resetHwBpHandler = resetHwBpHandler;

	objStruct.hProcess = hProcess;
	objStruct.isNextBp = isNextBp;
	objStruct.hwBpPairType = hwBpPairType;
	objStruct.hwBpIsEmu = hwBpIsEmu;
	return objStruct;
}

//驱动_新增硬件断点，返回值：TRUE成功，FALSE失败
size_t addProcessHwBp(HardwareBpStruct *objHardwareBpStruct)
{
	if (driverLink < 0)
	{
		return false;
	}
	size_t ret = ioctl(driverLink, IOCTL_ADD_PROCESS_HWBP, objHardwareBpStruct);
	if (ret != 0)
	{
		LOGI("addProcessHwBp ioctl():%s\n", strerror(errno));
		return false;
	}
	else
	{
		hwBpMap[(size_t)objHardwareBpStruct->hwBpAddr] = *objHardwareBpStruct;
	}
	return ret;
}

//驱动_获取CPU支持硬件访问断点的数量，返回值：数量
int getNumWRPS()
{
	if (driverLink < 0)
	{
		return false;
	}
	int res = ioctl(driverLink, IOCTL_GET_NUM_WRPS, 0);
	return res;
}

//驱动_获取CPU支持硬件执行断点的数量，返回值：数量
int getNumBRPS()
{
	if (driverLink < 0)
	{
		return false;
	}
	int res = ioctl(driverLink, IOCTL_GET_NUM_BRPS, 0);
	return res;
}

//断开驱动，返回值：TRUE成功，FALSE失败
bool disconnectDriver()
{
	if (driverLink < 0)
	{
		return false;
	}
	close(driverLink);
	return true;
}

//连接驱动（错误代码），返回值：驱动连接句柄，>=0代表成功
size_t connectDriver()
{
	driverLink = open(DEV_FILENAME, O_RDWR);
	if (driverLink < 0)
	{
		LOGI("open error():%s\n", strerror(errno));
		return false;
	}
	else
	{
		LOGI("open success, ready to CLEAR ALL HWBPS");
		int res = ioctl(driverLink, IOCTL_CLEAR_ALL_HWBPS, 0);
	}
	return driverLink;
}

void sig44HwbpHandler(int signumber, siginfo_t *info, void *uc)
{
	LOGI("sig44 enter");
	ucontext_t *innerUc = (ucontext_t *)uc;
	Dl_info myinfo = {0};
	dladdr((void *)innerUc->uc_mcontext.pc, &myinfo);
	HardwareBpStruct objCurrentHardwareBpStruct = findHardwareBpStruct((size_t)info->_sifields._sigfault._addr);
	LOGI("sig44 hwBpPairType:%d hwBpAddr:%zx isNextBp:%zx hwBpLen:%zx _sigfault._addr:%zx",
		 objCurrentHardwareBpStruct.hwBpPairType,
		 objCurrentHardwareBpStruct.hwBpAddr,
		 objCurrentHardwareBpStruct.isNextBp,
		 objCurrentHardwareBpStruct.hwBpLen,
		 (size_t)info->_sifields._sigfault._addr);
	//sleep(10);
	/* 执行断点 */
	if (objCurrentHardwareBpStruct.hwBpPairType == HW_BREAKPOINT_X)
	{
		// 执行断点的正式断点
		if (!objCurrentHardwareBpStruct.isNextBp)
		{
			LOGI("正式断点CallBack HW_BREAKPOINT_X 开始");
			// 删除正式断点
			delProcessHwBp(&objCurrentHardwareBpStruct);
			if (isHwEmu)
			{
				LOGI("初始化Trampoline HW_BREAKPOINT_X 开始");
				void *trampolineAddr = getEmuEntry((void *)objCurrentHardwareBpStruct.hwBpAddr, E_EntryType_HwExecBp);
				innerUc->uc_mcontext.pc = (size_t)trampolineAddr;
				LOGI("初始化Trampoline HW_BREAKPOINT_X 完成");
			}
			else
			{
				LOGI("初始化备胎断点 HW_BREAKPOINT_X 开始");
				addHardwareBreakpoint((size_t)innerUc->uc_mcontext.pc + 0x4, HW_BREAKPOINT_LEN_4, HW_BREAKPOINT_X, 0, (size_t)objCurrentHardwareBpStruct.hwBpAddr, objCurrentHardwareBpStruct.hwBpLen, objCurrentHardwareBpStruct.hwBpType, 0,
									  0, 1, HW_BREAKPOINT_X, 0);
				LOGI("初始化备胎断点 HW_BREAKPOINT_X 完成");
			}
			dumpStackUnwind();
			LOGI("正式断点CallBack HW_BREAKPOINT_X 完成");
		}
		// 执行断点的备胎断点
		else if (objCurrentHardwareBpStruct.isNextBp)
		{
			LOGI("备胎断点CallBack HW_BREAKPOINT_X start");
			// 删除执行断点的备胎断点
			delProcessHwBp(&objCurrentHardwareBpStruct);
			if (objCurrentHardwareBpStruct.resetHwBpAddr != 0)
			{
				// 初始化原来的正式断点
				addHardwareBreakpoint(objCurrentHardwareBpStruct.resetHwBpAddr, objCurrentHardwareBpStruct.resetHwBpLen, objCurrentHardwareBpStruct.resetHwBpType, objCurrentHardwareBpStruct.resetHwBpHandler,
									  0, 0, 0, 0,
									  0, 0, HW_BREAKPOINT_X, 0);
			}
			else
			{
				LOGI("Can't find ResetHwBp Addr to restore Origin HwBp");
			}
			LOGI("备胎断点CallBack HW_BREAKPOINT_X finish");
		}
	}

	/* 读断点 */
	else if (objCurrentHardwareBpStruct.hwBpPairType == HW_BREAKPOINT_R)
	{
		//读断点的正式断点
		if (!objCurrentHardwareBpStruct.isNextBp)
		{
			LOGI("正式断点CallBack HW_BREAKPOINT_R start");
			// 删除正式断点
			delProcessHwBp(&objCurrentHardwareBpStruct);
			// 初始化备胎断点.
			addHardwareBreakpoint((size_t)innerUc->uc_mcontext.pc + 0x4, HW_BREAKPOINT_LEN_4, HW_BREAKPOINT_X, 0,
								  (size_t)objCurrentHardwareBpStruct.hwBpAddr, objCurrentHardwareBpStruct.hwBpLen, objCurrentHardwareBpStruct.hwBpType, 0,
								  0, 1, HW_BREAKPOINT_R, 0);
			dumpStackUnwind();
			LOGI("正式断点CallBack  HW_BREAKPOINT_R finish");
		}
		//读断点的备胎断点
		else if (objCurrentHardwareBpStruct.isNextBp)
		{
			LOGI("备胎断点CallBack HW_BREAKPOINT_R start");
			//sleep(30);
			// 删除自己断点.
			delProcessHwBp(&objCurrentHardwareBpStruct);
			if (objCurrentHardwareBpStruct.resetHwBpAddr != 0)
			{
				addHardwareBreakpoint(objCurrentHardwareBpStruct.resetHwBpAddr, objCurrentHardwareBpStruct.resetHwBpLen, objCurrentHardwareBpStruct.resetHwBpType, objCurrentHardwareBpStruct.resetHwBpHandler,
									  0, 0, 0, 0,
									  0, 0, HW_BREAKPOINT_R, 0);
			}
			else
			{
				LOGI("Can't find ResetHwBp Addr to restore hwbp");
			}
			LOGI("备胎断点CallBack HW_BREAKPOINT_R finish");
		}
	}

	/* 写断点 */
	else if (objCurrentHardwareBpStruct.hwBpPairType == HW_BREAKPOINT_W)
	{
		if (!objCurrentHardwareBpStruct.isNextBp)
		{
			LOGI("正式断点CallBack HW_BREAKPOINT_W start");
			// 删除正式断点
			delProcessHwBp(&objCurrentHardwareBpStruct);
			// 初始化备胎断点.
			addHardwareBreakpoint((size_t)innerUc->uc_mcontext.pc + 0x4, HW_BREAKPOINT_LEN_4, HW_BREAKPOINT_X, 0,
								  (size_t)objCurrentHardwareBpStruct.hwBpAddr, objCurrentHardwareBpStruct.hwBpLen, objCurrentHardwareBpStruct.hwBpType, 0,
								  0, 1, HW_BREAKPOINT_W, 0);
			dumpStackUnwind();
			LOGI("正式断点CallBack  HW_BREAKPOINT_W finish");
		}
		//备胎断点
		else if (objCurrentHardwareBpStruct.isNextBp)
		{
			LOGI("备胎断点CallBack HW_BREAKPOINT_W start");
			// 删除自己断点.
			delProcessHwBp(&objCurrentHardwareBpStruct);
			if (objCurrentHardwareBpStruct.resetHwBpAddr != 0)
			{
				// 初始化原来的正式断点，但这里需要提前知道，他是给哪个断点当的备胎。
				addHardwareBreakpoint(objCurrentHardwareBpStruct.resetHwBpAddr, objCurrentHardwareBpStruct.resetHwBpLen, objCurrentHardwareBpStruct.resetHwBpType, objCurrentHardwareBpStruct.resetHwBpHandler,
									  0, 0, 0, 0,
									  0, 0, HW_BREAKPOINT_W, 0);
			}
			else
			{
				LOGI("Can't find ResetHwBp Addr to restore hwbp");
			}
			LOGI("备胎断点CallBack HW_BREAKPOINT_W finish");
		}
	}

	/* 硬件读写断点 */
	else if (objCurrentHardwareBpStruct.hwBpPairType == HW_BREAKPOINT_RW)
	{
		if (!objCurrentHardwareBpStruct.isNextBp)
		{
			LOGI("正式断点CallBack HW_BREAKPOINT_RW start");
			// 删除正式断点
			delProcessHwBp(&objCurrentHardwareBpStruct);
			// 初始化备胎断点.
			addHardwareBreakpoint((size_t)innerUc->uc_mcontext.pc + 0x4, HW_BREAKPOINT_LEN_4, HW_BREAKPOINT_X, 0,
								  (size_t)objCurrentHardwareBpStruct.hwBpAddr, objCurrentHardwareBpStruct.hwBpLen, objCurrentHardwareBpStruct.hwBpType, 0,
								  0, 1, HW_BREAKPOINT_RW, 0);
			LOGI("正式断点CallBack  HW_BREAKPOINT_RW finish");
		}
		//备胎断点
		else if (objCurrentHardwareBpStruct.isNextBp)
		{
			LOGI("备胎断点CallBack HW_BREAKPOINT_RW start");
			// 删除自己断点.
			delProcessHwBp(&objCurrentHardwareBpStruct);
			if (objCurrentHardwareBpStruct.resetHwBpAddr != 0)
			{
				addHardwareBreakpoint(objCurrentHardwareBpStruct.resetHwBpAddr, objCurrentHardwareBpStruct.resetHwBpLen, objCurrentHardwareBpStruct.resetHwBpType, objCurrentHardwareBpStruct.resetHwBpHandler,
									  0, 0, 0, 0,
									  0, 0, HW_BREAKPOINT_RW, 0);
			}
			else
			{
				LOGI("Can't find ResetHwBp Addr to restore hwbp");
			}
			LOGI("备胎断点CallBack HW_BREAKPOINT_RW finish");
		}
	}
	else
	{
		LOGI("Unrecognize hwBpPairType!");
	}
	return;
}

void registerSignal44HwBp()
{
	LOGI("register signal 44 hwbp");
	struct sigaction sig;
	sig.sa_sigaction = sig44HwbpHandler;
	sig.sa_flags = SA_SIGINFO;
	sigaction(SIG_HWBP, &sig, NULL);
	return;
}

bool addHardwareBreakpoint(size_t hwBpAddr, size_t hwBpLen, size_t hwBpType, size_t hwBpHandler,
						   size_t resetHwBpAddr, size_t resetHwBpLen, size_t resetHwBpType, size_t resetHwBpHandler,
						   size_t hProcess, size_t isNextBp, size_t hwBpPairType, size_t hwBpIsEmu)
{
	HardwareBpStruct objHardwareBpStruct = initHardwareBpStruct(
		(size_t)hwBpAddr,
		hwBpLen, hwBpType, hwBpHandler,
		resetHwBpAddr, resetHwBpLen, resetHwBpType, resetHwBpHandler,
		hProcess, isNextBp, hwBpPairType, hwBpIsEmu);
	size_t ret = addProcessHwBp(&objHardwareBpStruct);
	LOGI("addHardwareBreakpoint ret:%zx", ret);
	printHwBpMap();
	return true;
}

void initHardwareModule()
{
	LOGI("initHardwareModule");
	registerSignal44HwBp();
	connectDriver();
	LOGI("BPs:[%d] WPs:[%d]", getNumBRPS(), getNumWRPS());
	return;
}
