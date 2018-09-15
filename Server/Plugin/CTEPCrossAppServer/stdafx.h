// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             //  从 Windows 头文件中排除极少使用的信息
// Windows 头文件:
#include <windows.h>


#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // 某些 CString 构造函数将是显式的

#include <atlbase.h>
#include <atlstr.h>

// TODO: 在此处引用程序需要的其他头文件
#pragma warning(push, 4)
#pragma warning(disable:4200)// 使用了非标准扩展 : 结构/联合中的零大小数组, 当UDT包含大小为零的数组时，无法生成复制构造函数或副本赋值运算符
#pragma warning(disable:4481)// 使用了非标准扩展: 请重写说明符“override”
#pragma warning(disable:4482)// 使用了非标准扩展: 限定名中使用了枚举“EmPacketOperationType”

#include "Log4Cpp_Lib/log4cpplib.h"
using namespace _Log4Cpp;

#include "CommonInclude/CommonImpl.h"



