#pragma once
#include "util.hpp"

namespace n_smbios
{
	char smbois_vendor[100]{ 0 };
	char smbois_version[100]{ 0 };
	char smbois_date[100]{ 0 };
	char smbois_manufacturer[100]{ 0 };
	char smbois_product_name[100]{ 0 };
	char smbois_serial_number[100]{ 0 };

	typedef struct
	{
		UINT8   Type;
		UINT8   Length;
		UINT8   Handle[2];
	} SMBIOS_HEADER;

	typedef UINT8   SMBIOS_STRING;

	typedef struct
	{
		SMBIOS_HEADER   Hdr;
		SMBIOS_STRING   Vendor;
		SMBIOS_STRING   BiosVersion;
		UINT8           BiosSegment[2];
		SMBIOS_STRING   BiosReleaseDate;
		UINT8           BiosSize;
		UINT8           BiosCharacteristics[8];
	} SMBIOS_TYPE0;

	typedef struct
	{
		SMBIOS_HEADER   Hdr;
		SMBIOS_STRING   Manufacturer;
		SMBIOS_STRING   ProductName;
		SMBIOS_STRING   Version;
		SMBIOS_STRING   SerialNumber;

		//
		// always byte copy this data to prevent alignment faults!
		//
		GUID			Uuid; // EFI_GUID == GUID?

		UINT8           WakeUpType;
	} SMBIOS_TYPE1;

	typedef struct
	{
		SMBIOS_HEADER   Hdr;
		SMBIOS_STRING   Manufacturer;
		SMBIOS_STRING   ProductName;
		SMBIOS_STRING   Version;
		SMBIOS_STRING   SerialNumber;
	} SMBIOS_TYPE2;

	typedef struct
	{
		SMBIOS_HEADER   Hdr;
		SMBIOS_STRING   Manufacturer;
		UINT8           Type;
		SMBIOS_STRING   Version;
		SMBIOS_STRING   SerialNumber;
		SMBIOS_STRING   AssetTag;
		UINT8           BootupState;
		UINT8           PowerSupplyState;
		UINT8           ThermalState;
		UINT8           SecurityStatus;
		UINT8           OemDefined[4];
	} SMBIOS_TYPE3;

	char* get_smbios_string(SMBIOS_HEADER* header, SMBIOS_STRING str)
	{
		if (header == 0 || str == 0) return 0;

		const char* start = reinterpret_cast<char*>(header) + header->Length;
		if (*start == 0) return 0;

		while (--str) start += strlen(start) + 1;

		return const_cast<char*>(start);
	}

	void process_smbios_table(SMBIOS_HEADER* header)
	{
		if (header == 0 || header->Length == 0)
			return;

		if (header->Type == 0)
		{
			SMBIOS_TYPE0* type_0 = reinterpret_cast<SMBIOS_TYPE0*>(header);
			if (type_0)
			{
				char* vendor = get_smbios_string(header, type_0->Vendor);
				char* version = get_smbios_string(header, type_0->BiosVersion);
				char* date = get_smbios_string(header, type_0->BiosReleaseDate);

				if (vendor)
				{
					n_log::printf("old vendor : %s \n", vendor);
					RtlCopyMemory(vendor, smbois_vendor, strlen(vendor));
					//n_log::printf("new vendor : %s \n", n_util::random_string(vendor, 0));
				}

				if (version)
				{
					n_log::printf("old version : %s \n", version);
					RtlCopyMemory(version, smbois_version, strlen(version));
					//n_log::printf("new version : %s \n", n_util::random_string(version, 0));
				}

				if (date)
				{
					n_log::printf("old date : %s \n", date);
					RtlCopyMemory(date, smbois_date, strlen(date));
					//n_log::printf("new date : %s \n", n_util::random_string(date, 0));
				}
			}
		}

		else if (header->Type == 1)
		{
			SMBIOS_TYPE1* type_1 = reinterpret_cast<SMBIOS_TYPE1*>(header);
			if (type_1)
			{
				char* manu_facturer = get_smbios_string(header, type_1->Manufacturer);
				char* product_name = get_smbios_string(header, type_1->ProductName);
				char* serial_number = get_smbios_string(header, type_1->SerialNumber);
				char* version = get_smbios_string(header, type_1->Version);

				if (manu_facturer)
				{
					n_log::printf("old manufacturer : %s \n", manu_facturer);
					RtlCopyMemory(manu_facturer, smbois_manufacturer, strlen(manu_facturer));
					//n_log::printf("new manufacturer : %s \n", n_util::random_string(manu_facturer, 0));
				}

				if (product_name)
				{
					n_log::printf("old product name : %s \n", product_name);
					RtlCopyMemory(product_name, smbois_product_name, strlen(product_name));
					//n_log::printf("new product name : %s \n", n_util::random_string(product_name, 0));
				}

				if (serial_number)
				{
					n_log::printf("old serial number : %s \n", serial_number);
					RtlCopyMemory(serial_number, smbois_serial_number, strlen(serial_number));
					//n_log::printf("new serial number : %s \n", n_util::random_string(serial_number, 0));
				}

				if (version)
				{
					n_log::printf("old version : %s \n", version);
					RtlCopyMemory(version, smbois_version, strlen(version));
					//n_log::printf("new version : %s \n", n_util::random_string(version, 0));
				}
			}
		}

		else if (header->Type == 2)
		{
			SMBIOS_TYPE2* type_2 = reinterpret_cast<SMBIOS_TYPE2*>(header);
			if (type_2)
			{
				char* manu_facturer = get_smbios_string(header, type_2->Manufacturer);
				char* product_name = get_smbios_string(header, type_2->ProductName);
				char* serial_number = get_smbios_string(header, type_2->SerialNumber);
				char* version = get_smbios_string(header, type_2->Version);

				if (manu_facturer)
				{
					n_log::printf("old manufacturer : %s \n", manu_facturer);
					RtlCopyMemory(manu_facturer, smbois_manufacturer, strlen(manu_facturer));
					//n_log::printf("new manufacturer : %s \n", n_util::random_string(manu_facturer, 0));
				}

				if (product_name)
				{
					n_log::printf("old product name : %s \n", product_name);
					RtlCopyMemory(product_name, smbois_product_name, strlen(product_name));
					//n_log::printf("new product name : %s \n", n_util::random_string(product_name, 0));
				}

				if (serial_number)
				{
					n_log::printf("old serial number : %s \n", serial_number);
					RtlCopyMemory(serial_number, smbois_serial_number, strlen(serial_number));
					//n_log::printf("new serial number : %s \n", n_util::random_string(serial_number, 0));
				}

				if (version)
				{
					n_log::printf("old version : %s \n", version);
					RtlCopyMemory(version, smbois_version, strlen(version));
					//n_log::printf("new version : %s \n", n_util::random_string(version, 0));
				}
			}
		}

		else if (header->Type == 3)
		{
			SMBIOS_TYPE3* type_3 = reinterpret_cast<SMBIOS_TYPE3*>(header);
			if (type_3)
			{
				char* manu_facturer = get_smbios_string(header, type_3->Manufacturer);
				char* version = get_smbios_string(header, type_3->Version);
				char* serial_number = get_smbios_string(header, type_3->SerialNumber);

				if (manu_facturer)
				{
					n_log::printf("old manufacturer : %s \n", manu_facturer);
					RtlCopyMemory(manu_facturer, smbois_manufacturer, strlen(manu_facturer));
					//n_log::printf("new manufacturer : %s \n", n_util::random_string(manu_facturer, 0));
				}

				if (serial_number)
				{
					n_log::printf("old serial number : %s \n", serial_number);
					RtlCopyMemory(serial_number, smbois_serial_number, strlen(serial_number));
					//n_log::printf("new serial number : %s \n", n_util::random_string(serial_number, 0));
				}

				if (version)
				{
					n_log::printf("old version : %s \n", version);
					RtlCopyMemory(version, smbois_version, strlen(version));
					//n_log::printf("new version : %s \n", n_util::random_string(version, 0));
				}
			}
		}
	}

