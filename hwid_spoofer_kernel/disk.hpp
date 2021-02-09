#pragma once
#include "util.hpp"
#include "log.hpp"
#include <ntstrsafe.h>
#include <ntdddisk.h>
#include <ntddscsi.h>
#include <ata.h>
#include <ntintsafe.h>
#include <mountmgr.h>
#include <mountdev.h>

namespace n_disk
{
	int disk_mode = 1;
	char disk_serial_buffer[100]{ 0 };
	char disk_product_buffer[100]{ 0 };
	char disk_product_revision_buffer[100]{ 0 };

	bool disk_guid_random = false;
	bool disk_volumn_clean = false;
	bool disk_smart_disable = false;

	typedef NTSTATUS(__fastcall* DiskEnableDisableFailurePrediction)(void* a1, bool a2);

	typedef struct _STOR_SCSI_IDENTITY
	{
		char Space[0x8]; // +0x008 SerialNumber     : _STRING
		STRING SerialNumber;
	} STOR_SCSI_IDENTITY, *PSTOR_SCSI_IDENTITY;

	typedef struct _TELEMETRY_UNIT_EXTENSION
	{
		/*
			+0x000 Flags            : <anonymous-tag>
			+0x000 DeviceHealthEventsLogged : Pos 0, 1 Bit
			+0x000 FailedFirstSMARTCommand : Pos 1, 1 Bit
			+0x000 FailedFirstDeviceStatisticsLogCommand : Pos 2, 1 Bit
			+0x000 FailedFirstNvmeCloudSSDCommand : Pos 3, 1 Bit
			+0x000 SmartPredictFailure : Pos 4, 1 Bit
			+0x000 Reserved         : Pos 5, 27 Bits
		 */
		int SmartMask;
	} TELEMETRY_UNIT_EXTENSION, *PTELEMETRY_UNIT_EXTENSION;

	// lkd> dt storport!_RAID_UNIT_EXTENSION -b
	typedef struct _RAID_UNIT_EXTENSION
	{
		union
		{
			struct
			{
				char Space[0x68]; // +0x068 Identity         : _STOR_SCSI_IDENTITY
				STOR_SCSI_IDENTITY Identity;
			} _Identity;

			struct
			{
				char Space[0x7c8]; // +0x7c8 TelemetryExtension : _TELEMETRY_UNIT_EXTENSION
				TELEMETRY_UNIT_EXTENSION Telemetry;
			} _Smart;
		};
	} RAID_UNIT_EXTENSION, *PRAID_UNIT_EXTENSION;

	typedef struct _IDSECTOR {
		USHORT  wGenConfig;
		USHORT  wNumCyls;
		USHORT  wReserved;
		USHORT  wNumHeads;
		USHORT  wBytesPerTrack;
		USHORT  wBytesPerSector;
		USHORT  wSectorsPerTrack;
		USHORT  wVendorUnique[3];
		CHAR    sSerialNumber[20];
		USHORT  wBufferType;
		USHORT  wBufferSize;
		USHORT  wECCSize;
		CHAR    sFirmwareRev[8];
		CHAR    sModelNumber[40];
		USHORT  wMoreVendorUnique;
		USHORT  wDoubleWordIO;
		USHORT  wCapabilities;
		USHORT  wReserved1;
		USHORT  wPIOTiming;
		USHORT  wDMATiming;
		USHORT  wBS;
		USHORT  wNumCurrentCyls;
		USHORT  wNumCurrentHeads;
		USHORT  wNumCurrentSectorsPerTrack;
		ULONG   ulCurrentSectorCapacity;
		USHORT  wMultSectorStuff;
		ULONG   ulTotalAddressableSectors;
		USHORT  wSingleWordDMA;
		USHORT  wMultiWordDMA;
		BYTE    bThisReserved[128];
	} IDSECTOR, *PIDSECTOR;

	typedef __int64(__fastcall* RaidUnitRegisterInterfaces)(PRAID_UNIT_EXTENSION a1);

	PDRIVER_DISPATCH g_original_partmgr_control = 0;

	PDRIVER_DISPATCH g_original_disk_control = 0;

	PDRIVER_DISPATCH g_original_mountmgr_control = 0;

