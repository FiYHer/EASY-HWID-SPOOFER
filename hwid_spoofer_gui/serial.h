#pragma once
#pragma warning(disable: 4996 4311 4302)

#include <Windows.h>

class DiskSectorRW
{
public:
	virtual bool Open(char *vol) = 0;
	virtual void Close() = 0;
	virtual bool ReadSector(DWORD sector, char *Buffer, int sectorSize = 512) = 0;
	virtual bool WriteSector(DWORD sector, char *buffer, int sectorSize = 512) = 0;
};

class DiskSectorWinNT : public DiskSectorRW
{
private:
	HANDLE m_hDisk;
public:
	bool Open(char *vol);
	void Close();
	bool ReadSector(DWORD sector, char *Buffer, int sectorSize = 512);
	bool WriteSector(DWORD sector, char *Buffer, int sectorSize = 512);
};

class DiskSectorWin9x : public DiskSectorRW
{
private:
	HANDLE m_hVmm32;
	bool m_bOpened;
	char m_chDrive;
	BYTE m_nDriveNo;
	bool m_bW9xOsr2AndAbove;
	bool m_bUseLocking;
public:

	DiskSectorWin9x() : m_bUseLocking(false) { }
	bool Open(char *vol);
	void Close();

	bool ReadSector(DWORD sector, char *Buffer, int sectorSize = 512);
	bool WriteSector(DWORD sector, char *Buffer, int sectorSize = 512);

	static bool LockLogicalVolume(HANDLE hVWin32, BYTE   bDriveNum, BYTE   bLockLevel, WORD wPermissions);
	static bool UnlockLogicalVolume(HANDLE hVWin32, BYTE bDriveNum);

	static bool ReadLogicalSectors(HANDLE hDev, BYTE   bDrive, DWORD  dwStartSector, WORD wSectors, LPBYTE lpSectBuff);
	static bool WriteLogicalSectors(HANDLE hDev, BYTE   bDrive, DWORD  dwStartSector, WORD   wSectors, LPBYTE lpSectBuff);

	static bool NewReadSectors(HANDLE hDev, BYTE   bDrive, DWORD  dwStartSector, WORD   wSectors, LPBYTE lpSectBuff);
	static bool NewWriteSectors(HANDLE hDev, BYTE   bDrive, DWORD  dwStartSector, WORD   wSectors, LPBYTE lpSectBuff);
};

class DiskSector
{
private:
	DiskSectorRW *util;
public:
	DiskSector();
	~DiskSector();
	bool Open(char *vol);
	void Close();
	bool ReadSector(DWORD sector, char *Buffer, int sectorSize = 512);
	bool WriteSector(DWORD sector, char *buffer, int sectorSize = 512);
};
