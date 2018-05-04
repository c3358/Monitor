
///////////////////////////////////////////////////////////////////////////////
///

/// �˲�����Ҫ��һЩ������������ʵ��
///
/// 
///
/// 
///  .
///
///////////////////////////////////////////////////////////////////////////////


#ifdef __cplusplus
extern "C" {
#endif
#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>
#include <string.h>
#ifdef __cplusplus
}; // extern "C"
#endif

#include "tools.h"
#define  ENCRYPTFILE_HEADER_BUFFER_TAG '123'
#define  ENCRYPTFILE_BUFFER_TAG '234'
#define PAGEDCODE code_seg("PAGE")  
#define LOCKEDCODE code_seg()  



/************************************************************************/
/* ����ƥ�䴮 ����ƥ���ƫ�ƣ���ƥ�䷵��-1                                  */
/************************************************************************/
INT UnicodeStringIndexOf(UNICODE_STRING *sour,UNICODE_STRING *val)
{
	//ԭ�����ȱ�����ڱȽϴ�
	if((sour->Length)<(val->Length))
	{

		return -1;
	}

	int i;
	//ÿ���ַ���ʼ
	int sour_len=sour->Length/2;
	int val_len=val->Length/2;
	int len=sour_len-val_len+1;

	//////////////DbgPrint("sour is %wZ and val is %wZ",sour,val);
	//////////////DbgPrint("sour_len is %d and val_len is %d and len is %d",sour_len,val_len,len);
	for(i=0;i<len;i++)
	{
		//��־����ʾ��ǰƫ�ƿ�ʼ�Ƿ�ƥ��Ŀ�괮
		bool flag=true;
		for(int j=0;j<val_len;j++)
		{
			//�����ƥ�䣬���flag��Ϊfalse,��������Ƚ�;
			WCHAR c1=sour->Buffer[i+j];
			WCHAR c2=val->Buffer[j];
			//////////////DbgPrint("C1 IS %C AND C2 IS %C",c1,c2);
			if(c1!=c2)
			{
				flag=false;
				break;
			}
		}
		//flagΪtrue��֤����ǰƫ��ƥ��ɹ�����
		if(flag)
		{

			return i;
		}
	}
	return -1;
}



//�ͷŽ�������
void FreeProcessInfoList(PPROCESS_INFO head)
{
	PPROCESS_INFO tmp=NULL;
	PPROCESS_INFO next=NULL;

	tmp=head;
	while(tmp!=NULL)
	{
		next=tmp->next;
		ExFreePoolWithTag(tmp,BUFFER_SWAP_TAG) ;
		tmp=next;
	}
}
//�ͷ�һ�����Ա�
void FreeStrategy(PTYPE_KEY_PROCESS head)
{
	PTYPE_KEY_PROCESS tmp=NULL;
	PTYPE_KEY_PROCESS next=NULL;
	tmp=head;
	while(tmp!=NULL)
	{
		next=tmp->next;
		//ɾ����������
		FreeProcessInfoList(tmp->processInfo);
		ExFreePoolWithTag(tmp,BUFFER_SWAP_TAG);
		tmp=next;		
	}

}

//�жϽ������Ƿ�Ϊ�����͵Ļ��ܽ���,keyWork Ϊ����ָ��
BOOLEAN IsSecretProcess(PTYPE_KEY_PROCESS keyWord,CHAR *processName)
{
	if (keyWord==NULL||processName==NULL)
	{
		return FALSE;
	}

	PPROCESS_INFO info = keyWord->processInfo;
	
	while(info!=NULL)
	{
		KdPrint(("��ǰ����processName=%s\n  ���Ա��еĽ��� info->processName=%s\n",processName,info->processName));
		/*if(strncmp(processName,info->processName,strlen(processName))==0)
		{
		return TRUE;
		}*/
		info=info->next;
	}
	return FALSE;
}

/************************************************************************/
/*    ��ȡ��������ƫ��                                                 */
/************************************************************************/
//////////////////////////////////////////////////////////////////////////
//��ȡ��������


ULONG 
GetProcessNameOffset(
    VOID
    )
{
    PEPROCESS       curproc;
    ULONG             i;
 
    curproc = PsGetCurrentProcess();
 
    //
    // Scan for 12KB, hopping the KPEB never grows that big!
    //
    for( i = 0; i < 3*PAGE_SIZE; i++ ) 
	{
     
        if( !strncmp( "System", (PCHAR) curproc + i, strlen("System") )) 
		{

            return i;
        }
    }
    //
    // Name not found - oh, well
    //
    return 0;
}
 

