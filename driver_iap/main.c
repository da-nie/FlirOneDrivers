#include <stdarg.h>
#include "ntddk.h"
#include "wdf.h"
#include "prototypes.h"
#include "usbdi.h"
#include "wdfusb.h"
#include "initguid.h"
#include "define.h"

extern char DriverName[];//имя драйвера

//структура контекста устройства (устройств ведь можно подключить несколько с помощью одного и того же драйвера)
typedef struct _DEVICE_CONTEXT 
{
 WDFUSBDEVICE UsbDevice;//идентификатор устройства
 WDFUSBINTERFACE UsbInterface;//идентификатор интерфейса
 WDFUSBPIPE BulkReadPipe;//канал на чтение
 WDFUSBPIPE BulkWritePipe;//канал на запись
} DEVICE_CONTEXT,*PDEVICE_CONTEXT;
 
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT,GetDeviceContext)
 
//----------------------------------------------------------------------------------------------------
//функция, вызываемая при загрузке драйвера
//DriverObject - указатель на объект-драйвер, соответствующий загружаемому драйверу. 
//RegistryPath - указатель на строку в формате Unicode с именем ключа реестра, соответствующего загружаемому драйверу.
//----------------------------------------------------------------------------------------------------
NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject,IN PUNICODE_STRING RegistryPath)
{
 NTSTATUS status;//возвращаемое значение
 WDF_DRIVER_CONFIG config;//конфигурация драйвера
 WDF_OBJECT_ATTRIBUTES attributes;//параметры устройства
 
 KdPrint(("%s: DriverEntry.\n",DriverName));//выводим сообщение в консоль отладчика 
 WDF_DRIVER_CONFIG_INIT(&config,EvtDeviceAdd);//инициализируем стандартную конфигурацию
  
 //инициализируем атрибуты
 WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
 
 //создаём драйвер устройства
 status=WdfDriverCreate(DriverObject,RegistryPath,&attributes,&config,WDF_NO_HANDLE);
 if (!NT_SUCCESS(status))//произошла ошибка
 {
  KdPrint(("%s: WdfDriverCreate failed 0x%x\n",DriverName,status));
  return(status);
 } 
 return(status);
}
//----------------------------------------------------------------------------------------------------
//функция, вызываемая при добавлении устройства
//---------------------------------------------------------------------------------------------------- 
NTSTATUS EvtDeviceAdd(IN WDFDRIVER Driver,IN PWDFDEVICE_INIT DeviceInit)
{
 NTSTATUS status;//возвращаемое значение
 WDFDEVICE device;//устройство
 WDF_OBJECT_ATTRIBUTES attributes;//атрибуты устройства (там будет контекст устройства)
 PDEVICE_CONTEXT pDevContext;//контекст устройства
 WDF_PNPPOWER_EVENT_CALLBACKS pnpPowerCallbacks;//событие подключения устройсва
 WDF_IO_QUEUE_CONFIG ioQueueConfig;//конфигурация функций обработки запросов
  
 UNREFERENCED_PARAMETER(Driver);//макрос, чтобы компилятор не ругался на неиспользуемый параметр
  
 KdPrint(("%s: EvtDeviceAdd\n",DriverName));//выводим сообщение в консоль отладчика 
  
 //регистрируем функцию подготовки устройства после подключения
 WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);
 pnpPowerCallbacks.EvtDevicePrepareHardware=EvtDevicePrepareHardware;//задаём функцию настройки устройства
 WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit,&pnpPowerCallbacks);//привязываем её к событию
 WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes,DEVICE_CONTEXT);//инициалдизируем атрибуты контекстом
  
 status=WdfDeviceCreate(&DeviceInit,&attributes,&device);//создаём устройство
 if (!NT_SUCCESS(status))//произошла ошибка
 {
  KdPrint(("%s: WdfDeviceCreate failed 0x%x\n",DriverName,status));
  return(status);
 }
  
 pDevContext=GetDeviceContext(device);//получаем контекст устройства
 //создаём интерфейс для работы с устройством (через него программы будут общаться с драйвером)
 status=WdfDeviceCreateDeviceInterface(device,(LPGUID)&GUID_DEVINTERFACE_FLIRONEGEN2,NULL);
 if (!NT_SUCCESS(status))//произошла ошибка
 {
  KdPrint(("%s: WdfDeviceCreateDeviceInterface failed 0x%x\n",DriverName,status));
  return(status);
 }
 //подключаем обработчики чтения и записи для очереди событий
 WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&ioQueueConfig,WdfIoQueueDispatchParallel);
 ioQueueConfig.EvtIoDeviceControl=EvtIoDeviceControl;
 ioQueueConfig.EvtIoRead=EvtIoRead;
 ioQueueConfig.EvtIoWrite=EvtIoWrite;
  
 status=WdfIoQueueCreate(device,&ioQueueConfig,WDF_NO_OBJECT_ATTRIBUTES,WDF_NO_HANDLE);
 if (!NT_SUCCESS(status))//произошла ошибка
 {
  KdPrint(("%s: WdfIoQueueCreate failed 0x%x\n",DriverName,status));
  return(status);
 }  
 return(status);
}
 
