#include "disk.h"
#include "loader.hpp"
#include "resource.h"
#include "lang.h"

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

// Enable Win32 visual styles
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name = 'Microsoft.Windows.Common-Controls' \
version = '6.0.0.0' \
processorArchitecture = '*' \
publicKeyToken = '6595b64144ccf1df' \
language = '*'\"")

const wchar_t* drv_name = L"NSpoofer";

// Localized message box. detail (optional) is appended to the text.
int Msg(HWND hWnd, Str text, UINT flags, const char* detail = nullptr)
{
	std::wstring body = T(text);
	if (detail && *detail)
	{
		body += L"\n";
		body += std::wstring(detail, detail + strlen(detail));
	}
	bool is_error = (flags & MB_ICONMASK) == MB_ICONERROR;
	return MessageBoxW(hWnd, body.c_str(), T(is_error ? S_ERROR : S_NOTICE), flags);
}
HANDLE g_Driver = INVALID_HANDLE_VALUE;

// Disk init
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

// GPU init
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

// Disk commands
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
			Msg(hWnd, S_NO_VOLUME, MB_OK | MB_ICONERROR);
			return;
		}

		char serial[50]{ 0 };
		GetWindowTextA(GetDlgItem(hWnd, IDC_EDIT1), serial, 50);
		if (strlen(serial) == 0)
		{
			Msg(hWnd, S_NEED_SERIAL, MB_OK | MB_ICONERROR);
			return;
		}

		if (change_serial_number(buffer[0], atoi(serial)))
			Msg(hWnd, S_SERIAL_OK, MB_OK, serial);
		else
			Msg(hWnd, S_SERIAL_FAIL, MB_OK | MB_ICONERROR, buffer);
	}
	break;
	case IDC_BUTTON2:
	{
		srand((unsigned int)time(0));
		std::map<char, unsigned long> drivers = get_disk_all_drive_serial();
		for (const auto& it : drivers)
		{
			if (change_serial_number(it.first, rand() * rand()))
				Msg(hWnd, S_SERIAL_OK, MB_OK | MB_ICONINFORMATION);
			else
				Msg(hWnd, S_SERIAL_FAIL, MB_OK | MB_ICONERROR);
		}
	}
	break;
	case IDC_BUTTON5:
	{
		if (g_Driver == INVALID_HANDLE_VALUE)
		{
			Msg(hWnd, S_NO_DRIVER, MB_OK | MB_ICONERROR);
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
			Msg(hWnd, S_NO_DRIVER, MB_OK | MB_ICONERROR);
			return;
		}

		char buffer[100]{ 0 };
		GetWindowTextA(GetDlgItem(hWnd, IDC_EDIT3), buffer, 100);
		if (strlen(buffer) == 0)
		{
			Msg(hWnd, S_NEED_DISK_SERIAL, MB_OK | MB_ICONERROR);
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
				Msg(hWnd, S_NO_DRIVER, MB_OK | MB_ICONERROR);
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
				Msg(hWnd, S_NEED_DISK_VALUES, MB_OK | MB_ICONERROR);
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
				Msg(hWnd, S_NO_DRIVER, MB_OK | MB_ICONERROR);
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
				Msg(hWnd, S_NO_DRIVER, MB_OK | MB_ICONERROR);
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
				Msg(hWnd, S_NO_DRIVER, MB_OK | MB_ICONERROR);
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
				Msg(hWnd, S_NO_DRIVER, MB_OK | MB_ICONERROR);
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

// Driver commands
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
			Msg(hWnd, S_DRIVER_LOADED_ALREADY, MB_OK | MB_ICONERROR);
			return;
		}

		OPENFILENAMEW ofn = { 0 };
		WCHAR strFilename[MAX_PATH] = { 0 };
		ofn.lStructSize = sizeof(OPENFILENAMEW);
		ofn.hwndOwner = hWnd;
		std::wstring filter = std::wstring(T(S_SYS_FILTER)) + std::wstring(1, 0) + L"*.sys" + std::wstring(1, 0);
		ofn.lpstrFilter = filter.c_str();
		ofn.nFilterIndex = 1;
		ofn.lpstrFile = strFilename;
		ofn.nMaxFile = MAX_PATH;
		ofn.lpstrInitialDir = NULL;
		ofn.lpstrTitle = T(S_PICK_FILE);
		ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
		if (GetOpenFileNameW(&ofn) == FALSE)
		{
			Msg(hWnd, S_NO_DRIVER_FILE, MB_OK | MB_ICONERROR);
			return;
		}

		if (start_install_driver(strFilename, drv_name, true) == false)
		{
			Msg(hWnd, S_INSTALL_FAIL, MB_OK | MB_ICONERROR);
			return;
		}

#define DRIVER_NAME "\\\\.\\HwidSpoofer"
		g_Driver = CreateFileA(DRIVER_NAME, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
		if (g_Driver == INVALID_HANDLE_VALUE)
			Msg(hWnd, S_LINK_FAIL, MB_OK | MB_ICONERROR);
		else
		{
			Msg(hWnd, S_LINK_OK, MB_OK | MB_ICONINFORMATION);
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

		Msg(hWnd, S_UNLOAD_OK, MB_OK | MB_ICONINFORMATION);
		EnableWindow(GetDlgItem(hWnd, IDC_BUTTON3), TRUE);
	}
	break;
	}
}

