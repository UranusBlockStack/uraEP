#pragma once

// ****************************************ERROR CODE***********************************************
#define E_UNINIT			MAKE_MY_ERRCODE(0x4001)							// 接口未初始化
#define E_DISCONNECTED		MAKE_WINDOWS_ERRCODE(ERROR_PIPE_NOT_CONNECTED)	// 接口连接已断开
#define E_REFUSE_APP		MAKE_MY_ERRCODE(0x4100)							// 应用名称被拒绝
#define E_REFUSE_STATE		MAKE_MY_ERRCODE(0x4101)							// 当前状态拒绝此操作

//		E_INVALIDARG														// 参数无效
//		E_FAIL																// 其他未知错误
//		
//		S_OK																// 执行正确
//		S_FALSE																// 执行了重复的操作
#define S_IO_PENDING		 ERROR_IO_PENDING								// 异步操作还没有完成
// ****************************************ERROR CODE END*******************************************

enum CROSSAPP_STATUS
{
	error			= -1,
	none			= 0,
	initialized		= 1,
	connecting		= 2,
	working			= 3,
	closing			= 4,
};

interface ICTEPAppProtocolCrossCallBack
{
	_VIRT(CROSSAPP_STATUS) GetStatus() = 0;	// 获取当前链接的状态
	_VIRT_H		Initialize(LPCSTR sAppName, DWORD SessionId = (DWORD)-1) = 0;		// 初始化获取
	_VIRT_H		Connect() = 0;						//创建一个当前用户当前应用的通道(动态或者静态)
	_VIRT_V		Disconnect() = 0;

	_VIRT_H		Write(__in char* buff, __in DWORD size) = 0;

	// Read会返回读到的包, 处理完成后需要调用FreeBuff释放对应内存,赋值pOL会返回S_IO_PENDING, pOL为空时会一直等待到有数据才返回
	_VIRT_H		Read(__out char** buff, __out DWORD* dwRead, HANDLE* pHandleEvent = nullptr) = 0;
	_VIRT_H		FreeBuff(__in char* buff) = 0;
};

ICTEPAppProtocolCrossCallBack* CrossAppGetInstance();
void CrossAppRelease(ICTEPAppProtocolCrossCallBack* pi);






























