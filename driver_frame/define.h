#ifndef DEFINE_H
#define DEFINE_H

//����� �������� ����� �� ������
#define BULK_READ_ENDPOINT_ADDR		0x85
//����� �������� ����� �� ������
#define BULK_WRITE_ENDPOINT_ADDR	0x06

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
char DriverName[]="FlirOneGen2_Frame";
 
//���������-���������� ������������� �������� (� ��� ������� ������� ����� �������� �� ��������)
// {1B2E6756-0344-449e-8B5B-BD57F582766B}
DEFINE_GUID(GUID_DEVINTERFACE_FLIRONEGEN2,0x1b2e6756, 0x344, 0x449e, 0x8b, 0x5b, 0xbd, 0x57, 0xf5, 0x82, 0x76, 0x6b);


#endif

