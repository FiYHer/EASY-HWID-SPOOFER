#pragma once
#include "util.hpp"
#include "log.hpp"
#include <ifdef.h>
#include <minwindef.h>
#include <ntddndis.h>

namespace n_nic
{
	char permanent_mac[100]{ 0 };
	char current_mac[100]{ 0 };

	bool arp_table_handle = false;
	int mac_mode = 0;

	// dt ndis!_NDIS_IF_BLOCK
	typedef struct _NDIS_IF_BLOCK {
		char _padding_0[0x464];
		IF_PHYSICAL_ADDRESS_LH ifPhysAddress; // 0x464
		IF_PHYSICAL_ADDRESS_LH PermanentPhysAddress; // 0x486
	} NDIS_IF_BLOCK, *PNDIS_IF_BLOCK;

	typedef struct _KSTRING {
		char _padding_0[0x10];
		WCHAR Buffer[1]; // 0x10 at least
	} KSTRING, *PKSTRING;

	// dt ndis!_NDIS_FILTER_BLOCK
	typedef struct _NDIS_FILTER_BLOCK {
		char _padding_0[0x8];
		struct _NDIS_FILTER_BLOCK *NextFilter; // 0x8
		char _padding_1[0x18];
		PKSTRING FilterInstanceName; // 0x28
	} NDIS_FILTER_BLOCK, *PNDIS_FILTER_BLOCK;

	typedef struct _NSI_PARAMS
	{
		__int64 field_0;
		__int64 field_8;
		__int64 field_10;
		int Type;
		int field_1C;
		int field_20;
		int field_24;
		char field_42;
		__int64 AddrTable;
		int AddrEntrySize;
		int field_34;
		__int64 NeighborTable;
		int NeighborTableEntrySize;
		int field_44;
		__int64 StateTable;
		int StateTableEntrySize;
		int field_54;
		__int64 OwnerTable;
		int OwnerTableEntrySize;
		int field_64;
		int Count;
		int field_6C;
	}NSI_PARAMS, *PNSI_PARAMS;

	typedef struct _NIC_ARRAY
	{
		PDRIVER_OBJECT driver_object;
		PDRIVER_DISPATCH original_function;
	}NIC_ARRAY, *PNIC_ARRAY;

	const int max_array_size = 20;
	int array_size = 0;
	NIC_ARRAY g_nic_array[max_array_size] = { 0 };
	KSPIN_LOCK g_lock;

	PDRIVER_DISPATCH g_original_arp_control = 0;

	wchar_t* paste_guid(wchar_t* str, size_t len)
	{
		if (str == 0) return 0;
		if (len == 0) len = wcslen(str);

		size_t index = 0;
		for (size_t i = 0; i < len; i++)
		{
			if (str[i] == L'{') index = i;
			else if (str[i] == L'}')
			{
				str[i + 1] = 0;
				break;
			}
		}

		return str + index;
	}

	NTSTATUS my_arp_handle_control(PDEVICE_OBJECT device, PIRP irp)
	{
		PIO_STACK_LOCATION ioc = IoGetCurrentIrpStackLocation(irp);

#define IOCTL_NSI_PROXY_ARP (0x0012001B)
#define IOCTL_ARP_TABLE (0x12000F)
#define NSI_PARAMS_ARP (11)

		switch (ioc->Parameters.DeviceIoControl.IoControlCode)
		{
		case IOCTL_NSI_PROXY_ARP:
		case IOCTL_ARP_TABLE:
		{
			NTSTATUS status = g_original_arp_control(device, irp);

			if (NT_SUCCESS(status))
				RtlZeroMemory(irp->UserBuffer, ioc->Parameters.DeviceIoControl.OutputBufferLength);

			return status;
		}
		}

		return g_original_arp_control(device, irp);
	}

