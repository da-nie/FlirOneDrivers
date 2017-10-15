#include <stdarg.h>
#include "ntddk.h"
#include "wdf.h"
#include "prototypes.h"
#include "usbdi.h"
#include "wdfusb.h"
#include "initguid.h"
#include "define.h"

extern char DriverName[];//��� ��������

//��������� ��������� ���������� (��������� ���� ����� ���������� ��������� � ������� ������ � ���� �� ��������)
typedef struct _DEVICE_CONTEXT 
{
 WDFUSBDEVICE UsbDevice;//������������� ����������
 WDFUSBINTERFACE UsbInterface;//������������� ����������
 WDFUSBPIPE BulkReadPipe;//����� �� ������
 WDFUSBPIPE BulkWritePipe;//����� �� ������
} DEVICE_CONTEXT,*PDEVICE_CONTEXT;
 
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT,GetDeviceContext)
 
//----------------------------------------------------------------------------------------------------
//�������, ���������� ��� �������� ��������
//DriverObject - ��������� �� ������-�������, ��������������� ������������ ��������. 
//RegistryPath - ��������� �� ������ � ������� Unicode � ������ ����� �������, ���������������� ������������ ��������.
//----------------------------------------------------------------------------------------------------
NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject,IN PUNICODE_STRING RegistryPath)
{
 NTSTATUS status;//������������ ��������
 WDF_DRIVER_CONFIG config;//������������ ��������
 WDF_OBJECT_ATTRIBUTES attributes;//��������� ����������
 
 KdPrint(("%s: DriverEntry.\n",DriverName));//������� ��������� � ������� ��������� 
 WDF_DRIVER_CONFIG_INIT(&config,EvtDeviceAdd);//�������������� ����������� ������������
  
 //�������������� ��������
 WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
 
 //������ ������� ����������
 status=WdfDriverCreate(DriverObject,RegistryPath,&attributes,&config,WDF_NO_HANDLE);
 if (!NT_SUCCESS(status))//��������� ������
 {
  KdPrint(("%s: WdfDriverCreate failed 0x%x\n",DriverName,status));
  return(status);
 } 
 return(status);
}
//----------------------------------------------------------------------------------------------------
//�������, ���������� ��� ���������� ����������
//---------------------------------------------------------------------------------------------------- 
NTSTATUS EvtDeviceAdd(IN WDFDRIVER Driver,IN PWDFDEVICE_INIT DeviceInit)
{
 NTSTATUS status;//������������ ��������
 WDFDEVICE device;//����������
 WDF_OBJECT_ATTRIBUTES attributes;//�������� ���������� (��� ����� �������� ����������)
 PDEVICE_CONTEXT pDevContext;//�������� ����������
 WDF_PNPPOWER_EVENT_CALLBACKS pnpPowerCallbacks;//������� ����������� ���������
 WDF_IO_QUEUE_CONFIG ioQueueConfig;//������������ ������� ��������� ��������
  
 UNREFERENCED_PARAMETER(Driver);//������, ����� ���������� �� ������� �� �������������� ��������
  
 KdPrint(("%s: EvtDeviceAdd\n",DriverName));//������� ��������� � ������� ��������� 
  
 //������������ ������� ���������� ���������� ����� �����������
 WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);
 pnpPowerCallbacks.EvtDevicePrepareHardware=EvtDevicePrepareHardware;//����� ������� ��������� ����������
 WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit,&pnpPowerCallbacks);//����������� � � �������
 WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes,DEVICE_CONTEXT);//��������������� �������� ����������
  
 status=WdfDeviceCreate(&DeviceInit,&attributes,&device);//������ ����������
 if (!NT_SUCCESS(status))//��������� ������
 {
  KdPrint(("%s: WdfDeviceCreate failed 0x%x\n",DriverName,status));
  return(status);
 }
  
 pDevContext=GetDeviceContext(device);//�������� �������� ����������
 //������ ��������� ��� ������ � ����������� (����� ���� ��������� ����� �������� � ���������)
 status=WdfDeviceCreateDeviceInterface(device,(LPGUID)&GUID_DEVINTERFACE_FLIRONEGEN2,NULL);
 if (!NT_SUCCESS(status))//��������� ������
 {
  KdPrint(("%s: WdfDeviceCreateDeviceInterface failed 0x%x\n",DriverName,status));
  return(status);
 }
 //���������� ����������� ������ � ������ ��� ������� �������
 WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&ioQueueConfig,WdfIoQueueDispatchParallel);
 ioQueueConfig.EvtIoDeviceControl=EvtIoDeviceControl;
 ioQueueConfig.EvtIoRead=EvtIoRead;
 ioQueueConfig.EvtIoWrite=EvtIoWrite;
  
 status=WdfIoQueueCreate(device,&ioQueueConfig,WDF_NO_OBJECT_ATTRIBUTES,WDF_NO_HANDLE);
 if (!NT_SUCCESS(status))//��������� ������
 {
  KdPrint(("%s: WdfIoQueueCreate failed 0x%x\n",DriverName,status));
  return(status);
 }  
 return(status);
}
 
