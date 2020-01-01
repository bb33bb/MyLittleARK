#pragma once


/************************
*   Process
**************************/
//0x2c0 bytes (sizeof)
typedef struct _EPROCESS
{
    char Pcb[0xb8];                                                   //0x0
    struct _LIST_ENTRY ActiveProcessLinks;                                  //0xb8
    char ProcessQuotaUsage[0xc8];                                             //0xc0
    struct _LIST_ENTRY ThreadListHead;                                      //0x188
    char SecurityPort[0x130];                                                     //0x190
}MY_EPROCESS, * P_MY_EPROCESS;


struct _PEB
{
    UCHAR InheritedAddressSpace;                                            //0x0
    UCHAR ReadImageFileExecOptions;                                         //0x1
    UCHAR BeingDebugged;
    UCHAR BitField;
    void* Mutant;                                                           //0x4
    void* ImageBaseAddress;                                                 //0x8
    struct _PEB_LDR_DATA* Ldr;
};
//0x30 bytes (sizeof)
struct _PEB_LDR_DATA
{
    ULONG Length;                                                           //0x0
    UCHAR Initialized;                                                      //0x4
    VOID* SsHandle;                                                         //0x8
    struct _LIST_ENTRY InLoadOrderModuleList;                               //0xc
    struct _LIST_ENTRY InMemoryOrderModuleList;                             //0x14
    struct _LIST_ENTRY InInitializationOrderModuleList;                     //0x1c
    VOID* EntryInProgress;                                                  //0x24
    UCHAR ShutdownInProgress;                                               //0x28
    VOID* ShutdownThreadId;                                                 //0x2c
};
typedef struct _MY_LDR_DATA_TABLE_ENTRY {
    LIST_ENTRY InLoadOrderLinks;    //˫������
    LIST_ENTRY InMemoryOrderLinks;
    LIST_ENTRY InInitializationOrderLinks;
    PVOID DllBase;
    PVOID EntryPoint;
    ULONG SizeOfImage;
    UNICODE_STRING FullDllName;
    UNICODE_STRING BaseDllName;
    ULONG Flags;
    USHORT LoadCount;
    USHORT TlsIndex;
    union {
        LIST_ENTRY HashLinks;
        struct {
            PVOID SectionPointer;
            ULONG CheckSum;
        }s1;
    }u1;
    union {
        struct {
            ULONG TimeDateStamp;
        }s2;
        struct {
            PVOID LoadedImports;
        }s3;
    }u2;
}MY_LDR_DATA_TABLE_ENTRY, * P_MY_LDR_DATA_TABLE_ENTRY;

/************************
*   ETHREAD
**************************/
//0x2b8 bytes (sizeof)
typedef struct _ETHREAD
{
    char Tcb[0x268];                                                    //0x0
    struct _LIST_ENTRY ThreadListEntry;                                     //0x268
    char RundownProtect[0x48];                                  //0x270
}MY_ETHREAD, * P_MY_ETHREAD;

/************************
*   IDT 
**************************/

typedef struct _IDT_INFO {
    UINT16 uIdtLimit;   // IDT��Χ
    UINT16 uLowIdtBase;   // IDT�ͻ�ַ
    UINT16 uHighIdtBase;   // IDT�߻�ַ
}IDT_INFO, * PIDT_INFO;
//0x8 bytes (sizeof)
typedef struct _IDTENTRY
{
    // USHORT == UINT16
    USHORT uOffsetLow;       //0x0���͵�ַƫ��
    USHORT uSelector;     //0x2����ѡ����

    USHORT uAccess;      //0x4
    //UINT8 uReserved;     // ����
    //UINT8 GateType : 4;     // �ж�����
    //UINT8 StorageSegment : 1;   // Ϊ0�����ж���
    //UINT8 DPL : 2;      // ��Ȩ��
    //UINT8 Present : 1;      // ��δʹ���жϿ���Ϊ0

    USHORT uOffsetHigh; //0x6   // �ߵ�ַƫ��
}IDTENTRY, * PIDTENTRY;

/************************
*   GDT
**************************/
typedef struct _GDT_INFO {
    UINT16 uGdtLimit;
    UINT16 uLowGdtBase;
    UINT16 uHighGdtBase;
}GDT_INFO, * PGDT_INFO;

//0x8 bytes (sizeof)
typedef struct _GDTENTRY
{
    USHORT LimitLow;                                                        //0x0
    USHORT BaseLow;                                                         //0x2
    struct
    {
        UCHAR BaseMid;                                                  //0x4
        UCHAR Flags1;                                                   //0x5
        UCHAR Flags2;                                                   //0x6
        UCHAR BaseHi;                                                   //0x7
    } HighWord;
}GDTENTRY, * PGDTENTRY;





/************************
*   SSDT
**************************/

typedef struct _SSDT
{
    PULONG ServiceTableBase;	//������ַ����׵�ַ
    PULONG ServiceCounterTableBase;	// ��������ÿ�����������õĴ���
    ULONG  NumberOfService;		// �������ĸ���, NumberOfService * 4 ����������ַ��Ĵ�С
    UCHAR* ParamTableBase;	// �����������׵�ַ
}SSDT, * PSSDT;

typedef struct _SSDTDescriptor
{
    SSDT	ntoskrnl;		// ntoskrnl.exe�ķ���������SSDT
    SSDT	win32k;			// ShadowSSDT
    SSDT	reserve1;
    SSDT	reserve2;
} SSDTDescriptor, * PSSDTDescriptor;