	NTSTATUS my_part_info_ioc(PDEVICE_OBJECT device, PIRP irp, PVOID context)
	{
		if (context)
		{
			n_util::IOC_REQUEST request = *(n_util::PIOC_REQUEST)context;
			ExFreePool(context);

			if (request.BufferLength >= sizeof(PARTITION_INFORMATION_EX))
			{
				PPARTITION_INFORMATION_EX info = (PPARTITION_INFORMATION_EX)request.Buffer;
				if (info->PartitionStyle == PARTITION_STYLE_GPT && disk_guid_random)
					n_util::random_string((char*)&info->Gpt.PartitionId, sizeof(GUID));
			}

			if (request.OldRoutine && irp->StackCount > 1)
				return request.OldRoutine(device, irp, request.OldContext);
		}

		return STATUS_SUCCESS;
	}

	NTSTATUS my_part_layout_ioc(PDEVICE_OBJECT device, PIRP irp, PVOID context)
	{
		if (context)
		{
			n_util::IOC_REQUEST request = *(n_util::PIOC_REQUEST)context;
			ExFreePool(context);

			if (request.BufferLength >= sizeof(DRIVE_LAYOUT_INFORMATION_EX))
			{
				PDRIVE_LAYOUT_INFORMATION_EX info = (PDRIVE_LAYOUT_INFORMATION_EX)request.Buffer;
				if (PARTITION_STYLE_GPT == info->PartitionStyle && disk_guid_random)
					n_util::random_string((char*)&info->Gpt.DiskId, sizeof(GUID));
			}

			if (request.OldRoutine && irp->StackCount > 1)
				return request.OldRoutine(device, irp, request.OldContext);
		}

		return STATUS_SUCCESS;
	}

	// guid
	NTSTATUS my_partmgr_handle_control(PDEVICE_OBJECT device, PIRP irp)
	{
		PIO_STACK_LOCATION ioc = IoGetCurrentIrpStackLocation(irp);
		unsigned long msg = ioc->Parameters.DeviceIoControl.IoControlCode;

		if (msg == IOCTL_DISK_GET_PARTITION_INFO_EX)
			n_util::change_ioc(ioc, irp, my_part_info_ioc);

		else if (msg == IOCTL_DISK_GET_DRIVE_LAYOUT_EX)
			n_util::change_ioc(ioc, irp, my_part_layout_ioc);

		return g_original_partmgr_control(device, irp);
	}

	NTSTATUS my_storage_query_ioc(PDEVICE_OBJECT device, PIRP irp, PVOID context)
	{
		if (context)
		{
			n_util::IOC_REQUEST request = *(n_util::PIOC_REQUEST)context;
			ExFreePool(context);

			if (request.BufferLength >= sizeof(STORAGE_DEVICE_DESCRIPTOR))
			{
				PSTORAGE_DEVICE_DESCRIPTOR desc = (PSTORAGE_DEVICE_DESCRIPTOR)request.Buffer;

				ULONG serial_offset = desc->SerialNumberOffset;
				ULONG product_offset = desc->ProductIdOffset;
				ULONG product_revision_offset = desc->ProductRevisionOffset;

				if (serial_offset
					&& product_offset
					&& product_revision_offset
					&& serial_offset < request.BufferLength
					&& product_offset < request.BufferLength
					&& product_revision_offset < request.BufferLength)
				{
					char* serial = (char*)desc + serial_offset;
					char* product = (char*)desc + product_offset;
					char* product_revision = (char*)desc + product_revision_offset;

					if (serial && product && product_revision)
					{
						switch (disk_mode)
						{
						case 0:
							RtlCopyMemory(serial, disk_serial_buffer, strlen(serial));
							RtlCopyMemory(product, disk_product_buffer, strlen(product));
							RtlCopyMemory(product_revision, disk_product_revision_buffer, strlen(product_revision));
							break;
						case 1:
							n_util::random_string(serial, 0);
							n_util::random_string(product, 0);
							n_util::random_string(product_revision, 0);
							break;
						case 2:
							RtlZeroMemory(serial, strlen(serial));
							RtlZeroMemory(product, strlen(product));
							RtlZeroMemory(product_revision, strlen(product_revision));
							break;
						}
					}
				}
			}

			if (request.OldRoutine && irp->StackCount > 1)
				return request.OldRoutine(device, irp, request.OldContext);
		}

		return STATUS_SUCCESS;
	}

