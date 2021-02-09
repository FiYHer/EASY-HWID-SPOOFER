#include "disk.h"

bool change_serial_number(char drive, DWORD serial)
{
	const size_t max_pbsi = 3;

	struct partial_boot_sector_info
	{
		CONST CHAR* Fs; // file system name
		DWORD FsOffs; // offset of file system name in the boot sector
		DWORD SerialOffs; // offset of the serial number in the boot sector
	};

	static const partial_boot_sector_info pbsi[max_pbsi] =
	{
	 {"FAT32", 0x52, 0x43},
	 {"FAT",   0x36, 0x27},
	 {"NTFS",  0x03, 0x48}
	};

	TCHAR szDrive[12];
	wsprintfA(szDrive, "%c:\\", drive);

	DiskSector disk;
	if (!disk.Open(szDrive)) return false;

	// read sector
	char Sector[512];
	if (!disk.ReadSector(0, Sector)) return false;

	// Try to search for a valid boot sector
	DWORD i;
	for (i = 0; i < max_pbsi; i++)
	{
		if (strncmp(pbsi[i].Fs, Sector + pbsi[i].FsOffs, strlen(pbsi[i].Fs)) == 0)
		{
			// We found a valid signature
			break;
		}
	}

	if (i >= max_pbsi) return false;

	// Patch the serial number
	*(PDWORD)(Sector + pbsi[i].SerialOffs) = serial;

	// write boot sector
	if (!disk.WriteSector(0, Sector)) return false;

	return true;
}

std::map<char, unsigned long> get_disk_all_drive_serial()
{
	std::map<char, unsigned long> result;

	for (char c = 'a'; c <= 'z'; c++)
	{
		char buffer[50]{ 0 };
		wsprintfA(buffer, "%c:\\", c);

		unsigned long serial, component, flags;

		const int size = 200;
		char volumn[size]{ 0 };
		char systems[size]{ 0 };
		if (GetVolumeInformationA(buffer, volumn, size, &serial, &component, &flags, systems, size))
		{
			result[c] = serial;
		}
	}

	return result;
}