//2017.9.12
PCHAR
GetCurrentProcessName(ULONG ProcessNameOffset)
{
    PEPROCESS       curproc;
    char            *nameptr;
    ULONG           i;
 
    //
    // We only try and get the name if we located the name offset
    //
    if( ProcessNameOffset ) {
    
        //
        // Get a pointer to the current process block
        //
        curproc = PsGetCurrentProcess();
 
        //
        // Dig into it to extract the name. Make sure to leave enough room
        // in the buffer for the appended process ID.
        //
        nameptr   = (PCHAR) curproc + ProcessNameOffset;
		/*
		#if defined(_M_IA64)
        sprintf( szName + strlen(szName), ":%I64d", PsGetCurrentProcessId());
		#else
        sprintf( szName + strlen(szName), ":%d", (ULONG) PsGetCurrentProcessId());
		#endif
		//*/
 
    } else {
		
       nameptr="";
    }

	//KdPrind(("GetCurrentProcessName������ = %s\n",nameptr));
    return nameptr;
}


/************************************************************************/
/*            �ַ�������    chat * ��wchar_t* ��ת  2017.9.11    */
/************************************************************************/

void wstr2cstr(const wchar_t *pwstr , char *pcstr, size_t len)
{     
	for (size_t i=0;i<len;i++)
	{
		pcstr[i*2]=(char)(pwstr[i]>>8);//��λ
		pcstr[i*2+1]=(char)(pwstr[i]);//��λ�ض�
	}
}    

void cstr2wstr( const char *pcstr,wchar_t *pwstr , size_t len)
{     
	for(size_t i=0;i<len;i++)
	{
		pwstr[i]=(WCHAR)pcstr[i];
	}
}  




/** ��ȡ�ļ���·�������ͣ������ļ��У�ȥ����**/