//----------------------------------------------------------------------------------------------------
//функция, вызываемая при настройке устройства
//----------------------------------------------------------------------------------------------------
NTSTATUS EvtDevicePrepareHardware(IN WDFDEVICE Device,IN WDFCMRESLIST ResourceList,IN WDFCMRESLIST ResourceListTranslated)
{
 long n;
 NTSTATUS status;//возвращаемое значение
 PDEVICE_CONTEXT pDeviceContext;//контекст устройства
 WDF_USB_DEVICE_SELECT_CONFIG_PARAMS configParams;//параметры устройства
 UCHAR numInterfaces;
 WDFUSBPIPE pipe;//канал
 UCHAR ep_index=0;//номер конечной точки
 WDF_USB_PIPE_INFORMATION pipeConfig;//настройка канала
  
 UNREFERENCED_PARAMETER(ResourceList);//макрос, чтобы компилятор не ругался на неиспользуемый параметр
 UNREFERENCED_PARAMETER(ResourceListTranslated);//макрос, чтобы компилятор не ругался на неиспользуемый параметр
  
 KdPrint(("%s: EvtDevicePrepareHardware\n",DriverName)); 
  
 pDeviceContext=GetDeviceContext(Device);//получаем контекст устройства
  
 //создаём объект USB-устройства, если он ещё не был создан
 if (pDeviceContext->UsbDevice==NULL)//ещё не создавали USB-устройства
 {
  status=WdfUsbTargetDeviceCreate(Device,WDF_NO_OBJECT_ATTRIBUTES,&pDeviceContext->UsbDevice);
  if (!NT_SUCCESS(status))//произошла ошибка
  {
   KdPrint(("%s: WdfUsbTargetDeviceCreate failed 0x%x\n",DriverName,status)); 
   return(status);
  }
 }  
 //инициализируем как один интерфейс  
 numInterfaces=WdfUsbTargetDeviceGetNumInterfaces(pDeviceContext->UsbDevice);
 KdPrint(("%s: Interfaces:0x%02x.\n",DriverName,numInterfaces));
  
 WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_SINGLE_INTERFACE(&configParams);
    
 //выбираем нужную конфигурацию для устройства USB
 status=WdfUsbTargetDeviceSelectConfig(pDeviceContext->UsbDevice,WDF_NO_OBJECT_ATTRIBUTES,&configParams);
 if(!NT_SUCCESS(status))//произошла ошибка
 {
  KdPrint(("%s: WdfUsbTargetDeviceSelectConfig failed 0x%x\n",DriverName,status));
  return(status);
 }
 pDeviceContext->UsbInterface=configParams.Types.SingleInterface.ConfiguredUsbInterface;
 //получаем 2 канала: на запись и на чтение
 pDeviceContext->BulkReadPipe=NULL;
 pDeviceContext->BulkWritePipe=NULL;
 WDF_USB_PIPE_INFORMATION_INIT(&pipeConfig);
 while(1)
 {
  pipe=WdfUsbInterfaceGetConfiguredPipe(pDeviceContext->UsbInterface,ep_index,&pipeConfig);
  if (pipe==NULL) break;//больше конечных точек нет
  KdPrint(("FlirOneGen2_iAP: EndPoint addr:0x%02x\n",pipeConfig.EndpointAddress));
  if (pipeConfig.EndpointAddress==BULK_READ_ENDPOINT_ADDR) pDeviceContext->BulkReadPipe=pipe;
  if (pipeConfig.EndpointAddress==BULK_WRITE_ENDPOINT_ADDR) pDeviceContext->BulkWritePipe=pipe;
  ep_index++;
 }
  
 //канал на чтение
 if (pDeviceContext->BulkReadPipe!=NULL)
 {
  KdPrint(("%s: BulkReadPipe is OK.\n",DriverName));
  WdfUsbTargetPipeSetNoMaximumPacketSizeCheck(pDeviceContext->BulkReadPipe);//отключаем проверку на максимальный размер пакета 
 }
 else KdPrint(("%s: BulkReadPipe is NULL!\n",DriverName));
  
 //канал на запись
 if (pDeviceContext->BulkWritePipe!=NULL)
 {
  KdPrint(("%s: BulkWritePipe is OK.\n",DriverName));
  WdfUsbTargetPipeSetNoMaximumPacketSizeCheck(pDeviceContext->BulkWritePipe);//отключаем проверку на максимальный размер пакета
 }
 else KdPrint(("%s BulkWritePipe is NULL!\n",DriverName));
 return(status);
}
 
