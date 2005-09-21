#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>

unsigned char* inbuffer;
unsigned char* outbuffer;

// wait for up to 0.1seconds for an acknowledgement
int getack(int fd) {
  fd_set set;
  timeval timeout;
  FD_ZERO(&set);
  FD_SET(fd,&set);
  timeout.tv_sec=0;
  timeout.tv_usec=100000;
  if (select(FD_SETSIZE,&set,NULL,NULL,&timeout)) {
    int b=read(fd,inbuffer,1);
    if(b==1) {
      if(inbuffer[0]==0xfa) {
        //printf("<< Acknowledged\r\n");
        return 1;
      }
      else if (inbuffer[0]=0xfe)
        printf("<- PS/2: Error\r\n");
      else	
        printf("<- Not an acknowledgement:%02x\r\n",inbuffer[0]);
    };
  };
  return 0;
};

// Try to receive n bytes
int receive(int fd,int n) {
  int a=0,b;
  while(a<n) {
    b= read(fd,inbuffer+a,n-a);
    if(b==-1) {
      printf("Read Error!\r\n");
      return 0;
    } if (b==0) 
      return a;
    else {
      if(inbuffer[a]==0xfa) 
      	printf("<- Acknowledged\r\n");
      else
        a=a+b;
    }
  }
  return a;
}

// Send cmd to the fd filehandle
void command(int fd, unsigned char cmd) {
  outbuffer[0]=cmd;
  write(fd,outbuffer,1);
  
}  

// Detect trackpoint device!
int detectTP(int fd) {
  fd_set set;
  timeval timeout;
  FD_ZERO(&set);
  FD_SET(fd,&set);
  timeout.tv_sec=0;
  timeout.tv_usec=100000;

  printf("-> TP: Send extended ID\r\n");
  command(fd,0xe1);
  if (! getack(fd))
  {
    if(inbuffer[0]==0xfe) { // give it a second try
      command(fd,0xe1);
      if(getack(fd)) goto ack;
    }
    printf("No trackpoint controller detected\r\n");  
    return 0;
      
  }  
  ack:
  int a=0;
  while(select(FD_SETSIZE,&set,NULL,NULL,&timeout)) {
    read(fd,inbuffer+a,1);
    a++;
  }
  char * version;
  if(inbuffer[0]==0x01) {
    switch(inbuffer[1]) {
      case 0x01:
      	version = "8E/98";
	break;
      case 0x02:
      	version = "A4";
	break;
      case 0x03:
      	version = "AB";
	break;
      case 0x04:
      	version = "03";
	break;
      case 0x05:
      	version = "B2/B4";
	break;
      case 0x06:
      	version = "B1/B3/B5/B8/2B";
	break;
      case 0x07:
      	version = "?";
	break;
      case 0x08:
      	version = "?";
	break;
      case 0x09:
      	version = "?";
	break;
      case 0x0A:
      	version = "35";
	break;
      case 0x0B:
      	version = "3A/3B";
	break;
      case 0x0C:
      	version = "3C";
	break;
      case 0x0D:
      	version = "3D";
	break;
      case 0x0E:
      	version = "3E";
	break;
      default:
      	printf("Unrecognized ID: %02x\n",inbuffer[1]);
	return 0;
    }
    printf("IBM Trackpoint controller detected. Version %s \r\n",version);
    return 1;
  } else {
    printf("No IBM trackpoint controller detected\r\n");  
    printf("Extended ID: %02x %02x\r\n",inbuffer[0],inbuffer[1]);
    return 0;
  }
}
    
 
// Reset PS/2 device
int reset(int fd) {
  command(fd,0xff);
  printf("-> PS/2: Reset\r\n");
  getack(fd);
  receive(fd,2); 
  if(inbuffer[0]==0xaa && inbuffer[1]==0x00) {
    printf("<- PS/2: Succesfull initialization\r\n");
    return 1;
  } else
    return 0;  
}  

// Get PS/2 ID
int getdeviceid(int fd) {
  command(fd,0xf2);
  getack(fd);
  printf("-> PS/2: Get device ID\r\n");
  if(receive(fd,1)==1) {
    printf("<- PS/2: Device ID: %02x\n",inbuffer[0]);
    return inbuffer[0];
  }
}  

void enabledatareport(int fd) {
  command(fd, 0xf4);
  printf("-> PS/2: (re)Enable data-reporting\r\n");
  getack(fd);
}  

void disabledatareport(int fd) {
  command(fd, 0xf5);
  printf("-> PS/2: Disable data-reporting\r\n");
  getack(fd);
}  

void setsamplerate(int fd, int rate) {
  command(fd,0xf3);  
  printf("-> PS/2: Set sample rate: %u\r\n",rate);
  getack(fd);
  command(fd,rate);
  getack(fd);
}  

void setresolution(int fd, int res) {
  command(fd,0xe8);
  printf("-> PS/2: Set resolution: %u\r\n",res);
  getack(fd);
  command(fd,res);
  getack(fd);
}

void getstatus(int fd) {
  command(fd,0xe9);
  printf("-> Send status\r\n");
  receive(fd,3);
  printf("<- Status is: 0x%02x,0x%02x,0x%02x\n",inbuffer[0],inbuffer[1],inbuffer[2]);
}

void getlogiid(int fd) {
  receive(fd,3);
  unsigned char a = inbuffer[0];
  unsigned char id = ((a >> 4) & 0x07) | ((a<<3) & 0x78);
  printf("Logi-id: %d\r\n", id);
};


// Put trackpoint device into transparent mode
void enabletransparent(int fd) {
  command(fd,0xe2);
  getack(fd);
  command(fd,0x4e);
  getack(fd);
  printf("-> TP: Set soft-transparent\r\n");
}

// Get trackpoint device out of transparent mode
// Notice the command gives not-ack because its in transparent mode,
// just ignore these. After the command finishes to acknowledges are sent.
void disabletransparent(int fd) {
  command(fd,0xe2); 
  //getack(fd);
  command(fd,0xb9);
  getack(fd);
  printf("-> TP: Cancel soft-transparent\r\n");
}

// Detect external device on trackpoint controller
bool detectExternal(int fd) {
  command(fd,0xe2);
  getack(fd);
  command(fd,0x21);
  getack(fd);
  receive(fd,1);
  if(inbuffer[0] && 8) {
    printf("<- TP: External device present\r\n");
    return true;
  }  
  else {
    printf("<- TP: No external device\r\n");
    return false;
  }  
}  

// IMPS/2 magic-initialization and detection
bool detectIMPS(int fd) {
  setsamplerate(fd,200);  
  setsamplerate(fd,100);  
  setsamplerate(fd,80);
  if(getdeviceid(fd)==3) {
    printf("IMPS/2 mouse detected\r\n");
  }
}  


int main() {
  inbuffer = new unsigned char[200];
  outbuffer = new unsigned char[200];
  int fd = open("/dev/psaux", O_RDWR);
  reset(fd);
  // If kernel<2.4.9 remember to disable datareport, 
  // in later kernels reset is enough
  disabledatareport(fd);
  sleep(1); // make sure we dont get any more confusing datareports.
  if(detectTP(fd)) {
    if(detectExternal(fd)) {
      enabletransparent(fd); 
      if(! detectIMPS(fd)) disabletransparent(fd);
      setsamplerate(fd,200);
    }
  } else
    detectIMPS(fd);
  
  enabledatareport(fd);
    
  close(fd);
}