// BIOS commands
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
			Msg(hWnd, S_NO_DRIVER, MB_OK | MB_ICONERROR);
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
			Msg(hWnd, S_NEED_BIOS, MB_OK | MB_ICONERROR);
			return;
		}

		if (Msg(hWnd, S_BIOS_ONCE, MB_YESNO | MB_ICONQUESTION) == IDYES)
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

// GPU commands
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
			Msg(hWnd, S_NO_DRIVER, MB_OK | MB_ICONERROR);
			return;
		}

		char buffer[100]{ 0 };
		GetWindowTextA(GetDlgItem(hWnd, IDC_EDIT4), buffer, 100);
		if (strlen(buffer) == 0)
		{
			Msg(hWnd, S_NEED_GPU_SERIAL, MB_OK | MB_ICONERROR);
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

// NIC commands
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
				Msg(hWnd, S_NO_DRIVER, MB_OK | MB_ICONERROR);
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
				Msg(hWnd, S_NO_DRIVER, MB_OK | MB_ICONERROR);
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
				Msg(hWnd, S_NO_DRIVER, MB_OK | MB_ICONERROR);
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
				Msg(hWnd, S_NEED_MAC, MB_OK | MB_ICONERROR);
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

// Dialog procedure
INT_PTR CALLBACK DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		LangDetect();
		LangApply(hWnd);
		DiskInitialize(hWnd);
		GpuInitialize(hWnd);
		return 1;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDC_BUTTON_LANG)
		{
			g_lang = (g_lang == LANG_EN) ? LANG_ZH : LANG_EN;
			LangApply(hWnd);
			return 1;
		}
		DiskCommand(hWnd, uMsg, wParam, lParam);
		DriverCommand(hWnd, uMsg, wParam, lParam);
		BoisCommand(hWnd, uMsg, wParam, lParam);
		GpuCommand(hWnd, uMsg, wParam, lParam);
		NicCommand(hWnd, uMsg, wParam, lParam);
		return 1;
	case WM_CLOSE:
		if (g_Driver != INVALID_HANDLE_VALUE)
		{
			Msg(hWnd, S_UNLOAD_FIRST, MB_OK | MB_ICONERROR);
			return 0;
		}
		PostQuitMessage(0);
		return 1;
	}

	return 0;
}

// Entry point
int _stdcall WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
	HWND h = CreateDialogParamW(hInstance, MAKEINTRESOURCEW(IDD_DIALOG1), 0, DialogProc, 0);
	if (h)
	{
		// Show and update right after creation
		ShowWindow(h, SW_SHOW);
		UpdateWindow(h);

		// Set the window icon
		SendMessageW(h, WM_SETICON, FALSE, (LPARAM)LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_ICON1)));

		MSG msg{ 0 };
		while (GetMessageW(&msg, 0, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
	}

	return 1;
}