	void handle_smbios_table(void* mapped, unsigned long length)
	{
		char* end_address = static_cast<char*>(mapped) + length;

		while (true)
		{
			SMBIOS_HEADER* header = static_cast<SMBIOS_HEADER*>(mapped);
			if (header->Type == 127 && header->Length == 4)
				break;

			process_smbios_table(header);

			char* ptr = static_cast<char*>(mapped) + header->Length;
			while (0 != (*ptr | *(ptr + 1))) ptr++;
			ptr += 2;
			if (ptr >= end_address)
				break;

			mapped = ptr;
		}
	}

	bool spoofer_smbios()
	{
		DWORD64 address = 0;
		DWORD32 size = 0;
		if (n_util::get_module_base_address("ntoskrnl.exe", address, size) == false) return false;
		n_log::printf("ntoskrnl address : %llx \t size : %x \n", address, size);

		// WmipFindSMBiosStructure -> WmipSMBiosTablePhysicalAddress
		PPHYSICAL_ADDRESS physical_address = (PPHYSICAL_ADDRESS)n_util::find_pattern_image(address,
			"\x48\x8B\x0D\x00\x00\x00\x00\x48\x85\xC9\x74\x00\x8B\x15",
			"xxx????xxxx?xx");
		if (physical_address == 0) return false;

		physical_address = reinterpret_cast<PPHYSICAL_ADDRESS>(reinterpret_cast<char*>(physical_address) + 7 + *reinterpret_cast<int*>(reinterpret_cast<char*>(physical_address) + 3));
		if (physical_address == 0) return false;
		n_log::printf("physical address : %llx \n", physical_address);

		// WmipFindSMBiosStructure -> WmipSMBiosTableLength
		DWORD64 physical_length_address = n_util::find_pattern_image(address,
			"\x8B\x1D\x00\x00\x00\x00\x48\x8B\xD0\x44\x8B\xC3\x48\x8B\xCD\xE8\x00\x00\x00\x00\x8B\xD3\x48\x8B",
			"xx????xxxxxxxxxx????xxxx");
		if (physical_length_address == 0) return false;

		unsigned long physical_length = *reinterpret_cast<unsigned long*>(static_cast<char*>((void*)physical_length_address) + 6 + *reinterpret_cast<int*>(static_cast<char*>((void*)physical_length_address) + 2));
		if (physical_length == 0) return false;
		n_log::printf("physical length : %d \n", physical_length);

		void* mapped = MmMapIoSpace(*physical_address, physical_length, MmNonCached);
		if (mapped == 0) return false;

		handle_smbios_table(mapped, physical_length);

		MmUnmapIoSpace(mapped, physical_length);

		return true;
	}
}