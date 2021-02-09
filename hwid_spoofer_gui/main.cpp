#include "disk.h"
#include "loader.hpp"
#include "resource.h"

#include <time.h>
#include <windowsx.h>

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

// 开启Win32视觉效果
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name = 'Microsoft.Windows.Common-Controls' \
version = '6.0.0.0' \
processorArchitecture = '*' \
publicKeyToken = '6595b64144ccf1df' \
language = '*'\"")

const wchar_t* drv_name = L"NSpoofer";
HANDLE g_Driver = INVALID_HANDLE_VALUE;

// 硬盘初始化
VOID DiskInitialize(HWND h)
{
	std::map<char, unsigned long> drivers = get_disk_all_drive_serial();
	for (const auto& it : drivers)
	{
		char buffer[20]{ 0 };
		wsprintfA(buffer, "%c", it.first);
		ComboBox_AddString(GetDlgItem(h, IDC_COMBO1), buffer);
	}
}

// 显卡初始化
VOID GpuInitialize(HWND h)
{
	FILE *file = 0;
	char cmd[1024] = { 0 };

	if ((file = _popen("nvidia-smi -L", "r")) != NULL)
	{
		fgets(cmd, 1024, file);
		_pclose(file);

		auto beg = std::string{ cmd }.find("GPU-");
		auto end = std::string{ cmd }.find(")", beg);
		if (beg == std::string::npos || end == std::string::npos) return;
		std::string buf = std::string{ cmd }.substr(beg + 4, end - (beg + 4));

		SetWindowTextA(GetDlgItem(h, IDC_EDIT4), buf.c_str());
	}
}