//----------------------------------------------------------------------------------------------------
//функция, вызываемая при DeviceIoControl
//----------------------------------------------------------------------------------------------------
VOID EvtIoDeviceControl(IN WDFQUEUE Queue,IN WDFREQUEST Request,IN size_t OutputBufferLength, IN size_t InputBufferLength,IN ULONG IoControlCode)
{
 WDFDEVICE device;//устройство
 PDEVICE_CONTEXT pDeviceContext;//контекст устройства
 NTSTATUS status;//возвращаемое значение
 WDF_USB_CONTROL_SETUP_PACKET controlSetupPacket;//управляющий пакет
 WDF_MEMORY_DESCRIPTOR memDesc;//дескриптор области памяти
 WDFMEMORY memory;//область памяти
 WDF_REQUEST_SEND_OPTIONS sendOptions;//опции пересылки управляющего пакета
 size_t bytesTransferred=0;//количество переданных байт
 unsigned char data[2]={0,0};
 USHORT value=0;
 USHORT index=0;
 unsigned char ok=0;
 long n;
 unsigned char *ptr;
  
 UNREFERENCED_PARAMETER(InputBufferLength);
 UNREFERENCED_PARAMETER(OutputBufferLength);
  
 KdPrint(("%s: EvtIoDeviceControl.\n",DriverName));//выводим сообщение в консоль отладчика 
  
 status=STATUS_INVALID_DEVICE_REQUEST;//запрос не прошёл
  
 device=WdfIoQueueGetDevice(Queue);
 pDeviceContext=GetDeviceContext(device);
  
 if (pDeviceContext->UsbDevice==NULL)
 {
  KdPrint(("%s: USBDevice=NULL!\n",DriverName));//выводим сообщение в консоль отладчика 
  WdfRequestCompleteWithInformation(Request,status,bytesTransferred);
  return;
 }

 if (IoControlCode==IOCTL_FLIR_ONE_GEN2_FRAME_STOP)//остановить устройство
 {
  KdPrint(("%s: IOCTL_FLIR_ONE_GEN2_FRAME_STOP.\n",DriverName));//выводим сообщение в консоль отладчика 
  value=0;
  index=2;
  ok=1;
 }
 if (IoControlCode==IOCTL_FLIR_ONE_GEN2_FRAME_START)//запустить устройство
 {
  KdPrint(("%s: IOCTL_FLIR_ONE_GEN2_FRAME_START.\n",DriverName));//выводим сообщение в консоль отладчика 
  value=1;
  index=2;
  ok=1;
 }
 if (IoControlCode==IOCTL_FLIR_ONE_GEN2_FILEIO_STOP)//остановить устройство
 {
  KdPrint(("%s: IOCTL_FLIR_ONE_GEN2_FILEIO_STOP.\n",DriverName));//выводим сообщение в консоль отладчика 
  value=0;
  index=1;
  ok=1;
 }
 if (IoControlCode==IOCTL_FLIR_ONE_GEN2_FILEIO_START)//запустить устройство
 {
  KdPrint(("%s: IOCTL_FLIR_ONE_GEN2_FILEIO_START.\n",DriverName));//выводим сообщение в консоль отладчика 
  value=1;
  index=1;
  ok=1;
 }
 if (ok==1) 
 {
  //шлём управляющее сообщение в нулевую конечную точку
  //настраиваем область памяти
  status=WdfRequestRetrieveInputMemory(Request,&memory);
  if (!NT_SUCCESS(status)) 
  {
   KdPrint(("%s: WdfRequestRetrieveMemory failed 0x%x",DriverName,status));
   WdfRequestCompleteWithInformation(Request,status,bytesTransferred);
   return;
  }
  //выдаём пакеты настройки конфигурации устройства
   
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
  WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&memDesc,data,0);//данных не передаём
   
  //настраиваем параметры пересылки
  WDF_REQUEST_SEND_OPTIONS_INIT(&sendOptions,WDF_REQUEST_SEND_OPTION_TIMEOUT);
  WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&sendOptions,WDF_REL_TIMEOUT_IN_MS(200));  
   
  status=WdfUsbTargetDeviceSendControlTransferSynchronously(pDeviceContext->UsbDevice,WDF_NO_HANDLE,&sendOptions,&controlSetupPacket,&memDesc,(PULONG)&bytesTransferred);
  if (!NT_SUCCESS(status)) 
  {
   KdPrint(("%s: SendControlTransfer failed 0x%x",DriverName,status));
   WdfRequestCompleteWithInformation(Request,status,bytesTransferred);
   return;
  }
  KdPrint(("%s: Ok. Transferred:0x%02x\n",DriverName,bytesTransferred));//выводим сообщение в консоль отладчика 
 }
 WdfRequestCompleteWithInformation(Request,status,bytesTransferred);
 return;
}
 
