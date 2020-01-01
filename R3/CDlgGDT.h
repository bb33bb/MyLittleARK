#pragma once
#include "CDlgListBase.h"

/**************************
*   GDT entry structure
*****************************/
//0x8 bytes (sizeof)
typedef struct _GDTENTRY
{
    USHORT LimitLow;                                                        //0x0
    USHORT BaseLow;                                                         //0x2
    union
    {
        struct
        {
            UCHAR BaseMid;                                                  //0x4
            UCHAR Flags1;                                                   //0x5
            UCHAR Flags2;                                                   //0x6
            UCHAR BaseHi;                                                   //0x7
        } Bytes;                                                            //0x4
        struct
        {
            ULONG BaseMid : 8;
            ULONG Type : 4;
            ULONG S : 1;
            ULONG Dpl : 2;
            ULONG Pres : 1;
            ULONG LimitHi : 4;
            ULONG Avl : 1;
            ULONG Reserved_0 : 1;
            ULONG D_B : 1;
            ULONG Granularity : 1;
            ULONG BaseHi : 8;
        } Bits;
    } HighWord;
}GDTENTRY, * PGDTENTRY;


class CDlgGDT :
	public CDlgListBase
{
public:
	virtual BOOL OnInitDialog();
};