	NTSTATUS my_nic_ioc_handle_PERMANENT(PDEVICE_OBJECT device, PIRP irp, PVOID context)
	{
		if (context)
		{
			n_util::IOC_REQUEST request = *(n_util::PIOC_REQUEST)context;
			ExFreePool(context);

			if (irp->MdlAddress)
			{
				switch (mac_mode)
				{
				case 0:
					n_util::random_string((char*)MmGetSystemAddressForMdl(irp->MdlAddress), 6);
					break;
				case 1:
					RtlCopyMemory((char*)MmGetSystemAddressForMdl(irp->MdlAddress), permanent_mac, 6);
					break;
				}
			}

			if (request.OldRoutine && irp->StackCount > 1)
				return request.OldRoutine(device, irp, request.OldContext);
		}

		return STATUS_SUCCESS;
	}

	NTSTATUS my_nic_ioc_handle_CURRENT(PDEVICE_OBJECT device, PIRP irp, PVOID context)
	{
		if (context)
		{
			n_util::IOC_REQUEST request = *(n_util::PIOC_REQUEST)context;
			ExFreePool(context);

			if (irp->MdlAddress)
			{
				switch (mac_mode)
				{
				case 0:
					n_util::random_string((char*)MmGetSystemAddressForMdl(irp->MdlAddress), 6);
					break;
				case 1:
					RtlCopyMemory((char*)MmGetSystemAddressForMdl(irp->MdlAddress), current_mac, 6);
					break;
				}
			}

			if (request.OldRoutine && irp->StackCount > 1)
				return request.OldRoutine(device, irp, request.OldContext);
		}

		return STATUS_SUCCESS;
	}

