#pragma once
#include "CDlgListBase.h"

typedef struct _IDTENTRY
{
    // USHORT == UINT16
    USHORT uOffsetLow;       //0x0���͵�ַƫ��
    USHORT uSelector;     //0x2����ѡ����

    //USHORT uAccess;      //0x4
    UINT8 uReserved;     // ����
    UINT8 GateType : 4;     // �ж�����
    UINT8 StorageSegment : 1;   // Ϊ0�����ж���
    UINT8 DPL : 2;      // ��Ȩ��
    UINT8 Present : 1;      // ��δʹ���жϿ���Ϊ0

    USHORT uOffsetHigh; //0x6   // �ߵ�ַƫ��
}IDTENTRY, * PIDTENTRY;


class CDlgIDT :
	public CDlgListBase
{
public:
	virtual BOOL OnInitDialog();
};