NTSTATUS GetFileInformation(__inout PFLT_CALLBACK_DATA Data,
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__inout PSTREAM_HANDLE_CONTEXT ctx)
{
	NTSTATUS status;
	PFLT_FILE_NAME_INFORMATION nameInfo = NULL;
	//����жϼ�
	if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
	{
		return STATUS_UNSUCCESSFUL;
	}
	//�ж��Ƿ��ļ���
	BOOLEAN isDir;	
	status = FltIsDirectory(FltObjects->FileObject,FltObjects->Instance,&isDir);


	if (NT_SUCCESS(status))
	{
		//�ļ���ֱ������
		if (isDir)
		{
			return STATUS_UNSUCCESSFUL;  //���ļ���ֱ�ӷ���ʧ��
		}
		else
		{
			//��ȡ�ļ�����FLT_FILE_NAME_OPENED �ļ����򿪲���
			status=FltGetFileNameInformation(Data,
				FLT_FILE_NAME_NORMALIZED|FLT_FILE_NAME_QUERY_ALWAYS_ALLOW_CACHE_LOOKUP,
				&nameInfo);

			if (NT_SUCCESS(status))
			{
					 
				status = FltParseFileNameInformation(nameInfo);
				
				if (NT_SUCCESS(status))
				{
					ctx->fileFullPath = nameInfo->Name;
					ctx->fileStyle = nameInfo->Extension;
					ctx->fileVolumeName = nameInfo->Volume;
					ctx->fileName  = nameInfo->ParentDir;
				}    
			}
		}

		if (NULL!= nameInfo)
		{
			FltReleaseFileNameInformation(nameInfo);
		}
	}
	

	return status;

}




 ULONG	GetTime()
{
	LARGE_INTEGER TickCount = {0};
	LARGE_INTEGER GelinTime = {0};
	LARGE_INTEGER LocalTime = {0};
	TIME_FIELDS NowFields;
	ULONG Inc = 0;
	ULONG Day = 0;
	ULONG Hour = 0;
	ULONG Minute = 0;
	ULONG Second = 0;

	Inc = KeQueryTimeIncrement();
	KeQueryTickCount(&TickCount);
	TickCount.QuadPart *= Inc;
	TickCount.QuadPart /= 10000;
	Day = TickCount.QuadPart / (1000*60*60*24);
	Hour = TickCount.QuadPart % (1000*60*60*24) / (1000*60*60);
	Minute = TickCount.QuadPart % (1000*60*60*24) % (1000*60*60) / (1000*60);
	Second = TickCount.QuadPart % (1000*60*60*24) % (1000*60*60) % (1000*60) / 1000;

	//KdPrint(("ϵͳ������%2d��%2dСʱ%2d����%2d��\n", Day, Hour, Minute, Second));

	//ULONG Time[1] = {0};
//	Time[0] = Hour*3600+Minute*60+Second;
	//KdPrint(("ϵͳ���� %2ld��\n", Time[0]));
	return  (Hour*3600+Minute*60+Second);

	/*KeQuerySystemTime(&GelinTime);
	ExSystemTimeToLocalTime(&GelinTime, &LocalTime);
	RtlTimeToTimeFields(&LocalTime, &NowFields);
	KdPrint(("ϵͳ��ǰʱ�� : %4d��%2d��%2d�� %2d:%2d:%2d\n",
		NowFields.Year, 
		NowFields.Month,
		NowFields.Day,
		NowFields.Hour,
		NowFields.Minute,
		NowFields.Second));*/
}






 //
 // Enable log event: for synchronization
 //
 static KEVENT   gs_eventEnableKeLog;

 //----------------------------------------------------------------------
 //
 // initialization interface
 //
 //----------------------------------------------------------------------
 //
 // initialize the global data structures, when the driver is loading. 
 // (Call in DriverEntry())
 //
 NTSTATUS
	 Dbg_LoadInit()
 {
	 // Initialize the event
	 KeInitializeEvent(&gs_eventEnableKeLog, SynchronizationEvent, TRUE);
	 return STATUS_SUCCESS;
 }

 static void WaitForWriteMutex()
 {
	 // Wait for enable log event
	 KeWaitForSingleObject(&gs_eventEnableKeLog, Executive, KernelMode, TRUE, 0);
	 KeClearEvent(&gs_eventEnableKeLog);
 }
 static void ReleaseWriteMutex()
 {
	 // Set enable log event
	 KeSetEvent(&gs_eventEnableKeLog, 0, FALSE);
 }
 //----------------------------------------------------------------------
 //
 // DbgKeLog
 //
 // Trace to file.
 //
 //----------------------------------------------------------------------
 VOID DbgKeLog(PCHAR lpszLog)
 {
	 if (KeGetCurrentIrql() > PASSIVE_LEVEL)
	 {
		// TOKdPrint(("TKeHook: KeLog: IRQL too hight.../n"));
		 KdPrint(("TKeHook: KeLog: IRQL too hight.../n"));
		 return ;
	 }
	 WaitForWriteMutex();

	 KdPrint(("%s", lpszLog));

	 __try
	 {
		 IO_STATUS_BLOCK  IoStatus;
		 OBJECT_ATTRIBUTES objectAttributes;
		 NTSTATUS status;
		 HANDLE FileHandle = NULL;
		 UNICODE_STRING fileName;
		 RtlInitUnicodeString(&fileName,L"//??//C://KeLog.log") ;
	

		 InitializeObjectAttributes (&objectAttributes,
			 (PUNICODE_STRING)&fileName,
			 OBJ_CASE_INSENSITIVE,  //�Դ�Сд����
			 NULL,
			 NULL );

		 status = ZwCreateFile(&FileHandle,
			 FILE_APPEND_DATA, //���������
			 &objectAttributes,
			 &IoStatus,
			 0, 
			 FILE_ATTRIBUTE_NORMAL,
			 FILE_SHARE_WRITE,
			 FILE_OPEN_IF, //��ʹ����Ҳ����
			 FILE_SYNCHRONOUS_IO_NONALERT,
			 NULL,     
			 0 );



			 ZwWriteFile(FileHandle,
				 NULL,
				 NULL,
				 NULL,
				 &IoStatus,
				 lpszLog,
				 strlen(lpszLog),
				 NULL,
				 NULL 
				 );

			 ZwClose(FileHandle);
		


		 ReleaseWriteMutex();
		 return ;
	 }
	 __except(EXCEPTION_EXECUTE_HANDLER)
	 {
		 ReleaseWriteMutex();
		 KdPrint(("TKeHook: DbgKeLog() except: %0xd !!/n", GetExceptionCode()));
		 return ;
	 }
 }