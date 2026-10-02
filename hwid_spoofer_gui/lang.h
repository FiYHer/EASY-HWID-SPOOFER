#pragma once
#include <windows.h>
#include "resource.h"

// UI language table. Add a column to add a language.
enum Lang { LANG_EN = 0, LANG_ZH = 1, LANG_COUNT };

enum Str
{
	S_TITLE,
	S_NOTICE,
	S_ERROR,

	// messages
	S_NO_VOLUME, S_NEED_SERIAL, S_SERIAL_OK, S_SERIAL_FAIL,
	S_NO_DRIVER, S_NEED_DISK_SERIAL, S_NEED_DISK_VALUES,
	S_DRIVER_LOADED_ALREADY, S_NO_DRIVER_FILE, S_INSTALL_FAIL,
	S_LINK_FAIL, S_LINK_OK, S_UNLOAD_OK,
	S_NEED_BIOS, S_BIOS_ONCE, S_NEED_GPU_SERIAL, S_NEED_MAC,
	S_UNLOAD_FIRST, S_SYS_FILTER, S_PICK_FILE,

	// dialog controls
	S_GRP_DISK, S_GRP_BIOS, S_GRP_GPU, S_GRP_NIC,
	S_BTN_SET_SERIAL, S_BTN_RANDOM_ALL, S_BTN_LOAD, S_BTN_UNLOAD,
	S_BTN_NO_SMART, S_BTN_NO_HOOK, S_BTN_BIOS, S_BTN_GPU, S_BTN_LANG,
	S_CHK_CUSTOM, S_CHK_RANDOM, S_CHK_CLEAR, S_CHK_GUID, S_CHK_VOLUME,
	S_CHK_ARP, S_CHK_MAC_RANDOM, S_CHK_MAC_CUSTOM,
	S_LBL_SERIAL, S_LBL_DISK_NAME, S_LBL_FIRMWARE,
	S_LBL_VENDOR, S_LBL_VERSION, S_LBL_DATE, S_LBL_MANUFACTURER, S_LBL_PRODUCT,
	S_LBL_GPU_NAME, S_LBL_GPU_MEM, S_LBL_PERM_MAC, S_LBL_CUR_MAC,
	S_COUNT
};

static const wchar_t* const g_strings[S_COUNT][LANG_COUNT] =
{
	/* S_TITLE */               { L"Hardware Info Modifier v1.1", L"硬件信息修改器 v1.1" },
	/* S_NOTICE */              { L"Notice", L"温馨提示" },
	/* S_ERROR */               { L"Error", L"错误" },

	/* S_NO_VOLUME */           { L"No volume selected", L"未选择卷标" },
	/* S_NEED_SERIAL */         { L"Enter the new serial number", L"请输入新序列号" },
	/* S_SERIAL_OK */           { L"Volume serial number changed", L"卷标序列号修改成功" },
	/* S_SERIAL_FAIL */         { L"Failed to change volume serial number", L"卷标序列号修改失败" },
	/* S_NO_DRIVER */           { L"Driver is not connected", L"未链接驱动程序" },
	/* S_NEED_DISK_SERIAL */    { L"Enter the new disk serial number", L"请输入硬盘新序列号" },
	/* S_NEED_DISK_VALUES */    { L"Fill in all new disk values", L"请输入完整硬盘新值" },
	/* S_DRIVER_LOADED_ALREADY*/{ L"Driver is already loaded", L"不允许重复加载驱动程序" },
	/* S_NO_DRIVER_FILE */      { L"No driver file selected", L"没有选择驱动文件" },
	/* S_INSTALL_FAIL */        { L"Failed to install driver", L"安装驱动程序失败" },
	/* S_LINK_FAIL */           { L"Failed to connect to driver", L"链接驱动程序失败" },
	/* S_LINK_OK */             { L"Connected to driver", L"链接驱动程序成功" },
	/* S_UNLOAD_OK */           { L"Driver unloaded", L"卸载驱动成功" },
	/* S_NEED_BIOS */           { L"Fill in all BIOS fields", L"请填写全部BIOS信息" },
	/* S_BIOS_ONCE */           { L"This can only be applied once; a second change causes a BSOD. Continue?",
	                              L"只能修改一次,二次修改会导致蓝屏,是否继续?" },
	/* S_NEED_GPU_SERIAL */     { L"Enter the new GPU serial number", L"请输入显卡新序列号" },
	/* S_NEED_MAC */            { L"Enter both MAC addresses", L"请输入完整MAC地址" },
	/* S_UNLOAD_FIRST */        { L"Unload the driver before exiting", L"请先卸载驱动后再结束程序" },
	/* S_SYS_FILTER */          { L"Driver files (*.sys)", L"SYS文件" },
	/* S_PICK_FILE */           { L"Select a driver file", L"请选择一个文件" },

	/* S_GRP_DISK */            { L"Disk", L"硬盘" },
	/* S_GRP_BIOS */            { L"BIOS", L"BIOS" },
	/* S_GRP_GPU */             { L"GPU", L"显卡" },
	/* S_GRP_NIC */             { L"Network", L"网卡" },
	/* S_BTN_SET_SERIAL */      { L"Set serial number", L"修改序列号" },
	/* S_BTN_RANDOM_ALL */      { L"Randomize all serial numbers", L"随机化修改全部序列号" },
	/* S_BTN_LOAD */            { L"Load driver", L"加载驱动程序" },
	/* S_BTN_UNLOAD */          { L"Unload driver", L"卸载驱动程序" },
	/* S_BTN_NO_SMART */        { L"Try to disable SMART (may BSOD)", L"尝试禁用SMART(可能蓝屏)" },
	/* S_BTN_NO_HOOK */         { L"Try hook-free serial change (may BSOD)", L"尝试无HOOK修改序列号(可能蓝屏)" },
	/* S_BTN_BIOS */            { L"Apply serial/version (may BSOD)", L"随机化序列号/版本号(可能蓝屏)" },
	/* S_BTN_GPU */             { L"Set GPU serial number", L"自定义显卡序列号" },
	/* S_BTN_LANG */            { L"中文", L"English" },
	/* S_CHK_CUSTOM */          { L"Custom mode", L"自定义模式" },
	/* S_CHK_RANDOM */          { L"Random mode", L"随机化模式" },
	/* S_CHK_CLEAR */           { L"Clear-all mode", L"全清空模式" },
	/* S_CHK_GUID */            { L"Randomize disk GUID", L"随机化硬盘GUID模式" },
	/* S_CHK_VOLUME */          { L"Clear disk VOLUME", L"全清空硬盘VOLUMN模式" },
	/* S_CHK_ARP */             { L"Clear ARP table", L"全清空ARP TABLE" },
	/* S_CHK_MAC_RANDOM */      { L"Randomize all physical MACs", L"随机化全部物理MAC地址" },
	/* S_CHK_MAC_CUSTOM */      { L"Custom physical MACs", L"自定义全部物理MAC地址" },
	/* S_LBL_SERIAL */          { L"Serial:", L"序列号:" },
	/* S_LBL_DISK_NAME */       { L"Disk name:", L"硬盘名:" },
	/* S_LBL_FIRMWARE */        { L"Firmware:", L"固件值:" },
	/* S_LBL_VENDOR */          { L"Vendor:", L"供应商:" },
	/* S_LBL_VERSION */         { L"Version:", L"版本号:" },
	/* S_LBL_DATE */            { L"Date:", L"时间点:" },
	/* S_LBL_MANUFACTURER */    { L"Manufacturer:", L"制作商:" },
	/* S_LBL_PRODUCT */         { L"Product:", L"产品名:" },
	/* S_LBL_GPU_NAME */        { L"GPU name:", L"显卡名:" },
	/* S_LBL_GPU_MEM */         { L"Memory:", L"显存数:" },
	/* S_LBL_PERM_MAC */        { L"Permanent MAC:", L"物理MAC:" },
	/* S_LBL_CUR_MAC */         { L"Current MAC:", L"当前MAC:" },
};