//----------------------------------------------------------------------------------------------------
//�������, ���������� ��� ��������� ����������
//----------------------------------------------------------------------------------------------------
NTSTATUS EvtDevicePrepareHardware(IN WDFDEVICE Device,IN WDFCMRESLIST ResourceList,IN WDFCMRESLIST ResourceListTranslated)
{
 long n;
 NTSTATUS status;//������������ ��������
 PDEVICE_CONTEXT pDeviceContext;//�������� ����������
 WDF_USB_DEVICE_SELECT_CONFIG_PARAMS configParams;//��������� ����������
 UCHAR numInterfaces;
 WDFUSBPIPE pipe;//�����
 UCHAR ep_index=0;//����� �������� �����
 WDF_USB_PIPE_INFORMATION pipeConfig;//��������� ������
  
 UNREFERENCED_PARAMETER(ResourceList);//������, ����� ���������� �� ������� �� �������������� ��������
 UNREFERENCED_PARAMETER(ResourceListTranslated);//������, ����� ���������� �� ������� �� �������������� ��������
  
 KdPrint(("%s: EvtDevicePrepareHardware\n",DriverName)); 
  
 pDeviceContext=GetDeviceContext(Device);//�������� �������� ����������
  
 //������ ������ USB-����������, ���� �� ��� �� ��� ������
 if (pDeviceContext->UsbDevice==NULL)//��� �� ��������� USB-����������
 {
  status=WdfUsbTargetDeviceCreate(Device,WDF_NO_OBJECT_ATTRIBUTES,&pDeviceContext->UsbDevice);
  if (!NT_SUCCESS(status))//��������� ������
  {
   KdPrint(("%s: WdfUsbTargetDeviceCreate failed 0x%x\n",DriverName,status)); 
   return(status);
  }
 }  
 //�������������� ��� ���� ���������  
 numInterfaces=WdfUsbTargetDeviceGetNumInterfaces(pDeviceContext->UsbDevice);
 KdPrint(("%s: Interfaces:0x%02x.\n",DriverName,numInterfaces));
  
 WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_SINGLE_INTERFACE(&configParams);
    
 //�������� ������ ������������ ��� ���������� USB
 status=WdfUsbTargetDeviceSelectConfig(pDeviceContext->UsbDevice,WDF_NO_OBJECT_ATTRIBUTES,&configParams);
 if(!NT_SUCCESS(status))//��������� ������
 {
  KdPrint(("%s: WdfUsbTargetDeviceSelectConfig failed 0x%x\n",DriverName,status));
  return(status);
 }
 pDeviceContext->UsbInterface=configParams.Types.SingleInterface.ConfiguredUsbInterface;
 //�������� 2 ������: �� ������ � �� ������
 pDeviceContext->BulkReadPipe=NULL;
 pDeviceContext->BulkWritePipe=NULL;
 WDF_USB_PIPE_INFORMATION_INIT(&pipeConfig);
 while(1)
 {
  pipe=WdfUsbInterfaceGetConfiguredPipe(pDeviceContext->UsbInterface,ep_index,&pipeConfig);
  if (pipe==NULL) break;//������ �������� ����� ���
  KdPrint(("FlirOneGen2_iAP: EndPoint addr:0x%02x\n",pipeConfig.EndpointAddress));
  if (pipeConfig.EndpointAddress==BULK_READ_ENDPOINT_ADDR) pDeviceContext->BulkReadPipe=pipe;
  if (pipeConfig.EndpointAddress==BULK_WRITE_ENDPOINT_ADDR) pDeviceContext->BulkWritePipe=pipe;
  ep_index++;
 }
  
 //����� �� ������
 if (pDeviceContext->BulkReadPipe!=NULL)
 {
  KdPrint(("%s: BulkReadPipe is OK.\n",DriverName));
  WdfUsbTargetPipeSetNoMaximumPacketSizeCheck(pDeviceContext->BulkReadPipe);//��������� �������� �� ������������ ������ ������ 
 }
 else KdPrint(("%s: BulkReadPipe is NULL!\n",DriverName));
  
 //����� �� ������
 if (pDeviceContext->BulkWritePipe!=NULL)
 {
  KdPrint(("%s: BulkWritePipe is OK.\n",DriverName));
  WdfUsbTargetPipeSetNoMaximumPacketSizeCheck(pDeviceContext->BulkWritePipe);//��������� �������� �� ������������ ������ ������
 }
 else KdPrint(("%s BulkWritePipe is NULL!\n",DriverName));
 return(status);
}
 