//----------------------------------------------------------------------------------------------------
//обработчик события чтения
//Queue-очередь.
//Request-структура запроса.
//Length-количество байт для чтения
//----------------------------------------------------------------------------------------------------
VOID EvtIoRead(IN WDFQUEUE Queue,IN WDFREQUEST Request,IN size_t Length)
{
 NTSTATUS status;//возвращаемое значение
 WDFUSBPIPE pipe;//канал
 WDFMEMORY reqMemory;
 PDEVICE_CONTEXT pDeviceContext;//контекст устройства
 BOOLEAN ret;
  
 UNREFERENCED_PARAMETER(Length);//макрос, чтобы компилятор не ругался на неиспользуемый параметр
  
 KdPrint(("%s: EvtIoRead.\n",DriverName));//выводим сообщение в консоль отладчика 
  
 pDeviceContext=GetDeviceContext(WdfIoQueueGetDevice(Queue));//получаем контекст устройства

 if (pDeviceContext->UsbDevice==NULL)
 {
  status=STATUS_INVALID_DEVICE_REQUEST;//запрос не прошёл
  KdPrint(("%s: USBDevice==NULL!\n",DriverName));//выводим сообщение в консоль отладчика 
  WdfRequestCompleteWithInformation(Request,status,0);
  return;
 }
 
 pipe=pDeviceContext->BulkReadPipe;//берём канал для чтения
 if (pipe==NULL)
 {
  KdPrint(("%s: Pipe is NULL!\n",DriverName));//выводим сообщение в консоль отладчика 
  status=STATUS_SUCCESS;
  WdfRequestCompleteWithInformation(Request,status,0);//завершение запроса
  return;
 }
  
 status=WdfRequestRetrieveOutputMemory(Request,&reqMemory);
 if (!NT_SUCCESS(status))//произошла ошибка
 {
  KdPrint(("%s: WdfRequestRetrieveOutputMemory failed 0x%x",DriverName,status));
  WdfRequestCompleteWithInformation(Request,status,0);//завершение запроса
  return;
 }
 //получаем данные из канала
 status=WdfUsbTargetPipeFormatRequestForRead(pipe,Request,reqMemory,NULL);
 if (!NT_SUCCESS(status))//произошла ошибка
 {
  KdPrint(("FlirOneGen2_iAP: WdfUsbTargetPipeFormatRequestForRead failed 0x%x", status));
  WdfRequestCompleteWithInformation(Request,status,0);//завершение запроса
  return;
 }
 //чтение будет асинхронное
 WdfRequestSetCompletionRoutine(Request,EvtRequestReadCompletionRoutine,pipe);
  
 ret=WdfRequestSend(Request,WdfUsbTargetPipeGetIoTarget(pipe),WDF_NO_SEND_OPTIONS);
 if (ret==FALSE) 
 {
  KdPrint(("%s: WdfRequestSend failed 0x%x",DriverName,status));
  status=WdfRequestGetStatus(Request);
  WdfRequestCompleteWithInformation(Request,status,0);//завершение запроса
  return; 
 } 
 else return;
 KdPrint(("%s: Ok.",DriverName));
 WdfRequestCompleteWithInformation(Request,status,0);//завершение запроса
 return;
}
 