// 硬盘的Command
VOID DiskCommand(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int wmId = LOWORD(wParam);
	int wmEvent = HIWORD(wParam);

	switch (wmId)
	{
	case IDC_BUTTON1:
	{
		char buffer[50]{ 0 };
		ComboBox_GetText(GetDlgItem(hWnd, IDC_COMBO1), buffer, 50);
		if (strlen(buffer) == 0)
		{
			MessageBoxA(hWnd, "未选择卷标", 0, MB_OK | MB_ICONERROR);
			return;
		}

		char serial[50]{ 0 };
		GetWindowTextA(GetDlgItem(hWnd, IDC_EDIT1), serial, 50);
		if (strlen(serial) == 0)
		{
			MessageBoxA(hWnd, "请输入新序列号", 0, MB_OK | MB_ICONERROR);
			return;
		}

		if (change_serial_number(buffer[0], atoi(serial)))
			MessageBoxA(hWnd, "卷标序列号修改成功", serial, MB_OK);
		else
			MessageBoxA(hWnd, "卷标序列号修改失败", buffer, MB_OK | MB_ICONERROR);
	}
	break;
	case IDC_BUTTON2:
	{
		srand((unsigned int)time(0));
		std::map<char, unsigned long> drivers = get_disk_all_drive_serial();
		for (const auto& it : drivers)
		{
			if (change_serial_number(it.first, rand() * rand()))
				MessageBoxA(hWnd, "卷标序列号修改成功", "温馨提示", MB_OK);
			else
				MessageBoxA(hWnd, "卷标序列号修改失败", 0, MB_OK | MB_ICONERROR);
		}
	}
	break;
	case IDC_BUTTON5:
	{
		if (g_Driver == INVALID_HANDLE_VALUE)
		{
			MessageBoxA(hWnd, "未链接驱动程序", 0, MB_OK | MB_ICONERROR);
			return;
		}

		DWORD res = 0;
		common_buffer common{ 0 };
		DeviceIoControl(g_Driver, ioctl_disk_disable_smart, &common, sizeof(common), 0, 0, &res, 0);
	}
	break;
	case IDC_BUTTON6:
	{
		if (g_Driver == INVALID_HANDLE_VALUE)
		{
			MessageBoxA(hWnd, "未链接驱动程序", 0, MB_OK | MB_ICONERROR);
			return;
		}

		char buffer[100]{ 0 };
		GetWindowTextA(GetDlgItem(hWnd, IDC_EDIT3), buffer, 100);
		if (strlen(buffer) == 0)
		{
			MessageBoxA(hWnd, "请输入硬盘新序列号", 0, MB_OK | MB_ICONERROR);
			return;
		}

		DWORD res = 0;
		common_buffer common{ 0 };
		strcpy(common._disk.serial_buffer, buffer);
		DeviceIoControl(g_Driver, ioctl_disk_disable_smart, &common, sizeof(common), 0, 0, &res, 0);
	}
	break;
	}

	switch (wmEvent)
	{
	case CBN_SELCHANGE:
		if (wmId == IDC_COMBO1)
		{
			char buffer[50]{ 0 };
			ComboBox_GetText(GetDlgItem(hWnd, IDC_COMBO1), buffer, 50);

			std::map<char, unsigned long> drivers = get_disk_all_drive_serial();
			for (const auto& it : drivers)
			{
				if (it.first == buffer[0])
				{
					SetWindowTextA(GetDlgItem(hWnd, IDC_EDIT1), std::to_string(it.second).c_str());
					break;
				}
			}
		}
		break;
	case BN_CLICKED:
		if (wmId == IDC_CHECK1)
		{
			if (g_Driver == INVALID_HANDLE_VALUE)
			{
				MessageBoxA(hWnd, "未链接驱动程序", 0, MB_OK | MB_ICONERROR);
				SendMessageA(GetDlgItem(hWnd, IDC_CHECK1), BM_SETCHECK, 0, 0);
				return;
			}

			char serial_buffer[100]{ 0 };
			char disk_product_buffer[100]{ 0 };
			char disk_product_revision_buffer[100]{ 0 };
			GetWindowTextA(GetDlgItem(hWnd, IDC_EDIT2), serial_buffer, 100);
			GetWindowTextA(GetDlgItem(hWnd, IDC_EDIT5), disk_product_buffer, 100);
			GetWindowTextA(GetDlgItem(hWnd, IDC_EDIT6), disk_product_revision_buffer, 100);
			if (strlen(serial_buffer) == 0 || strlen(disk_product_buffer) == 0 || strlen(disk_product_revision_buffer) == 0)
			{
				MessageBoxA(hWnd, "请输入完整硬盘新值", 0, MB_OK | MB_ICONERROR);
				SendMessageA(GetDlgItem(hWnd, IDC_CHECK1), BM_SETCHECK, 0, 0);
				return;
			}

			LRESULT result = SendMessageA(GetDlgItem(hWnd, IDC_CHECK1), BM_GETCHECK, 0, 0);
			if (result != BST_CHECKED) return;

			DWORD res = 0;
			common_buffer common{ 0 };
			common._disk.disk_mode = 0;
			strcpy(common._disk.serial_buffer, serial_buffer);
			strcpy(common._disk.product_buffer, disk_product_buffer);
			strcpy(common._disk.product_revision_buffer, disk_product_revision_buffer);
			DeviceIoControl(g_Driver, ioctl_disk_customize_serial, &common, sizeof(common), 0, 0, &res, 0);

			SendMessageA(GetDlgItem(hWnd, IDC_CHECK2), BM_SETCHECK, 0, 0);
			SendMessageA(GetDlgItem(hWnd, IDC_CHECK3), BM_SETCHECK, 0, 0);
		}
		if (wmId == IDC_CHECK2)
		{
			if (g_Driver == INVALID_HANDLE_VALUE)
			{
				MessageBoxA(hWnd, "未链接驱动程序", 0, MB_OK | MB_ICONERROR);
				SendMessageA(GetDlgItem(hWnd, IDC_CHECK2), BM_SETCHECK, 0, 0);
				return;
			}

			LRESULT result = SendMessageA(GetDlgItem(hWnd, IDC_CHECK2), BM_GETCHECK, 0, 0);
			if (result != BST_CHECKED) return;

			DWORD res = 0;
			common_buffer common{ 0 };
			common._disk.disk_mode = 1;
			DeviceIoControl(g_Driver, ioctl_disk_random_serial, &common, sizeof(common), 0, 0, &res, 0);

			SendMessageA(GetDlgItem(hWnd, IDC_CHECK1), BM_SETCHECK, 0, 0);
			SendMessageA(GetDlgItem(hWnd, IDC_CHECK3), BM_SETCHECK, 0, 0);
		}
		if (wmId == IDC_CHECK3)
		{
			if (g_Driver == INVALID_HANDLE_VALUE)
			{
				MessageBoxA(hWnd, "未链接驱动程序", 0, MB_OK | MB_ICONERROR);
				SendMessageA(GetDlgItem(hWnd, IDC_CHECK3), BM_SETCHECK, 0, 0);
				return;
			}

			LRESULT result = SendMessageA(GetDlgItem(hWnd, IDC_CHECK3), BM_GETCHECK, 0, 0);
			if (result != BST_CHECKED) return;

			DWORD res = 0;
			common_buffer common{ 0 };
			common._disk.disk_mode = 2;
			DeviceIoControl(g_Driver, ioctl_disk_null_serial, &common, sizeof(common), 0, 0, &res, 0);

			SendMessageA(GetDlgItem(hWnd, IDC_CHECK2), BM_SETCHECK, 0, 0);
			SendMessageA(GetDlgItem(hWnd, IDC_CHECK1), BM_SETCHECK, 0, 0);
		}
		if (wmId == IDC_CHECK4)
		{
			if (g_Driver == INVALID_HANDLE_VALUE)
			{
				MessageBoxA(hWnd, "未链接驱动程序", 0, MB_OK | MB_ICONERROR);
				return;
			}

			LRESULT result = SendMessageA(GetDlgItem(hWnd, IDC_CHECK4), BM_GETCHECK, 0, 0);

			DWORD res = 0;
			common_buffer common{ 0 };
			common._disk.guid_state = result;
			DeviceIoControl(g_Driver, ioctl_disk_random_guid, &common, sizeof(common), 0, 0, &res, 0);
		}
		if (wmId == IDC_CHECK5)
		{
			if (g_Driver == INVALID_HANDLE_VALUE)
			{
				MessageBoxA(hWnd, "未链接驱动程序", 0, MB_OK | MB_ICONERROR);
				return;
			}

			LRESULT result = SendMessageA(GetDlgItem(hWnd, IDC_CHECK5), BM_GETCHECK, 0, 0);

			DWORD res = 0;
			common_buffer common{ 0 };
			common._disk.volumn_state = result;
			DeviceIoControl(g_Driver, ioctl_disk_null_volumn, &common, sizeof(common), 0, 0, &res, 0);
		}
		break;
	}
}

