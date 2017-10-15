#ifndef DEFINE_H
#define DEFINE_H

//����� �������� ����� �� ������
#define BULK_READ_ENDPOINT_ADDR		0x81
//����� �������� ����� �� ������
#define BULK_WRITE_ENDPOINT_ADDR	0x02
 
//���� ����������
//���������� ���������� Frame
#define IOCTL_FLIR_ONE_GEN2_FRAME_STOP		1
//��������� ���������� Frame
#define IOCTL_FLIR_ONE_GEN2_FRAME_START		2
//���������� ���������� FileIO
#define IOCTL_FLIR_ONE_GEN2_FILEIO_STOP		4
//��������� ���������� FileIO
#define IOCTL_FLIR_ONE_GEN2_FILEIO_START	8

//��� ��������
char DriverName[]="FlirOneGen2_iAP";

//���������-���������� ������������� �������� (� ��� ������� ������� ����� �������� �� ��������)
// {41A83025-9DC8-4e75-8705-1209222FDC17}
DEFINE_GUID(GUID_DEVINTERFACE_FLIRONEGEN2,0x41a83025, 0x9dc8, 0x4e75, 0x87, 0x5, 0x12, 0x9, 0x22, 0x2f, 0xdc, 0x17);


#endif

