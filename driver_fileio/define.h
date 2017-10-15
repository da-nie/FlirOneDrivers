#ifndef DEFINE_H
#define DEFINE_H

//адрес конечной точки на чтение
#define BULK_READ_ENDPOINT_ADDR		0x83
//адрес конечной точки на запись
#define BULK_WRITE_ENDPOINT_ADDR	0x04
 
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
char DriverName[]="FlirOneGen2_FileIO";

//глобально-уникальный идентификатор драйвера (с его помощью драйвер будет доступен из программ)
// {AA0DFC0C-E043-4778-9DBA-28749A3AA9F5}
DEFINE_GUID(GUID_DEVINTERFACE_FLIRONEGEN2,0xaa0dfc0c, 0xe043, 0x4778, 0x9d, 0xba, 0x28, 0x74, 0x9a, 0x3a, 0xa9, 0xf5);


#endif

