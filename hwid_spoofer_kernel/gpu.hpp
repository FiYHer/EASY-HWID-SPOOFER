#pragma once
#include "util.hpp"
#include "log.hpp"

namespace n_gpu
{
	char customize_gpu_serial[100]{ 0 };

	PDRIVER_DISPATCH g_original_device_control = 0;

	NTSTATUS my_device_control(PDEVICE_OBJECT device, PIRP irp)
	{
		PIO_STACK_LOCATION ioc = IoGetCurrentIrpStackLocation(irp);

#define IOCTL_NVIDIA_SMIL (0x8DE0008)
#define IOCTL_NVIDIA_SMIL_MAX (512)
		if (ioc->Parameters.DeviceIoControl.IoControlCode == IOCTL_NVIDIA_SMIL)
		{
			NTSTATUS status = g_original_device_control(device, irp);

			char* original_buffer = (char*)irp->UserBuffer;
			const int length = IOCTL_NVIDIA_SMIL_MAX;

			if (original_buffer)
			{
				const unsigned long tag = 'Gput';
				void* buffer = ExAllocatePoolWithTag(NonPagedPool, length, tag);
				if (buffer)
				{
					MM_COPY_ADDRESS addr{ 0 };
					addr.VirtualAddress = irp->UserBuffer;

					SIZE_T copy_size = 0;
					if (NT_SUCCESS(MmCopyMemory(buffer, addr, length, MM_COPY_MEMORY_VIRTUAL, &copy_size))
						&& copy_size == length)
					{
						const char* gpu = "GPU-";
						const size_t len = strlen(gpu);

						for (int i = 0; i < length - len; i++)
						{
							char* ptr = (char*)buffer + i;
							if (0 == memcmp(ptr, gpu, strlen(gpu)))
							{
								RtlCopyMemory(original_buffer + i + len, customize_gpu_serial, 100);
								// n_util::random_string(original_buffer + i + len, 0);
								break;
							}
						}
					}

					ExFreePoolWithTag(buffer, tag);
				}
			}

			return status;
		}

		return g_original_device_control(device, irp);
	}

	bool start_hook()
	{
		g_original_device_control = n_util::add_irp_hook(L"\\Driver\\nvlddmkm", my_device_control);
		return g_original_device_control;
	}

	bool clean_hook()
	{
		return n_util::del_irp_hook(L"\\Driver\\nvlddmkm", g_original_device_control);
	}
}