//----------------------------------------------------------------------------------------------------
//�������, ���������� ��� DeviceIoControl
//----------------------------------------------------------------------------------------------------
VOID EvtIoDeviceControl(IN WDFQUEUE Queue,IN WDFREQUEST Request,IN size_t OutputBufferLength, IN size_t InputBufferLength,IN ULONG IoControlCode)
{
 WDFDEVICE device;//����������
 PDEVICE_CONTEXT pDeviceContext;//�������� ����������
 NTSTATUS status;//������������ ��������
 WDF_USB_CONTROL_SETUP_PACKET controlSetupPacket;//����������� �����
 WDF_MEMORY_DESCRIPTOR memDesc;//���������� ������� ������
 WDFMEMORY memory;//������� ������
 WDF_REQUEST_SEND_OPTIONS sendOptions;//����� ��������� ������������ ������
 size_t bytesTransferred=0;//���������� ���������� ����
 unsigned char data[2]={0,0};
 USHORT value=0;
 USHORT index=0;
 unsigned char ok=0;
 long n;
 unsigned char *ptr;
  
 UNREFERENCED_PARAMETER(InputBufferLength);
 UNREFERENCED_PARAMETER(OutputBufferLength);
  
 KdPrint(("%s: EvtIoDeviceControl.\n",DriverName));//������� ��������� � ������� ��������� 
  
 status=STATUS_INVALID_DEVICE_REQUEST;//������ �� ������
  
 device=WdfIoQueueGetDevice(Queue);
 pDeviceContext=GetDeviceContext(device);
  
 if (pDeviceContext->UsbDevice==NULL)
 {
  KdPrint(("%s: USBDevice=NULL!\n",DriverName));//������� ��������� � ������� ��������� 
  WdfRequestCompleteWithInformation(Request,status,bytesTransferred);
  return;
 }

 if (IoControlCode==IOCTL_FLIR_ONE_GEN2_FRAME_STOP)//���������� ����������
 {
  KdPrint(("%s: IOCTL_FLIR_ONE_GEN2_FRAME_STOP.\n",DriverName));//������� ��������� � ������� ��������� 
  value=0;
  index=2;
  ok=1;
 }
 if (IoControlCode==IOCTL_FLIR_ONE_GEN2_FRAME_START)//��������� ����������
 {
  KdPrint(("%s: IOCTL_FLIR_ONE_GEN2_FRAME_START.\n",DriverName));//������� ��������� � ������� ��������� 
  value=1;
  index=2;
  ok=1;
 }
 if (IoControlCode==IOCTL_FLIR_ONE_GEN2_FILEIO_STOP)//���������� ����������
 {
  KdPrint(("%s: IOCTL_FLIR_ONE_GEN2_FILEIO_STOP.\n",DriverName));//������� ��������� � ������� ��������� 
  value=0;
  index=1;
  ok=1;
 }
 if (IoControlCode==IOCTL_FLIR_ONE_GEN2_FILEIO_START)//��������� ����������
 {
  KdPrint(("%s: IOCTL_FLIR_ONE_GEN2_FILEIO_START.\n",DriverName));//������� ��������� � ������� ��������� 
  value=1;
  index=1;
  ok=1;
 }
 if (ok==1) 
 {
  //��� ����������� ��������� � ������� �������� �����
  //����������� ������� ������
  status=WdfRequestRetrieveInputMemory(Request,&memory);
  if (!NT_SUCCESS(status)) 
  {
   KdPrint(("%s: WdfRequestRetrieveMemory failed 0x%x",DriverName,status));
   WdfRequestCompleteWithInformation(Request,status,bytesTransferred);
   return;
  }
  //����� ������ ��������� ������������ ����������
   
  /* Flir config
  01 0b 01 00 01 00 00 00 c4 d5
  0 bmRequestType = 01
  1 bRequest = 0b
  2 wValue 0001 type (H) index (L)    stop=0/start=1 (Alternate Setting)
  4 wIndex 01                         interface 1/2
  5 wLength 00
  6 Data 00 00
  */
  
  WDF_USB_CONTROL_SETUP_PACKET_INIT(&controlSetupPacket,BmRequestHostToDevice,BmRequestToInterface,USB_REQUEST_SET_INTERFACE,value,index);  
  KdPrint(("%s: Send: ",DriverName));
  ptr=(unsigned char*)(&controlSetupPacket);
  for(n=0;n<sizeof(controlSetupPacket);n++)
  {
   KdPrint(("0x%02x ",ptr[n]));
  }
  KdPrint(("\n"));  
   
  WDF_MEMORY_DESCRIPTOR_INIT_HANDLE(&memDesc,memory,NULL);
  WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&memDesc,data,0);//������ �� �������
   
  //����������� ��������� ���������
  WDF_REQUEST_SEND_OPTIONS_INIT(&sendOptions,WDF_REQUEST_SEND_OPTION_TIMEOUT);
  WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&sendOptions,WDF_REL_TIMEOUT_IN_MS(200));  
   
  status=WdfUsbTargetDeviceSendControlTransferSynchronously(pDeviceContext->UsbDevice,WDF_NO_HANDLE,&sendOptions,&controlSetupPacket,&memDesc,(PULONG)&bytesTransferred);
  if (!NT_SUCCESS(status)) 
  {
   KdPrint(("%s: SendControlTransfer failed 0x%x",DriverName,status));
   WdfRequestCompleteWithInformation(Request,status,bytesTransferred);
   return;
  }
  KdPrint(("%s: Ok. Transferred:0x%02x\n",DriverName,bytesTransferred));//������� ��������� � ������� ��������� 
 }
 WdfRequestCompleteWithInformation(Request,status,bytesTransferred);
 return;
}
 