// 驱动的Command
VOID DriverCommand(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int wmId = LOWORD(wParam);
	int wmEvent = HIWORD(wParam);

	switch (wmId)
	{
	case IDC_BUTTON3:
	{
		if (g_Driver != INVALID_HANDLE_VALUE)
		{
			MessageBoxA(hWnd, "不允许重复加载驱动程序", 0, MB_OK | MB_ICONERROR);
			return;
		}

		OPENFILENAMEW ofn = { 0 };
		WCHAR strFilename[MAX_PATH] = { 0 };//用于接收文件名
		ofn.lStructSize = sizeof(OPENFILENAMEW);//结构体大小
		ofn.hwndOwner = NULL;//拥有着窗口句柄，为NULL表示对话框是非模态的，实际应用中一般都要有这个句柄
		ofn.lpstrFilter = L"SYS文件\0*.sys\0\0";//设置过滤
		ofn.nFilterIndex = 1;//过滤器索引
		ofn.lpstrFile = strFilename;//接收返回的文件名，注意第一个字符需要为NULL
		ofn.nMaxFile = sizeof(strFilename);//缓冲区长度
		ofn.lpstrInitialDir = NULL;//初始目录为默认
		ofn.lpstrTitle = L"请选择一个文件";//使用系统默认标题留空即可
		ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;//文件、目录必须存在，隐藏只读选项
		if (GetOpenFileNameW(&ofn) == FALSE)
		{
			MessageBoxA(hWnd, "没有选择驱动文件", 0, MB_OK | MB_ICONERROR);
			return;
		}

		if (start_install_driver(strFilename, drv_name, true) == false)
		{
			MessageBoxA(hWnd, "安装驱动程序失败", 0, MB_OK | MB_ICONERROR);
			return;
		}

#define DRIVER_NAME "\\\\.\\HwidSpoofer"
		g_Driver = CreateFileA(DRIVER_NAME, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
		if (g_Driver == INVALID_HANDLE_VALUE)
			MessageBoxA(hWnd, "链接驱动程序失败", 0, MB_OK | MB_ICONERROR);
		else
		{
			MessageBoxA(hWnd, "链接驱动程序成功", "温馨提示", MB_OK);
			EnableWindow(GetDlgItem(hWnd, IDC_BUTTON3), FALSE);
		}
	}
	break;
	case IDC_BUTTON4:
	{
		if (g_Driver != INVALID_HANDLE_VALUE)
			CloseHandle(g_Driver);
		g_Driver = INVALID_HANDLE_VALUE;

		stop_driver(drv_name);
		unload_driver(drv_name);

		MessageBoxA(hWnd, "卸载驱动成功", "温馨提示", MB_OK);
		EnableWindow(GetDlgItem(hWnd, IDC_BUTTON3), TRUE);
	}
	break;
	}
}

// BOIS的Command
VOID BoisCommand(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int wmId = LOWORD(wParam);
	int wmEvent = HIWORD(wParam);

	switch (wmId)
	{
	case IDC_BUTTON7:
	{
		if (g_Driver == INVALID_HANDLE_VALUE)
		{
			MessageBoxA(hWnd, "未链接驱动程序", 0, MB_OK | MB_ICONERROR);
			return;
		}

		char vendor[100]{ 0 };
		char version[100]{ 0 };
		char date[100]{ 0 };
		char manufacturer[100]{ 0 };
		char product_name[100]{ 0 };
		char serial_number[100]{ 0 };
		GetWindowTextA(GetDlgItem(hWnd, IDC_EDIT7), vendor, 100);
		GetWindowTextA(GetDlgItem(hWnd, IDC_EDIT8), version, 100);
		GetWindowTextA(GetDlgItem(hWnd, IDC_EDIT9), date, 100);
		GetWindowTextA(GetDlgItem(hWnd, IDC_EDIT10), manufacturer, 100);
		GetWindowTextA(GetDlgItem(hWnd, IDC_EDIT11), product_name, 100);
		GetWindowTextA(GetDlgItem(hWnd, IDC_EDIT12), serial_number, 100);
		if (strlen(vendor) == 0
			|| strlen(version) == 0
			|| strlen(date) == 0
			|| strlen(manufacturer) == 0
			|| strlen(product_name) == 0
			|| strlen(serial_number) == 0)
		{
			MessageBoxA(hWnd, "请填写全部BOIS信息", 0, MB_OK | MB_ICONERROR);
			return;
		}

		if (MessageBoxA(hWnd, "只能修改一次,二次修改会导致蓝屏,是否继续?", "温馨提示", MB_YESNO | MB_ICONQUESTION) == IDYES)
		{
			DWORD res = 0;
			common_buffer common{ 0 };
			strcpy(common._smbois.vendor, vendor);
			strcpy(common._smbois.version, version);
			strcpy(common._smbois.date, date);
			strcpy(common._smbois.manufacturer, manufacturer);
			strcpy(common._smbois.product_name, product_name);
			strcpy(common._smbois.serial_number, serial_number);
			DeviceIoControl(g_Driver, ioctl_smbois_customize, &common, sizeof(common), 0, 0, &res, 0);
		}
	}
	break;
	}
}

// 显卡的Command
VOID GpuCommand(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int wmId = LOWORD(wParam);
	int wmEvent = HIWORD(wParam);

	switch (wmId)
	{
	case IDC_BUTTON8:
	{
		if (g_Driver == INVALID_HANDLE_VALUE)
		{
			MessageBoxA(hWnd, "未链接驱动程序", 0, MB_OK | MB_ICONERROR);
			return;
		}

		char buffer[100]{ 0 };
		GetWindowTextA(GetDlgItem(hWnd, IDC_EDIT4), buffer, 100);
		if (strlen(buffer) == 0)
		{
			MessageBoxA(hWnd, "请输入显卡新序列号", 0, MB_OK | MB_ICONERROR);
			return;
		}

		DWORD res = 0;
		common_buffer common{ 0 };
		strcpy(common._gpu.serial_buffer, buffer);
		DeviceIoControl(g_Driver, ioctl_gpu_customize, &common, sizeof(common), 0, 0, &res, 0);
	}
	break;
	}
}

// 网卡的Command
VOID NicCommand(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int wmId = LOWORD(wParam);
	int wmEvent = HIWORD(wParam);

	switch (wmEvent)
	{
	case BN_CLICKED:
		if (wmId == IDC_CHECK6)
		{
			if (g_Driver == INVALID_HANDLE_VALUE)
			{
				MessageBoxA(hWnd, "未链接驱动程序", 0, MB_OK | MB_ICONERROR);
				return;
			}

			LRESULT result = SendMessageA(GetDlgItem(hWnd, IDC_CHECK6), BM_GETCHECK, 0, 0);

			DWORD res = 0;
			common_buffer common{ 0 };
			common._nic.arp_table = result;
			DeviceIoControl(g_Driver, ioctl_arp_table_handle, &common, sizeof(common), 0, 0, &res, 0);
		}
		if (wmId == IDC_CHECK7)
		{
			if (g_Driver == INVALID_HANDLE_VALUE)
			{
				MessageBoxA(hWnd, "未链接驱动程序", 0, MB_OK | MB_ICONERROR);
				return;
			}

			LRESULT result = SendMessageA(GetDlgItem(hWnd, IDC_CHECK7), BM_GETCHECK, 0, 0);
			if (result == 0) return;

			SendMessageA(GetDlgItem(hWnd, IDC_CHECK8), BM_SETCHECK, 0, 0);

			DWORD res = 0;
			common_buffer common{ 0 };
			common._nic.mac_mode = 0;
			DeviceIoControl(g_Driver, ioctl_mac_random, &common, sizeof(common), 0, 0, &res, 0);
		}
		if (wmId == IDC_CHECK8)
		{
			if (g_Driver == INVALID_HANDLE_VALUE)
			{
				MessageBoxA(hWnd, "未链接驱动程序", 0, MB_OK | MB_ICONERROR);
				return;
			}

			LRESULT result = SendMessageA(GetDlgItem(hWnd, IDC_CHECK8), BM_GETCHECK, 0, 0);
			if (result == 0) return;

			SendMessageA(GetDlgItem(hWnd, IDC_CHECK7), BM_SETCHECK, 0, 0);

			char permanent_mac[100]{ 0 };
			char current_mac[100]{ 0 };
			GetWindowTextA(GetDlgItem(hWnd, IDC_EDIT15), permanent_mac, 100);
			GetWindowTextA(GetDlgItem(hWnd, IDC_EDIT17), current_mac, 100);
			if (strlen(permanent_mac) == 0 || strlen(current_mac) == 0)
			{
				MessageBoxA(hWnd, "请输入完整MAC地址", 0, MB_OK | MB_ICONERROR);
				return;
			}

			DWORD res = 0;
			common_buffer common{ 0 };
			strcpy(common._nic.permanent, permanent_mac);
			strcpy(common._nic.current, current_mac);
			common._nic.mac_mode = 1;
			DeviceIoControl(g_Driver, ioctl_mac_customize, &common, sizeof(common), 0, 0, &res, 0);
		}
		break;
	}
}

// 窗口过程函数
INT_PTR CALLBACK DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		DiskInitialize(hWnd);
		GpuInitialize(hWnd);
		return 1;
	case WM_COMMAND:
		DiskCommand(hWnd, uMsg, wParam, lParam);
		DriverCommand(hWnd, uMsg, wParam, lParam);
		BoisCommand(hWnd, uMsg, wParam, lParam);
		GpuCommand(hWnd, uMsg, wParam, lParam);
		NicCommand(hWnd, uMsg, wParam, lParam);
		return 1;
	case WM_CLOSE:
		if (g_Driver != INVALID_HANDLE_VALUE)
		{
			MessageBoxA(hWnd, "请先卸载驱动后再结束程序", 0, MB_OK | MB_ICONERROR);
			return 0;
		}
		PostQuitMessage(0);
		return 1;
	}

	return 0;
}

// 程序人口
int _stdcall WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
	HWND h = CreateDialogParamA(hInstance, MAKEINTRESOURCEA(IDD_DIALOG1), 0, DialogProc, 0);
	if (h)
	{
		// 创建窗口后马上显示和更新,不然窗口不出来哈
		ShowWindow(h, SW_SHOW);
		UpdateWindow(h);

		// 设置一下程序的图标
		SendMessage(h, WM_SETICON, FALSE, (LPARAM)LoadIconA(hInstance, MAKEINTRESOURCEA(IDI_ICON1)));

		MSG msg{ 0 };
		while (GetMessageA(&msg, 0, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessageA(&msg);
		}
	}

	return 1;
}