//----------------------------------------------------------------------------------------------------
//обработчик чтение завершено
//----------------------------------------------------------------------------------------------------
VOID EvtRequestReadCompletionRoutine(IN WDFREQUEST Request,IN WDFIOTARGET Target,PWDF_REQUEST_COMPLETION_PARAMS CompletionParams,IN WDFCONTEXT Context)
{
 NTSTATUS status;//возвращаемое значените
 size_t bytesRead=0;//количество считанных байт
 PWDF_USB_REQUEST_COMPLETION_PARAMS usbCompletionParams;
  
 UNREFERENCED_PARAMETER(Target);//макрос, чтобы компилятор не ругался на неиспользуемый параметр
 UNREFERENCED_PARAMETER(Context);//макрос, чтобы компилятор не ругался на неиспользуемый параметр
  
 status=CompletionParams->IoStatus.Status;
  
 usbCompletionParams=CompletionParams->Parameters.Usb.Completion;
 bytesRead=usbCompletionParams->Parameters.PipeRead.Length;
  
 if (NT_SUCCESS(status))//чтение завершено успешно
 {
  KdPrint(("%s: Number of bytes read: %I64d\n",DriverName,(INT64)bytesRead));
 }
 else 
 {
  KdPrint(("%s: Read failed - request status 0x%x UsbdStatus 0x%x\n",DriverName,status,usbCompletionParams->UsbdStatus));
 }
 WdfRequestCompleteWithInformation(Request,status,bytesRead);//завершение запроса
 return;
}
//----------------------------------------------------------------------------------------------------
//обработчик события записи
//Queue-очередь.
//Request-структура запроса.
//Length-количество байт для записи
//----------------------------------------------------------------------------------------------------
VOID EvtIoWrite(IN WDFQUEUE Queue,IN WDFREQUEST Request,IN size_t Length)
{
 NTSTATUS status;//возвращаемое значение
 WDFUSBPIPE pipe;//канал
 WDFMEMORY reqMemory;
 PDEVICE_CONTEXT pDeviceContext;//контекст устройства
 BOOLEAN ret;
  
 UNREFERENCED_PARAMETER(Length);//макрос, чтобы компилятор не ругался на неиспользуемый параметр
  
 pDeviceContext=GetDeviceContext(WdfIoQueueGetDevice(Queue));//получаем контекст устройства
  
 if (pDeviceContext->UsbDevice==NULL)
 {
  status=STATUS_INVALID_DEVICE_REQUEST;//запрос не прошёл
  KdPrint(("%s: USBDevice==NULL!\n",DriverName));//выводим сообщение в консоль отладчика 
  WdfRequestCompleteWithInformation(Request,status,0);
  return;
 }

 pipe=pDeviceContext->BulkWritePipe;//берём канал для записи

 if (pipe==NULL)
 {
  KdPrint(("%s: Pipe is NULL!\n",DriverName));//выводим сообщение в консоль отладчика 
  status=STATUS_SUCCESS;
  WdfRequestCompleteWithInformation(Request,status,0);//завершение запроса
  return;
 }

 status=WdfRequestRetrieveInputMemory(Request,&reqMemory);
 if (!NT_SUCCESS(status))//произошла ошибка
 {
  WdfRequestCompleteWithInformation(Request,status,0);//завершение запроса
  return;
 }
 //запись будет асинхронная
 status=WdfUsbTargetPipeFormatRequestForWrite(pipe,Request,reqMemory,NULL);
 if (!NT_SUCCESS(status))//произошла ошибка
 {
  WdfRequestCompleteWithInformation(Request,status,0);//завершение запроса
  return;
 }
  
 WdfRequestSetCompletionRoutine(Request,EvtRequestWriteCompletionRoutine,pipe);
  
 ret=WdfRequestSend(Request,WdfUsbTargetPipeGetIoTarget(pipe),WDF_NO_SEND_OPTIONS);
 if (ret==FALSE)//ошибка
 {
  status=WdfRequestGetStatus(Request);
  WdfRequestCompleteWithInformation(Request,status,0);//завершение запроса
  return;
 } 
 else return;
  
 WdfRequestCompleteWithInformation(Request,status,0);//завершение запроса
 return;
}
//----------------------------------------------------------------------------------------------------
//обработчик запись завершена
//----------------------------------------------------------------------------------------------------
VOID EvtRequestWriteCompletionRoutine(IN WDFREQUEST Request,IN WDFIOTARGET Target,PWDF_REQUEST_COMPLETION_PARAMS CompletionParams,IN WDFCONTEXT Context)
{
 NTSTATUS status;
 size_t bytesWritten=0;//количество считанных байт
 PWDF_USB_REQUEST_COMPLETION_PARAMS usbCompletionParams;
  
 UNREFERENCED_PARAMETER(Target);//макрос, чтобы компилятор не ругался на неиспользуемый параметр
 UNREFERENCED_PARAMETER(Context);//макрос, чтобы компилятор не ругался на неиспользуемый параметр
  
 status=CompletionParams->IoStatus.Status;
  
 usbCompletionParams=CompletionParams->Parameters.Usb.Completion;
  
 bytesWritten=usbCompletionParams->Parameters.PipeWrite.Length;
 if (NT_SUCCESS(status))//запись прошла успешно
 {
  KdPrint(("%s: Number of bytes written: %I64d\n",DriverName,(INT64)bytesWritten));
 }
 else//ошибка записи
 {
  KdPrint(("%s: Write failed: request Status 0x%x UsbdStatus 0x%x\n",DriverName,status,usbCompletionParams->UsbdStatus));
 }
 WdfRequestCompleteWithInformation(Request,status,bytesWritten);//завершение запроса
 return;
}