inline Lang g_lang = LANG_EN;

inline const wchar_t* T(Str id) { return g_strings[id][g_lang]; }

// Pick the default language from the user's UI language.
inline void LangDetect()
{
	g_lang = (PRIMARYLANGID(GetUserDefaultUILanguage()) == LANG_CHINESE) ? LANG_ZH : LANG_EN;
}

struct CtrlText { int id; Str str; };

static const CtrlText g_ctrl_texts[] =
{
	{ IDC_GRP_DISK, S_GRP_DISK }, { IDC_GRP_BIOS, S_GRP_BIOS },
	{ IDC_GRP_GPU, S_GRP_GPU }, { IDC_GRP_NIC, S_GRP_NIC },
	{ IDC_BUTTON1, S_BTN_SET_SERIAL }, { IDC_BUTTON2, S_BTN_RANDOM_ALL },
	{ IDC_BUTTON3, S_BTN_LOAD }, { IDC_BUTTON4, S_BTN_UNLOAD },
	{ IDC_BUTTON5, S_BTN_NO_SMART }, { IDC_BUTTON6, S_BTN_NO_HOOK },
	{ IDC_BUTTON7, S_BTN_BIOS }, { IDC_BUTTON8, S_BTN_GPU },
	{ IDC_BUTTON_LANG, S_BTN_LANG },
	{ IDC_CHECK1, S_CHK_CUSTOM }, { IDC_CHECK2, S_CHK_RANDOM }, { IDC_CHECK3, S_CHK_CLEAR },
	{ IDC_CHECK4, S_CHK_GUID }, { IDC_CHECK5, S_CHK_VOLUME },
	{ IDC_CHECK6, S_CHK_ARP }, { IDC_CHECK7, S_CHK_MAC_RANDOM }, { IDC_CHECK8, S_CHK_MAC_CUSTOM },
	{ IDC_LBL_DISK_SERIAL, S_LBL_SERIAL }, { IDC_LBL_DISK_NAME, S_LBL_DISK_NAME },
	{ IDC_LBL_DISK_FW, S_LBL_FIRMWARE }, { IDC_LBL_HOOKFREE_SERIAL, S_LBL_SERIAL },
	{ IDC_LBL_VENDOR, S_LBL_VENDOR }, { IDC_LBL_VERSION, S_LBL_VERSION },
	{ IDC_LBL_DATE, S_LBL_DATE }, { IDC_LBL_MANUFACTURER, S_LBL_MANUFACTURER },
	{ IDC_LBL_PRODUCT, S_LBL_PRODUCT }, { IDC_LBL_BIOS_SERIAL, S_LBL_SERIAL },
	{ IDC_LBL_GPU_SERIAL, S_LBL_SERIAL }, { IDC_LBL_GPU_NAME, S_LBL_GPU_NAME },
	{ IDC_LBL_GPU_MEM, S_LBL_GPU_MEM },
	{ IDC_LBL_PERM_MAC, S_LBL_PERM_MAC }, { IDC_LBL_CUR_MAC, S_LBL_CUR_MAC },
};

// Re-apply all dialog text for the current language.
inline void LangApply(HWND hWnd)
{
	SetWindowTextW(hWnd, T(S_TITLE));
	for (const auto& c : g_ctrl_texts)
		SetWindowTextW(GetDlgItem(hWnd, c.id), T(c.str));
}