	NTSTATUS my_mac_handle_control(PDEVICE_OBJECT device, PIRP irp)
	{
		KIRQL irql;
		KeAcquireSpinLock(&g_lock, &irql);

		for (int i = 0; i < array_size; i++)
		{
			NIC_ARRAY& item = g_nic_array[i];
			if (item.driver_object != device->DriverObject) continue;

			PIO_STACK_LOCATION ioc = IoGetCurrentIrpStackLocation(irp);
			unsigned long code = ioc->Parameters.DeviceIoControl.IoControlCode;

			if (code == IOCTL_NDIS_QUERY_GLOBAL_STATS)
			{
				DWORD type = *(PDWORD)irp->AssociatedIrp.SystemBuffer;
				if (type == OID_802_3_PERMANENT_ADDRESS
					|| type == OID_802_5_PERMANENT_ADDRESS)
					n_util::change_ioc(ioc, irp, my_nic_ioc_handle_PERMANENT);
				if (type == OID_802_3_CURRENT_ADDRESS
					|| type == OID_802_5_CURRENT_ADDRESS)
					n_util::change_ioc(ioc, irp, my_nic_ioc_handle_CURRENT);
			}

			KeReleaseSpinLock(&g_lock, irql);

			return item.original_function(device, irp);
		}

		KeReleaseSpinLock(&g_lock, irql);

		irp->IoStatus.Status = STATUS_SUCCESS;
		irp->IoStatus.Information = 0;

		IoCompleteRequest(irp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
	}

	bool spoofer_nic()
	{
		KeInitializeSpinLock(&g_lock);

		DWORD64 address = 0;
		DWORD32 size = 0;
		if (n_util::get_module_base_address("ndis.sys", address, size) == false) return false;
		n_log::printf("ndis address : %llx \t size : %x \n", address, size);

		PNDIS_FILTER_BLOCK ndis_global_filter_list = (PNDIS_FILTER_BLOCK)n_util::find_pattern_image(address,
			"\x40\x8A\xF0\x48\x8B\x05",
			"xxxxxx");
		if (ndis_global_filter_list == 0) return false;
		n_log::printf("ndis global filter list address : %llx \n", ndis_global_filter_list);

		DWORD64 ndis_filter_block = n_util::find_pattern_image(address,
			"\x48\x85\x00\x0F\x84\x00\x00\x00\x00\x00\x8B\x00\x00\x00\x00\x00\x33",
			"xx?xx?????x???xxx");
		if (ndis_filter_block == 0) return false;
		n_log::printf("ndis filter block address : %llx \n", ndis_filter_block);

		DWORD ndis_filter_block_offset = *(PDWORD)((PBYTE)ndis_filter_block + 12);
		if (ndis_filter_block_offset == 0) return false;
		n_log::printf("ndis filter block offset value : %x \n", ndis_filter_block_offset);

		ndis_global_filter_list = (PNDIS_FILTER_BLOCK)((PBYTE)ndis_global_filter_list + 3);
		ndis_global_filter_list = *(PNDIS_FILTER_BLOCK *)((PBYTE)ndis_global_filter_list + 7 + *(PINT)((PBYTE)ndis_global_filter_list + 3));
		n_log::printf("ndis global filter list address : %llx \n", ndis_global_filter_list);

		for (PNDIS_FILTER_BLOCK filter = ndis_global_filter_list; filter; filter = filter->NextFilter)
		{
			PNDIS_IF_BLOCK block = *(PNDIS_IF_BLOCK *)((PBYTE)filter + ndis_filter_block_offset);
			if (block == 0) continue;

			size_t length = wcslen(filter->FilterInstanceName->Buffer);
			const unsigned long tag = 'Nics';
			wchar_t* buffer = (wchar_t*)ExAllocatePoolWithTag(NonPagedPool, length, tag);
			if (buffer)
			{
				MM_COPY_ADDRESS addr{ 0 };
				addr.VirtualAddress = filter->FilterInstanceName->Buffer;

				SIZE_T read_size = 0;
				NTSTATUS status = MmCopyMemory(buffer, addr, length, MM_COPY_MEMORY_VIRTUAL, &read_size);
				if (status == STATUS_SUCCESS && read_size == length)
				{
					wchar_t* memory = (wchar_t*)ExAllocatePoolWithTag(NonPagedPool, length * 2, tag);
					if (memory)
					{
						RtlStringCbPrintfW(memory, length * 2, L"\\Device\\%ws", paste_guid(buffer, length));

						UNICODE_STRING adapter;
						RtlInitUnicodeString(&adapter, memory);

						PFILE_OBJECT file_object = 0;
						PDEVICE_OBJECT device_object = 0;
						status = IoGetDeviceObjectPointer(&adapter, FILE_READ_DATA, &file_object, &device_object);
						if (NT_SUCCESS(status))
						{
							PDRIVER_OBJECT driver_object = device_object->DriverObject;

							bool exists = false;
							for (int i = 0; i < array_size; i++)
							{
								if (g_nic_array[i].driver_object == driver_object)
								{
									exists = true;
									break;
								}
							}

							if (exists == false)
							{
								g_nic_array[array_size].driver_object = driver_object;
								g_nic_array[array_size].original_function = driver_object->MajorFunction[IRP_MJ_DEVICE_CONTROL];
								driver_object->MajorFunction[IRP_MJ_DEVICE_CONTROL] = my_mac_handle_control;
								n_log::printf("nic hook %llx -> %llx \n", g_nic_array[array_size].original_function, my_mac_handle_control);
								array_size++;
							}

							ObDereferenceObject(file_object);
						}

						ExFreePoolWithTag(memory, tag);
					}
				}

				ExFreePoolWithTag(buffer, tag);
			}

			n_util::random_string((char*)block->ifPhysAddress.Address, block->ifPhysAddress.Length);
			n_util::random_string((char*)block->PermanentPhysAddress.Address, block->PermanentPhysAddress.Length);
		}

		return true;
	}

	bool start_hook()
	{
		g_original_arp_control = n_util::add_irp_hook(L"\\Driver\\nsiproxy", my_arp_handle_control);
		return g_original_arp_control;
	}

	bool clean_hook()
	{
		KIRQL irql;
		KeAcquireSpinLock(&g_lock, &irql);

		for (int i = 0; i < array_size; i++)
		{
			NIC_ARRAY& item = g_nic_array[i];
			item.driver_object->MajorFunction[IRP_MJ_DEVICE_CONTROL] = item.original_function;
			n_log::printf("clean nic hook %llx -> %llx \n", my_mac_handle_control, item.driver_object->MajorFunction[IRP_MJ_DEVICE_CONTROL]);
		}

		KeReleaseSpinLock(&g_lock, irql);

		bool res = n_util::del_irp_hook(L"\\Driver\\nsiproxy", g_original_arp_control);
		return res;
	}
}