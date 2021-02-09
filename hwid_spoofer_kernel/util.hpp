#pragma once
#include <ntifs.h>
#include <ntddk.h>
#include <wdm.h>
#include <ntimage.h>

#include "log.hpp"

#ifdef __cplusplus
extern "C"
{
#endif

	typedef struct _SYSTEM_MODULE
	{
		ULONG_PTR Reserved[2];
		PVOID Base;
		ULONG Size;
		ULONG Flags;
		USHORT Index;
		USHORT Unknown;
		USHORT LoadCount;
		USHORT ModuleNameOffset;
		CHAR ImageName[256];
	} SYSTEM_MODULE, *PSYSTEM_MODULE;

	typedef struct _SYSTEM_MODULE_INFORMATION
	{
		ULONG_PTR ulModuleCount;
		SYSTEM_MODULE Modules[1];
	} SYSTEM_MODULE_INFORMATION, *PSYSTEM_MODULE_INFORMATION;

	NTSTATUS ZwQuerySystemInformation(
		DWORD32 systemInformationClass,
		PVOID systemInformation,
		ULONG systemInformationLength,
		PULONG returnLength);

	NTSTATUS ObReferenceObjectByName(
		PUNICODE_STRING objectName,
		ULONG attributes,
		PACCESS_STATE accessState,
		ACCESS_MASK desiredAccess,
		POBJECT_TYPE objectType,
		KPROCESSOR_MODE accessMode,
		PVOID parseContext, PVOID* object);

	extern POBJECT_TYPE *IoDriverObjectType;

#ifdef __cplusplus
}
#endif

namespace n_util
{
	typedef struct _IOC_REQUEST {
		PVOID Buffer;
		ULONG BufferLength;
		PVOID OldContext;
		PIO_COMPLETION_ROUTINE OldRoutine;
	} IOC_REQUEST, *PIOC_REQUEST;

	bool get_module_base_address(const char* name, DWORD64& addr, DWORD32& size)
	{
		unsigned long need_size = 0;
		ZwQuerySystemInformation(11, &need_size, 0, &need_size);
		if (need_size == 0) return false;

		const unsigned long tag = 'Util';
		PSYSTEM_MODULE_INFORMATION sys_mods = (PSYSTEM_MODULE_INFORMATION)ExAllocatePoolWithTag(NonPagedPool, need_size, tag);
		if (sys_mods == 0) return false;

		NTSTATUS status = ZwQuerySystemInformation(11, sys_mods, need_size, 0);
		if (!NT_SUCCESS(status))
		{
			ExFreePoolWithTag(sys_mods, tag);
			return false;
		}

		for (unsigned long long i = 0; i < sys_mods->ulModuleCount; i++)
		{
			PSYSTEM_MODULE mod = &sys_mods->Modules[i];
			if (strstr(mod->ImageName, name))
			{
				addr = (DWORD64)mod->Base;
				size = (DWORD32)mod->Size;
				break;
			}
		}

		ExFreePoolWithTag(sys_mods, tag);

		return true;
	}

	bool pattern_check(const char* data, const char* pattern, const char* mask)
	{
		size_t len = strlen(mask);

		for (size_t i = 0; i < len; i++)
		{
			if (data[i] == pattern[i] || mask[i] == '?')
				continue;
			else
				return false;
		}

		return true;
	}

	DWORD64 find_pattern(DWORD64 addr, DWORD32 size, const char* pattern, const char* mask)
	{
		size -= (DWORD32)strlen(mask);

		for (DWORD32 i = 0; i < size; i++)
		{
			if (pattern_check((const char*)addr + i, pattern, mask))
				return addr + i;
		}

		return 0;
	}

	DWORD64 find_pattern_image(DWORD64 addr, const char* pattern, const char* mask)
	{
		PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)addr;
		if (dos->e_magic != IMAGE_DOS_SIGNATURE) return 0;

		PIMAGE_NT_HEADERS64 nt = (PIMAGE_NT_HEADERS64)(addr + dos->e_lfanew);
		if (nt->Signature != IMAGE_NT_SIGNATURE) return 0;

		PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(nt);

		for (unsigned short i = 0; i < nt->FileHeader.NumberOfSections; i++)
		{
			PIMAGE_SECTION_HEADER p = &section[i];

			if (strstr((const char*)p->Name, ".text") || 'EGAP' == *reinterpret_cast<int*>(p->Name))
			{
				DWORD64 res = find_pattern(addr + p->VirtualAddress, p->Misc.VirtualSize, pattern, mask);
				if (res) return res;
			}
		}

		return 0;
	}

	char* random_string(char* str, int size)
	{
		if (size == 0) size = (int)strlen(str);
		if (size == 0) return 0;

		const int len = 63;
		const char char_maps[len] = "QWERTYUIOPASDFGHJKLZXCVBNMzxcvbnmasdfghjklqwertyuiop0123456789";

		unsigned long seed = KeQueryTimeIncrement();
		for (int i = 0; i < size; i++)
		{
			unsigned long index = RtlRandomEx(&seed) % len;
			str[i] = char_maps[index];
		}

		return str;
	}

	PDRIVER_DISPATCH add_irp_hook(const wchar_t* name, PDRIVER_DISPATCH new_func)
	{
		UNICODE_STRING str;
		RtlInitUnicodeString(&str, name);

		PDRIVER_OBJECT driver_object = 0;
		NTSTATUS status = ObReferenceObjectByName(&str, OBJ_CASE_INSENSITIVE, 0, 0, *IoDriverObjectType, KernelMode, 0, (void**)&driver_object);
		if (!NT_SUCCESS(status)) return 0;

		PDRIVER_DISPATCH old_func = driver_object->MajorFunction[IRP_MJ_DEVICE_CONTROL];
		driver_object->MajorFunction[IRP_MJ_DEVICE_CONTROL] = new_func;
		n_log::printf("%ws hook %llx -> %llx \n", name, old_func, new_func);

		ObDereferenceObject(driver_object);

		return old_func;
	}

	bool del_irp_hook(const wchar_t* name, PDRIVER_DISPATCH old_func)
	{
		UNICODE_STRING str;
		RtlInitUnicodeString(&str, name);

		PDRIVER_OBJECT driver_object = 0;
		NTSTATUS status = ObReferenceObjectByName(&str, OBJ_CASE_INSENSITIVE, 0, 0, *IoDriverObjectType, KernelMode, 0, (void**)&driver_object);
		if (!NT_SUCCESS(status)) return false;

		if (old_func)
		{
			n_log::printf("%ws clean hook %llx -> %llx \n", name, driver_object->MajorFunction[IRP_MJ_DEVICE_CONTROL], old_func);
			driver_object->MajorFunction[IRP_MJ_DEVICE_CONTROL] = old_func;
		}

		ObDereferenceObject(driver_object);

		return true;
	}

	bool change_ioc(PIO_STACK_LOCATION ioc, PIRP irp, PIO_COMPLETION_ROUTINE routine)
	{
		PIOC_REQUEST request = (PIOC_REQUEST)ExAllocatePool(NonPagedPool, sizeof(IOC_REQUEST));
		if (request == 0) return false;

		request->Buffer = irp->AssociatedIrp.SystemBuffer;
		request->BufferLength = ioc->Parameters.DeviceIoControl.OutputBufferLength;
		request->OldContext = ioc->Context;
		request->OldRoutine = ioc->CompletionRoutine;

		ioc->Control = SL_INVOKE_ON_SUCCESS;
		ioc->Context = request;
		ioc->CompletionRoutine = routine;

		return true;
	}
}