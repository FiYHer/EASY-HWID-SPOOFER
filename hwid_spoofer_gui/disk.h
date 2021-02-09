#pragma once
#include "serial.h"

#include <vector>
#include <map>
#include <string>

// 修改卷序列号
bool change_serial_number(char drive, DWORD serial);

// 获取全部硬盘卷和序列号
std::map<char, unsigned long> get_disk_all_drive_serial();
