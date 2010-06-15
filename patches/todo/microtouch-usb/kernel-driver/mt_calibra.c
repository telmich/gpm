// copyrights (c) WWO
// 
// Written by Rados³aw Garbacz
// 14/06/2000
// A simple calibration program for micro touch screen.


#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "touchscreen.h"
//#include <>

int getStatus(int _hFile)
{
  unsigned char buff[20];
  int nRet;

  //printf("Get status\n");
  if((nRet=ioctl(_hFile,TSCRN_CONTROLLER_STATUS ,buff)) < 0)
  {
    printf("Error ioctl %d\n",nRet);
    close(_hFile);
    exit(1);
  }
  //printf("cmd status = %d\n",buff[3]);

  return buff[3];
};
int main()
{
  int nRet,hFile;
  char buff[50];

  memset(buff, 0, sizeof(buff));

  if((hFile=open("/dev/usb/usbtest",O_RDONLY )) < 0)
  {
    printf("Error open %d\n",hFile);
    return 0;
  };
  (void)getStatus(hFile);
  printf("Soft reset of device\n");
  if((nRet=ioctl(hFile,73)) < 0)
  {
    printf("Error ioctl %d\n",nRet);
    return 0;
  }
  (void)getStatus(hFile);
  printf("Calibrate controler extended \n");
  *buff = TSCRN_CORNER_CALIBRATION_TYPE;
  if((nRet=ioctl(hFile,TSCRN_CALIBRATION,buff)) < 0)
  {
    printf("Error ioctl %d\n",nRet);
    return 0;
  }
  printf("\n");
  //while((nRet = getStatus(hFile)) != 1)
  //  printf("wait for staus 1 status=%d\r",nRet);
  //printf("\n");
  while((nRet=getStatus(hFile)) < 2)
  {
    printf("wait for lower left corner %d\n",nRet);
    sleep(1);
  }
  printf("\n");
  while((nRet=getStatus(hFile)) < 3)
  {
    printf("lower left corner complited;wait for upper right corner %d\n",nRet);
    sleep(1);
  }
  printf("\n");
  if((nRet=getStatus(hFile)) == 3)
    printf("upper right corner complited %d\n",nRet);
  else
    printf("upper right corner error %d\n",nRet);

  printf(" koniec kalibracji OK ioctl: %d \n",nRet);
  close(hFile);
  return 0;
}
