#pragma once
#include "CDlgListBase.h"

typedef struct _IDTENTRY
{
    // USHORT == UINT16
    USHORT uOffsetLow;       //0x0，低地址偏移
    USHORT uSelector;     //0x2，段选择器

    //USHORT uAccess;      //0x4
    UINT8 uReserved;     // 保留
    UINT8 GateType : 4;     // 中断类型
    UINT8 StorageSegment : 1;   // 为0则是中断门
    UINT8 DPL : 2;      // 特权级
    UINT8 Present : 1;      // 如未使用中断可置为0

    USHORT uOffsetHigh; //0x6   // 高地址偏移
}IDTENTRY, * PIDTENTRY;


class CDlgIDT :
	public CDlgListBase
{
public:
	virtual BOOL OnInitDialog();
};

