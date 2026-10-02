# EASY-HWID-SPOOFER

English | [中文](#中文)

A kernel-mode hardware information spoofing tool. The GUI supports English and Chinese (auto-detected from the Windows UI language, switchable with the language button).

<h1 align="center">
	<img src="demo.png" alt="">
	<br>
</h1>

## Tested systems
Win10-1909 and Win10-1903. Only the brave test on Win7.

## How it works
1. Hook the driver dispatch functions (good compatibility).

2. Locate physical memory and modify hardware data directly (weaker compatibility).

## About blue screens
1. Read and understand the kernel code.

2. Locate the crash with WinDbg and fix it.

3. Can't? Then keep learning.

## About the code
The driver code is decent, I think.

The GUI code is ugly. I probably won't maintain it for long, haha.

## Disclaimer
This code is a demo for learning. Asking whether it works against XXX anti-cheat is unrealistic.

You would do better buying a commercial HWID spoofer, which is far more likely to get past it.

Do it yourself.

---

# 中文
基于内核模式的硬件信息欺骗工具。GUI 支持中文和英文(根据系统界面语言自动选择,可用语言按钮切换)。

## 测试系统
在Win10-1909 Win10-1903,只有狠人才会在Win7上进行测试

## 工作原理
1.修改驱动程序的派遣函数(兼容性强)

2.定位到物理内存直接修改硬件数据(兼容性弱)

## 关于蓝屏
1.看懂内核代码

2.用WinDbg定位到蓝屏代码后进行修复

3.什么? 不会? 那你还要好好学哈

## 关于代码
驱动层代码应该还是看得过去的吧

但是GUI的代码是很难看,算了应该我也不会维护很久吧哈哈哈

## 最后声明
这代码更像一个Demo让大家去学习,问能用在XXX反作弊系统上么那是不现实的

那你还不如直接到外网买一个HWID SPOOFER,那大概率是能绕过XXX反作弊系统的

自己动手,丰衣足食