//----------------------------------------------------------------------------------------------------
//���������� ������� ������
//Queue-�������.
//Request-��������� �������.
//Length-���������� ���� ��� ������
//----------------------------------------------------------------------------------------------------
VOID EvtIoRead(IN WDFQUEUE Queue,IN WDFREQUEST Request,IN size_t Length)
{
 NTSTATUS status;//������������ ��������
 WDFUSBPIPE pipe;//�����
 WDFMEMORY reqMemory;
 PDEVICE_CONTEXT pDeviceContext;//�������� ����������
 BOOLEAN ret;
  
 UNREFERENCED_PARAMETER(Length);//������, ����� ���������� �� ������� �� �������������� ��������
  
 KdPrint(("%s: EvtIoRead.\n",DriverName));//������� ��������� � ������� ��������� 
  
 pDeviceContext=GetDeviceContext(WdfIoQueueGetDevice(Queue));//�������� �������� ����������

 if (pDeviceContext->UsbDevice==NULL)
 {
  status=STATUS_INVALID_DEVICE_REQUEST;//������ �� ������
  KdPrint(("%s: USBDevice==NULL!\n",DriverName));//������� ��������� � ������� ��������� 
  WdfRequestCompleteWithInformation(Request,status,0);
  return;
 }
 
 pipe=pDeviceContext->BulkReadPipe;//���� ����� ��� ������
 if (pipe==NULL)
 {
  KdPrint(("%s: Pipe is NULL!\n",DriverName));//������� ��������� � ������� ��������� 
  status=STATUS_SUCCESS;
  WdfRequestCompleteWithInformation(Request,status,0);//���������� �������
  return;
 }
  
 status=WdfRequestRetrieveOutputMemory(Request,&reqMemory);
 if (!NT_SUCCESS(status))//��������� ������
 {
  KdPrint(("%s: WdfRequestRetrieveOutputMemory failed 0x%x",DriverName,status));
  WdfRequestCompleteWithInformation(Request,status,0);//���������� �������
  return;
 }
 //�������� ������ �� ������
 status=WdfUsbTargetPipeFormatRequestForRead(pipe,Request,reqMemory,NULL);
 if (!NT_SUCCESS(status))//��������� ������
 {
  KdPrint(("FlirOneGen2_iAP: WdfUsbTargetPipeFormatRequestForRead failed 0x%x", status));
  WdfRequestCompleteWithInformation(Request,status,0);//���������� �������
  return;
 }
 //������ ����� �����������
 WdfRequestSetCompletionRoutine(Request,EvtRequestReadCompletionRoutine,pipe);
  
 ret=WdfRequestSend(Request,WdfUsbTargetPipeGetIoTarget(pipe),WDF_NO_SEND_OPTIONS);
 if (ret==FALSE) 
 {
  KdPrint(("%s: WdfRequestSend failed 0x%x",DriverName,status));
  status=WdfRequestGetStatus(Request);
  WdfRequestCompleteWithInformation(Request,status,0);//���������� �������
  return; 
 } 
 else return;
 KdPrint(("%s: Ok.",DriverName));
 WdfRequestCompleteWithInformation(Request,status,0);//���������� �������
 return;
}
 