	NTSTATUS my_ata_pass_ioc(PDEVICE_OBJECT device, PIRP irp, PVOID context)
	{
		if (context)
		{
			n_util::IOC_REQUEST request = *(n_util::PIOC_REQUEST)context;
			ExFreePool(context);

			if (request.BufferLength >= sizeof(ATA_PASS_THROUGH_EX) + sizeof(PIDENTIFY_DEVICE_DATA))
			{
				PATA_PASS_THROUGH_EX pte = (PATA_PASS_THROUGH_EX)request.Buffer;
				ULONG offset = (ULONG)pte->DataBufferOffset;
				if (offset && offset < request.BufferLength)
				{
					PIDENTIFY_DEVICE_DATA identity = (PIDENTIFY_DEVICE_DATA)((char*)request.Buffer + offset);
					if (identity)
					{
						if (disk_smart_disable)
						{
							identity->CommandSetSupport.SmartCommands = 0;
							identity->CommandSetActive.SmartCommands = 0;
						}

						char* serial = (char*)identity->SerialNumber;
						char* product = (char*)identity->FirmwareRevision;
						char* product_revision = (char*)identity->ModelNumber;
						if (serial && product && product_revision)
						{
							switch (disk_mode)
							{
							case 0:
								RtlCopyMemory(serial, disk_serial_buffer, strlen(serial));
								RtlCopyMemory(product, disk_product_buffer, strlen(product));
								RtlCopyMemory(product_revision, disk_product_revision_buffer, strlen(product_revision));
								break;
							case 1:
								n_util::random_string(serial, 0);
								n_util::random_string(product, 0);
								n_util::random_string(product_revision, 0);
								break;
							case 2:
								RtlZeroMemory(serial, strlen(serial));
								RtlZeroMemory(product, strlen(product));
								RtlZeroMemory(product_revision, strlen(product_revision));
								break;
							}
						}
					}
				}
			}

			if (request.OldRoutine && irp->StackCount > 1)
				return request.OldRoutine(device, irp, request.OldContext);
		}

		return STATUS_SUCCESS;
	}

	NTSTATUS my_smart_data_ioc(PDEVICE_OBJECT device, PIRP irp, PVOID context)
	{
		if (context)
		{
			n_util::IOC_REQUEST request = *(n_util::PIOC_REQUEST)context;
			ExFreePool(context);

			if (request.BufferLength >= sizeof(SENDCMDOUTPARAMS))
			{
				char* serial = ((PIDSECTOR)((PSENDCMDOUTPARAMS)request.Buffer)->bBuffer)->sSerialNumber;
				char* product = ((PIDSECTOR)((PSENDCMDOUTPARAMS)request.Buffer)->bBuffer)->sFirmwareRev;
				char* product_revision = ((PIDSECTOR)((PSENDCMDOUTPARAMS)request.Buffer)->bBuffer)->sModelNumber;

				if (serial && product && product_revision)
				{
					switch (disk_mode)
					{
					case 0:
						RtlCopyMemory(serial, disk_serial_buffer, strlen(serial));
						RtlCopyMemory(product, disk_product_buffer, strlen(product));
						RtlCopyMemory(product_revision, disk_product_revision_buffer, strlen(product_revision));
						break;
					case 1:
						n_util::random_string(serial, 0);
						n_util::random_string(product, 0);
						n_util::random_string(product_revision, 0);
						break;
					case 2:
						RtlZeroMemory(serial, strlen(serial));
						RtlZeroMemory(product, strlen(product));
						RtlZeroMemory(product_revision, strlen(product_revision));
						break;
					}
				}
			}

			if (request.OldRoutine && irp->StackCount > 1)
				return request.OldRoutine(device, irp, request.OldContext);
		}

		return STATUS_SUCCESS;
	}

	// serial
	NTSTATUS my_disk_handle_control(PDEVICE_OBJECT device, PIRP irp)
	{
		PIO_STACK_LOCATION ioc = IoGetCurrentIrpStackLocation(irp);
		const unsigned long code = ioc->Parameters.DeviceIoControl.IoControlCode;

		if (code == IOCTL_STORAGE_QUERY_PROPERTY)
		{
			if (StorageDeviceProperty == ((PSTORAGE_PROPERTY_QUERY)irp->AssociatedIrp.SystemBuffer)->PropertyId)
				n_util::change_ioc(ioc, irp, my_storage_query_ioc);
		}

		else if (code == IOCTL_ATA_PASS_THROUGH)
			n_util::change_ioc(ioc, irp, my_ata_pass_ioc);

		else if (code == SMART_RCV_DRIVE_DATA)
			n_util::change_ioc(ioc, irp, my_smart_data_ioc);

		return g_original_disk_control(device, irp);
	}

