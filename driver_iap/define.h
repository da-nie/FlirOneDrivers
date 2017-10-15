#ifndef DEFINE_H
#define DEFINE_H

//адрес конечной точки на чтение
#define BULK_READ_ENDPOINT_ADDR		0x81
//адрес конечной точки на запись
#define BULK_WRITE_ENDPOINT_ADDR	0x02
 
//коды управления
//остановить устройство Frame
#define IOCTL_FLIR_ONE_GEN2_FRAME_STOP		1
//запустить устройство Frame
#define IOCTL_FLIR_ONE_GEN2_FRAME_START		2
//остановить устройство FileIO
#define IOCTL_FLIR_ONE_GEN2_FILEIO_STOP		4
//запустить устройство FileIO
#define IOCTL_FLIR_ONE_GEN2_FILEIO_START	8

//имя драйвера
char DriverName[]="FlirOneGen2_iAP";

//глобально-уникальный идентификатор драйвера (с его помощью драйвер будет доступен из программ)
// {41A83025-9DC8-4e75-8705-1209222FDC17}
DEFINE_GUID(GUID_DEVINTERFACE_FLIRONEGEN2,0x41a83025, 0x9dc8, 0x4e75, 0x87, 0x5, 0x12, 0x9, 0x22, 0x2f, 0xdc, 0x17);


#endif