//----------------------------------------------------------------------------------------------------
//���������� ������ ���������
//----------------------------------------------------------------------------------------------------
VOID EvtRequestReadCompletionRoutine(IN WDFREQUEST Request,IN WDFIOTARGET Target,PWDF_REQUEST_COMPLETION_PARAMS CompletionParams,IN WDFCONTEXT Context)
{
 NTSTATUS status;//������������ ���������
 size_t bytesRead=0;//���������� ��������� ����
 PWDF_USB_REQUEST_COMPLETION_PARAMS usbCompletionParams;
  
 UNREFERENCED_PARAMETER(Target);//������, ����� ���������� �� ������� �� �������������� ��������
 UNREFERENCED_PARAMETER(Context);//������, ����� ���������� �� ������� �� �������������� ��������
  
 status=CompletionParams->IoStatus.Status;
  
 usbCompletionParams=CompletionParams->Parameters.Usb.Completion;
 bytesRead=usbCompletionParams->Parameters.PipeRead.Length;
  
 if (NT_SUCCESS(status))//������ ��������� �������
 {
  KdPrint(("%s: Number of bytes read: %I64d\n",DriverName,(INT64)bytesRead));
 }
 else 
 {
  KdPrint(("%s: Read failed - request status 0x%x UsbdStatus 0x%x\n",DriverName,status,usbCompletionParams->UsbdStatus));
 }
 WdfRequestCompleteWithInformation(Request,status,bytesRead);//���������� �������
 return;
}
//----------------------------------------------------------------------------------------------------
//���������� ������� ������
//Queue-�������.
//Request-��������� �������.
//Length-���������� ���� ��� ������
//----------------------------------------------------------------------------------------------------
VOID EvtIoWrite(IN WDFQUEUE Queue,IN WDFREQUEST Request,IN size_t Length)
{
 NTSTATUS status;//������������ ��������
 WDFUSBPIPE pipe;//�����
 WDFMEMORY reqMemory;
 PDEVICE_CONTEXT pDeviceContext;//�������� ����������
 BOOLEAN ret;
  
 UNREFERENCED_PARAMETER(Length);//������, ����� ���������� �� ������� �� �������������� ��������
  
 pDeviceContext=GetDeviceContext(WdfIoQueueGetDevice(Queue));//�������� �������� ����������
  
 if (pDeviceContext->UsbDevice==NULL)
 {
  status=STATUS_INVALID_DEVICE_REQUEST;//������ �� ������
  KdPrint(("%s: USBDevice==NULL!\n",DriverName));//������� ��������� � ������� ��������� 
  WdfRequestCompleteWithInformation(Request,status,0);
  return;
 }

 pipe=pDeviceContext->BulkWritePipe;//���� ����� ��� ������

 if (pipe==NULL)
 {
  KdPrint(("%s: Pipe is NULL!\n",DriverName));//������� ��������� � ������� ��������� 
  status=STATUS_SUCCESS;
  WdfRequestCompleteWithInformation(Request,status,0);//���������� �������
  return;
 }

 status=WdfRequestRetrieveInputMemory(Request,&reqMemory);
 if (!NT_SUCCESS(status))//��������� ������
 {
  WdfRequestCompleteWithInformation(Request,status,0);//���������� �������
  return;
 }
 //������ ����� �����������
 status=WdfUsbTargetPipeFormatRequestForWrite(pipe,Request,reqMemory,NULL);
 if (!NT_SUCCESS(status))//��������� ������
 {
  WdfRequestCompleteWithInformation(Request,status,0);//���������� �������
  return;
 }
  
 WdfRequestSetCompletionRoutine(Request,EvtRequestWriteCompletionRoutine,pipe);
  
 ret=WdfRequestSend(Request,WdfUsbTargetPipeGetIoTarget(pipe),WDF_NO_SEND_OPTIONS);
 if (ret==FALSE)//������
 {
  status=WdfRequestGetStatus(Request);
  WdfRequestCompleteWithInformation(Request,status,0);//���������� �������
  return;
 } 
 else return;
  
 WdfRequestCompleteWithInformation(Request,status,0);//���������� �������
 return;
}
//----------------------------------------------------------------------------------------------------
//���������� ������ ���������
//----------------------------------------------------------------------------------------------------
VOID EvtRequestWriteCompletionRoutine(IN WDFREQUEST Request,IN WDFIOTARGET Target,PWDF_REQUEST_COMPLETION_PARAMS CompletionParams,IN WDFCONTEXT Context)
{
 NTSTATUS status;
 size_t bytesWritten=0;//���������� ��������� ����
 PWDF_USB_REQUEST_COMPLETION_PARAMS usbCompletionParams;
  
 UNREFERENCED_PARAMETER(Target);//������, ����� ���������� �� ������� �� �������������� ��������
 UNREFERENCED_PARAMETER(Context);//������, ����� ���������� �� ������� �� �������������� ��������
  
 status=CompletionParams->IoStatus.Status;
  
 usbCompletionParams=CompletionParams->Parameters.Usb.Completion;
  
 bytesWritten=usbCompletionParams->Parameters.PipeWrite.Length;
 if (NT_SUCCESS(status))//������ ������ �������
 {
  KdPrint(("%s: Number of bytes written: %I64d\n",DriverName,(INT64)bytesWritten));
 }
 else//������ ������
 {
  KdPrint(("%s: Write failed: request Status 0x%x UsbdStatus 0x%x\n",DriverName,status,usbCompletionParams->UsbdStatus));
 }
 WdfRequestCompleteWithInformation(Request,status,bytesWritten);//���������� �������
 return;
}
