#include "smbios.hpp"
#include "disk.hpp"
#include "gpu.hpp"
#include "nic.hpp"

#define ioctl_disk_customize_serial CTL_CODE(FILE_DEVICE_UNKNOWN, 0x500, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define ioctl_disk_random_serial CTL_CODE(FILE_DEVICE_UNKNOWN, 0x501, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define ioctl_disk_null_serial CTL_CODE(FILE_DEVICE_UNKNOWN, 0x502, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define ioctl_disk_random_guid CTL_CODE(FILE_DEVICE_UNKNOWN, 0x503, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define ioctl_disk_null_volumn CTL_CODE(FILE_DEVICE_UNKNOWN, 0x504, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define ioctl_disk_disable_smart CTL_CODE(FILE_DEVICE_UNKNOWN, 0x505, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define ioctl_disk_change_serial CTL_CODE(FILE_DEVICE_UNKNOWN, 0x506, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)

#define ioctl_smbois_customize CTL_CODE(FILE_DEVICE_UNKNOWN, 0x600, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)

#define ioctl_gpu_customize CTL_CODE(FILE_DEVICE_UNKNOWN, 0x700, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)

#define ioctl_arp_table_handle CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define ioctl_mac_random CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define ioctl_mac_customize CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)

struct common_buffer
{
	union
	{
		struct disk
		{
			int disk_mode;
			char serial_buffer[100];
			char product_buffer[100];
			char product_revision_buffer[100];
			bool guid_state;
			bool volumn_state;
		}_disk;

		struct smbois
		{
			char vendor[100]{ 0 };
			char version[100]{ 0 };
			char date[100]{ 0 };
			char manufacturer[100]{ 0 };
			char product_name[100]{ 0 };
			char serial_number[100]{ 0 };
		}_smbois;

		struct gpu
		{
			char serial_buffer[100];
		}_gpu;

		struct nic
		{
			bool arp_table;
			int mac_mode;
			char permanent[100]{ 0 };
			char current[100]{ 0 };
		}_nic;
	};
};

PDEVICE_OBJECT g_device_object = nullptr;

NTSTATUS CreateIrp(PDEVICE_OBJECT device, PIRP irp)
{
	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;

	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS CloseIrp(PDEVICE_OBJECT device, PIRP irp)
{
	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;

	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS ControlIrp(PDEVICE_OBJECT device, PIRP irp)
{
	PIO_STACK_LOCATION io = IoGetCurrentIrpStackLocation(irp);

	common_buffer common{ 0 };
	RtlCopyMemory(&common, irp->AssociatedIrp.SystemBuffer, sizeof(common));

	// с╡ел
	switch (io->Parameters.DeviceIoControl.IoControlCode)
	{
	case ioctl_disk_customize_serial:
		n_disk::disk_mode = common._disk.disk_mode;
		RtlCopyMemory(n_disk::disk_serial_buffer, common._disk.serial_buffer, 100);
		RtlCopyMemory(n_disk::disk_product_buffer, common._disk.product_buffer, 100);
		RtlCopyMemory(n_disk::disk_product_revision_buffer, common._disk.product_revision_buffer, 100);
		break;
	case ioctl_disk_random_serial:
		n_disk::disk_mode = common._disk.disk_mode;
		break;
	case ioctl_disk_null_serial:
		n_disk::disk_mode = common._disk.disk_mode;
		break;
	case ioctl_disk_random_guid:
		n_disk::disk_guid_random = common._disk.guid_state;
		break;
	case ioctl_disk_null_volumn:
		n_disk::disk_volumn_clean = common._disk.volumn_state;
		break;
	case ioctl_disk_disable_smart:
		n_disk::disable_smart();
		n_disk::disk_smart_disable = true;
		break;
	case ioctl_disk_change_serial:
		RtlCopyMemory(n_disk::disk_serial_buffer, common._disk.serial_buffer, 100);
		n_disk::change_disk_serials();
		break;
	}

	// smbois
	switch (io->Parameters.DeviceIoControl.IoControlCode)
	{
	case ioctl_smbois_customize:
		RtlCopyMemory(n_smbios::smbois_vendor, common._smbois.vendor, 100);
		RtlCopyMemory(n_smbios::smbois_version, common._smbois.version, 100);
		RtlCopyMemory(n_smbios::smbois_date, common._smbois.date, 100);
		RtlCopyMemory(n_smbios::smbois_manufacturer, common._smbois.manufacturer, 100);
		RtlCopyMemory(n_smbios::smbois_product_name, common._smbois.product_name, 100);
		RtlCopyMemory(n_smbios::smbois_serial_number, common._smbois.serial_number, 100);
		n_smbios::spoofer_smbios();
		break;
	}

	// gpu
	switch (io->Parameters.DeviceIoControl.IoControlCode)
	{
	case ioctl_gpu_customize:
		RtlCopyMemory(n_gpu::customize_gpu_serial, common._gpu.serial_buffer, 100);
		break;
	}

	// mac
	switch (io->Parameters.DeviceIoControl.IoControlCode)
	{
	case ioctl_arp_table_handle:
		n_nic::arp_table_handle = common._nic.arp_table;
		break;
	case ioctl_mac_random:
		n_nic::mac_mode = common._nic.mac_mode;
		break;
	case ioctl_mac_customize:
		RtlCopyMemory(n_nic::permanent_mac, common._nic.permanent, 100);
		RtlCopyMemory(n_nic::current_mac, common._nic.current, 100);
		n_nic::mac_mode = common._nic.mac_mode;
		break;
	}

	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

extern "C" void DriverUnload(PDRIVER_OBJECT driver)
{
	n_log::printf("leave \n");

	UNICODE_STRING symbolic_link;
	RtlInitUnicodeString(&symbolic_link, L"\\DosDevices\\HwidSpoofer");
	IoDeleteSymbolicLink(&symbolic_link);

	IoDeleteDevice(driver->DeviceObject);

	n_disk::clean_hook();
	n_gpu::clean_hook();
	n_nic::clean_hook();
}

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING unicode)
{
	n_log::printf("entry \n");

	driver->DriverUnload = DriverUnload;

	UNICODE_STRING device_name;
	RtlInitUnicodeString(&device_name, L"\\Device\\HwidSpoofer");
	NTSTATUS status = IoCreateDevice(driver, 0, &device_name, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &g_device_object);
	if (!NT_SUCCESS(status) || g_device_object == nullptr) return STATUS_UNSUCCESSFUL;

	UNICODE_STRING symbolic_link;
	RtlInitUnicodeString(&symbolic_link, L"\\DosDevices\\HwidSpoofer");
	status = IoCreateSymbolicLink(&symbolic_link, &device_name);
	if (!NT_SUCCESS(status))
	{
		IoDeleteDevice(g_device_object);
		return STATUS_UNSUCCESSFUL;
	}

	driver->MajorFunction[IRP_MJ_CREATE] = CreateIrp;
	driver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ControlIrp;
	driver->MajorFunction[IRP_MJ_CLOSE] = CloseIrp;

	//n_disk::disable_smart();
	//n_disk::change_disk_serials();
	//n_disk::fuck_dispatch();
	n_disk::start_hook();
	//n_smbios::spoofer_smbios();
	n_gpu::start_hook();
	n_nic::spoofer_nic();
	n_nic::start_hook();

	g_device_object->Flags |= DO_DIRECT_IO;
	g_device_object->Flags &= ~DO_DEVICE_INITIALIZING;

	return STATUS_SUCCESS;
}