	NTSTATUS my_mount_points_ioc(PDEVICE_OBJECT device, PIRP irp, PVOID context)
	{
		if (context)
		{
			n_util::IOC_REQUEST request = *(n_util::PIOC_REQUEST)context;
			ExFreePool(context);

			if (request.BufferLength >= sizeof(MOUNTMGR_MOUNT_POINTS) && disk_volumn_clean)
			{
				PMOUNTMGR_MOUNT_POINTS points = (PMOUNTMGR_MOUNT_POINTS)request.Buffer;
				for (DWORD i = 0; i < points->NumberOfMountPoints; ++i)
				{
					PMOUNTMGR_MOUNT_POINT point = &points->MountPoints[i];

					if (point->UniqueIdOffset)
						point->UniqueIdLength = 0;

					if (point->SymbolicLinkNameOffset)
						point->SymbolicLinkNameLength = 0;
				}
			}

			if (request.OldRoutine && irp->StackCount > 1)
				return request.OldRoutine(device, irp, request.OldContext);
		}

		return STATUS_SUCCESS;
	}

	NTSTATUS my_mount_unique_ioc(PDEVICE_OBJECT device, PIRP irp, PVOID context)
	{
		if (context)
		{
			n_util::IOC_REQUEST request = *(n_util::PIOC_REQUEST)context;
			ExFreePool(context);

			if (request.BufferLength >= sizeof(MOUNTDEV_UNIQUE_ID) && disk_volumn_clean)
				((PMOUNTDEV_UNIQUE_ID)request.Buffer)->UniqueIdLength = 0;

			if (request.OldRoutine && irp->StackCount > 1)
				return request.OldRoutine(device, irp, request.OldContext);
		}

		return STATUS_SUCCESS;
	}

	// column
	NTSTATUS my_mountmgr_handle_control(PDEVICE_OBJECT device, PIRP irp)
	{
		PIO_STACK_LOCATION ioc = IoGetCurrentIrpStackLocation(irp);
		const unsigned long code = ioc->Parameters.DeviceIoControl.IoControlCode;

		if (code == IOCTL_MOUNTMGR_QUERY_POINTS)
			n_util::change_ioc(ioc, irp, my_mount_points_ioc);

		else if (code == IOCTL_MOUNTDEV_QUERY_UNIQUE_ID)
			n_util::change_ioc(ioc, irp, my_mount_unique_ioc);

		return g_original_mountmgr_control(device, irp);
	}

	bool disable_smart()
	{
		DWORD64 address = 0;
		DWORD32 size = 0;
		if (n_util::get_module_base_address("disk.sys", address, size) == false) return false;
		n_log::printf("disk address : %llx \t size : %x \n", address, size);

		DiskEnableDisableFailurePrediction func = (DiskEnableDisableFailurePrediction)n_util::find_pattern_image(address,
			"\x48\x89\x5c\x24\x00\x48\x89\x74\x24\x00\x57\x48\x81\xec\x00\x00\x00\x00\x48\x8b\x05\x00\x00\x00\x00\x48\x33\xc4\x48\x89\x84\x24\x00\x00\x00\x00\x48\x8b\x59\x60\x48\x8b\xf1\x40\x8a\xfa\x8b\x4b\x10",
			"xxxx?xxxx?xxxx????xxx????xxxxxxx????xxxxxxxxxxxxx"); //DiskEnableDisableFailurePrediction
		if (func == 0) return false;
		n_log::printf("DiskEnableDisableFailurePrediction address : %llx \n", func);

		UNICODE_STRING driver_disk;
		RtlInitUnicodeString(&driver_disk, L"\\Driver\\Disk");

		PDRIVER_OBJECT driver_object = nullptr;
		NTSTATUS status = ObReferenceObjectByName(&driver_disk, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, nullptr, 0, *IoDriverObjectType, KernelMode, nullptr, reinterpret_cast<PVOID*>(&driver_object));
		if (!NT_SUCCESS(status)) return false;
		n_log::printf("disk object address : %llx \n", driver_object);

		PDEVICE_OBJECT device_object_list[100]{ 0 };
		RtlZeroMemory(device_object_list, sizeof(device_object_list));

		ULONG number_of_device_objects = 0;
		status = IoEnumerateDeviceObjectList(driver_object, device_object_list, sizeof(device_object_list), &number_of_device_objects);
		if (!NT_SUCCESS(status))
		{
			ObDereferenceObject(driver_object);
			return false;
		}
		n_log::printf("number of device objects is : %d \n", number_of_device_objects);

		for (ULONG i = 0; i < number_of_device_objects; ++i)
		{
			PDEVICE_OBJECT device_object = device_object_list[i];
			status = func(device_object->DeviceExtension, false);
			if (NT_SUCCESS(status)) n_log::printf("DiskEnableDisableFailurePrediction success \n");

			ObDereferenceObject(device_object);
		}

		ObDereferenceObject(driver_object);

		return true;
	}

	bool handle_disk_serials(PDEVICE_OBJECT device_object, RaidUnitRegisterInterfaces func)
	{
		if (device_object == 0 || func == 0) return false;

		while (device_object->NextDevice)
		{
			do
			{
				if (device_object->DeviceType == FILE_DEVICE_DISK)
				{
					PRAID_UNIT_EXTENSION extension = reinterpret_cast<PRAID_UNIT_EXTENSION>(device_object->DeviceExtension);
					if (extension == 0)
					{
						n_log::printf("DeviceExtension buffer is null \n");
						break;
					}

					unsigned short length = extension->_Identity.Identity.SerialNumber.Length;
					if (length == 0)
					{
						n_log::printf("serial_number length is null \n");
						break;
					}

					n_log::printf("old disk serial number : %s \n", extension->_Identity.Identity.SerialNumber.Buffer);
					RtlCopyMemory(extension->_Identity.Identity.SerialNumber.Buffer, disk_serial_buffer, length);
					//n_util::random_string(extension->_Identity.Identity.SerialNumber.Buffer, length);
					n_log::printf("new disk serial number : %s \n", extension->_Identity.Identity.SerialNumber.Buffer);

					extension->_Smart.Telemetry.SmartMask = 0;
					func(extension);
				}
			} while (false);

			device_object = device_object->NextDevice;
		}

		return true;
	}

	bool change_disk_serials()
	{
		DWORD64 address = 0;
		DWORD32 size = 0;
		if (n_util::get_module_base_address("storport.sys", address, size) == false) return false;
		n_log::printf("storport address : %llx \t size : %x \n", address, size);

		RaidUnitRegisterInterfaces func = (RaidUnitRegisterInterfaces)n_util::find_pattern_image(address,
			"\x48\x89\x5C\x24\x00\x55\x56\x57\x48\x83\xEC\x50",
			"xxxx?xxxxxxx");// RaidUnitRegisterInterfaces
		if (func == 0) return false;
		n_log::printf("RaidUnitRegisterInterfaces address : %llx \n", func);

		for (int i = 0; i < 5; i++)
		{
			const wchar_t* format = L"\\Device\\RaidPort%d";
			wchar_t buffer[18]{ 0 };
			RtlStringCbPrintfW(buffer, 18 * sizeof(wchar_t), format, i);

			UNICODE_STRING raid_port;
			RtlInitUnicodeString(&raid_port, buffer);

			PFILE_OBJECT file_object = 0;
			PDEVICE_OBJECT device_object = 0;
			NTSTATUS status = IoGetDeviceObjectPointer(&raid_port, FILE_READ_DATA, &file_object, &device_object);
			if (NT_SUCCESS(status))
			{
				handle_disk_serials(device_object->DriverObject->DeviceObject, func);

				ObDereferenceObject(file_object);
			}
		}

		return true;
	}

	bool fuck_dispatch()
	{
		UNICODE_STRING disk;
		RtlInitUnicodeString(&disk, L"\\Driver\\Disk");

		PDRIVER_OBJECT driver_object;
		auto status = ObReferenceObjectByName(&disk, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, nullptr, 0, *IoDriverObjectType, KernelMode, nullptr, reinterpret_cast<PVOID*>(&driver_object));
		if (!NT_SUCCESS(status)) return false;

		// ю╤фа
		driver_object->MajorFunction[IRP_MJ_DEVICE_CONTROL] = driver_object->MajorFunction[IRP_MJ_QUERY_INFORMATION];

		ObDereferenceObject(driver_object);
		return true;
	}

	bool start_hook()
	{
		g_original_partmgr_control = n_util::add_irp_hook(L"\\Driver\\partmgr", my_partmgr_handle_control);
		g_original_disk_control = n_util::add_irp_hook(L"\\Driver\\disk", my_disk_handle_control);
		g_original_mountmgr_control = n_util::add_irp_hook(L"\\Driver\\mountmgr", my_mountmgr_handle_control);

		return g_original_partmgr_control && g_original_disk_control && g_original_mountmgr_control;
	}

	bool clean_hook()
	{
		bool state = true;
		state = state && n_util::del_irp_hook(L"\\Driver\\partmgr", g_original_partmgr_control);
		state = state && n_util::del_irp_hook(L"\\Driver\\disk", g_original_disk_control);
		state = state && n_util::del_irp_hook(L"\\Driver\\mountmgr", g_original_mountmgr_control);

		return state;
	}
}