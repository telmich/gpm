// show effects of lost bytes on ps/2 mouse protocol 
// Later: largely extended for mouse autodetect etc. (c) gunther mayer 13.05.2001
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

/* S48a reagiert auf Versapad: protokoll auf 4byte intellimouse
	  dto. auf logicord

12.06.2001: MS PNP id 

 */

#define GEN3U 1
#define IM 2
#define GEN3D 3

int reset=1;
int mspnp=0;
int logi=1;
int logicord=0; // dies irritiert manchmal genius 3U direkt nach hotplug
int logi_extended=1;
int logi_extra=0; /* NO! */
int logi_extended_s48zilog=1;
int genius=0;
int testid=0;
int a4tech=0;
int intellimouse_explorer=0;
int intellimouse=0;
int alpsglidepoint=0;
int versapad=0;
int kensington=0;
int synaptics=0;
int cirque=0;

int streamnostream=1;

int consumefa(FILE *f)
{  int c=0xfa;

   while(c==0xfa) {
	c=fgetc(f);
	if(c==0xfa) printf(" 0xfa");
   }
   printf("\n");
   return c;
}

int sendbuf(int fd,FILE *f,char *buf,int num)
{
  int i,c,rep;

  for(i=0;i<num;i++) {
    write(fd,&buf[i],1);
    c=fgetc(f);
     if(c!=0xfa) printf("sent: %02x expected FA, got: %02x\n",buf[i],c);
     if((c==0xfe) &&!! (rep!=1)) { 
		printf("repeating...\n");
		rep=1;
		i--;
     }
     else rep=0;
  }
}





void gete9(int fd,FILE *f)
{  char buf[1]; int a,b,c,d;
 buf[0]=0xe9;
        write(fd,&buf,1);
        a=fgetc(f);
if(a==0xfe) { printf("FE\n"); write(fd,&buf,1); a=consumefa(f);}
        b=fgetc(f);
        c=fgetc(f);
        d=fgetc(f);
        printf("      e9 is %02x %02x %02x %02x\n",a,b,c,d);

printf("\n");
}

int ratetest(int fd,FILE *f)
{ int i,r,a,b,c,rates[]={10,20,40,60,80,100,200};
  char buf[3];

  for(i=0;i<7;i++) {
	r=rates[i];
	printf("setrate %d ",r);
	buf[0]=0xf3;
	buf[1]=r;
	buf[2]=0xe9;
	write(fd,&buf,3);
	a=consumefa(f);
	b=fgetc(f);
        c=fgetc(f);
	if(c==r) printf(" OK\n");
	else printf(" nono (%2x %2x %2x)\n",a,b,c);
  }
}


	
    

int main(void)
{ 
	FILE *f;
	int i=0,x,a,y, xo,yo,xs,ys,r,m,l,t;
	int xmin=0,xmax=0,ymin=0,ymax=0,z;
	signed int dx,dy;
	int xtot=0,ytot=0,dz,e,ee,d,b,fd,c;
	char buf[255];
	char logi_id[255]={ 0xe8, 0x00, 0xe6, 0xe6, 0xe6,0xe9};
	char logi_cordless_status[]={ 0xe8, 0x03, 0xe7, 0xe7, 0xe7,0xe9};
	char logi_extended_id[]={ 0xe6, 0xe8,0, 0xe8,3, 0xe8,2, 0xe8,1 };
        char logi_extended_s48zilog_id[]={ 0xe6, 0xe8,0, 0xe8,3, 0xe8,2, 0xe8,1 ,0xf3,200};
        char logi_extended_idb[]={ 0xe6, 0xe8,3, 0xe8,1, 0xe8,2, 0xe8,3};
	char genius_id[]={ 0xe8, 0x03, 0xe6, 0xe6, 0xe6, 0xe9};
	char test_id[]= {0xe6, 0xe6, 0xe6, 0xe9};
	char a4tech_id[]={ 0xf3,200, 0xf3,100, 0xf3,80, 0xf3,60, 0xf3,40, 0xf3,20};
	char mspnp_id[]={ 0xf3, 20, 0xf3,40, 0xf3,60};
	char intellimouse_id[]={0xf3,200, 0xf3,100, 0xf3,80};
	char intellimouse_explorer_id[]={ 0xf3,200, 0xf3,200, 0xf3,80};
	// following are untested 
	char alps_glidepoint_id[]={ 0xf3,100, 0xe8,0, 0xe7, 0xe7, 0xe7, 0xe9};
	char versapad_id[]={ 0xe8,2, 0xf3,100, 0xe6, 0xe6, 0xe6, 0xe6, 0xe9};
	char kensington_id[]={0xf3,10,0xf2};
	char kensington_enable[]={0xe8,0, 0xf3,20, 0xf3,60, 0xf3,40, 0xf3,20, 0xf3,20, 
				  0xf3,60, 0xf3,40, 0xf3,20, 0xf3,20, 0xe9};
	char synaptics_id[]={ 0xe6, 0xe8,0, 0xe8,0, 0xe8,0,0xe8,0, 0xe9 };
	char cirque_id[]={ /*0xe9*/ 0xf5, 0xf5, 0xe7, 0xe6, 0xe7, 0xe9}; //monitorintro
	// from kitcat-0.1.8 source: cirque smartcat uses 18 bytes for hires mode !
	//  kitcat has 2 reset command in prologue !?
	//undoc:
	char cirque_hires_1[]={ 0xf4, 0xf4, 0xf2 /*read ID*/, 0xf3,0x28, 0xf4, 0xf5,
			0xf2, 0xf4, 0xf3,0x0a, 0xf3, 0xc8, 0xe7, 0xe8,0x02, 0xf4,
			0xf3,0x3c, 0xf5, 0xf4, 0xf4 };
	//according to documentation. commented out in kitcat
	char cirq_hires_2[]= {0xf2, 0xe7, 0xe8,0x02, 0xf5, 0xf2, 0xf4, 0xf4};
	// in kitcat
	char cirq_hires[]={0xe8,0, 0xf3, 0xc8, 0xe8,0x03, 0xf2, 0xf4, 0xf5, /*0xff, 0xf4 */};

	int typ=0;
	f=fopen("/dev/misc/psaux","r");
	if(f==NULL) strerror(errno);

fd = open("/dev/misc/psaux", O_RDWR);


//	fputc(0xf3,f);
//	fputc(400,f);

//	fputc(3,f);

if(reset) {
        buf[0]=0xff;
        write(fd,&buf,1);
        //a=fgetc(f);
        printf("reset\n");
printf("sleep 1\n");
sleep(1);


}

if(mspnp) {
	printf("Test MS PNP ID  (this hangs currently on other types due to lack of timeout:\n");
	write(fd,mspnp_id,6);
        a=consumefa(f);
	printf("PnP id a is %x\n",a);
	for(i=0;i<7;i++) {
		a=fgetc(f);
		printf("PnP id %x\n",a);
	}
	// My "Intellimouse 1.3A PS/2 Compatible" (OEM product ID on bottom)
	// responds with these 7 bytes (which are too given in "moudep.c" in Win2000DDK):
	// 0x32 0x1f 0x23 0x0b 0x0b 0x0b 0x03
	// The DDK recognizes MSH0002, MSH0005, MSH0010 as Wheel-IDs
	// The DDK says: New Mice respond with:MSHxxxx, old style: pnpxxxx
	// Sic: these bytes are sent without a POLL_DATA or GET_MOUSE_ID command !!!

	//my other mice respond here too: (I manually found out the number of bytes)
	// Serial+MousePort Mouse 2.0=    19 99 31 b1 19 99 0B 8B 21 A1 0B 8B 30 B0
	   // WinDDK speaks about older mice which send make and break codes (7+7)
	// MousePort Mouse 2.1A(oem)= no response 
	// trekker two button mouse 2.0A= 19    31    19    0B    21    0B    30
	// BasicMouse serial+PS/2    =    19 99 31 b1 19 99 0b 8b 21 a1 0b 8b 30 b0
	// IntelliMouse Trackball 1.0=    32    1F    23    0B    0B    0B    06
	// IntelliMouseExplorer usb/ps2=  32    1F    23    0B    0B    02    21

	/* http://www.microsoft.com/norge/support/windows2000/articles/216570.asp
	                         MSH0001 &ndash; Standard toknapps Microsoft Mouse
                                 MSH0002 &ndash; Microsoft Wheel Mouse
                                 MSH0005 &ndash; Microsoft Track Ball Mouse
	*/
}
	
if(streamnostream) {
        buf[0]=0xf0;
        write(fd,&buf,1);
        a=fgetc(f);
        printf("Non-stream mode  is %x\n",a);
}


        buf[0]=0xf4;
        write(fd,&buf,1);
        a=fgetc(f);
        printf("enable is %x\n",a);


        buf[0]=0xf2;
        write(fd,&buf,1);
	b=consumefa(f);
	printf("ID(f2) is %x\n",b);

ratetest(fd,f);



        buf[0]=0xf3;
        write(fd,&buf,1);
        a=fgetc(f);
        buf[0]=200;
        write(fd,&buf,1);
        a=fgetc(f);


        buf[0]=0xe8;
        write(fd,&buf,1);
        a=fgetc(f);
        buf[0]=3;
        write(fd,&buf,1);
        a=fgetc(f);



        buf[0]=0xe6;
        write(fd,&buf,1);
	printf("e6 is %02x\n",fgetc(f));


	buf[0]=0xe9;
	write(fd,&buf,1);
	b=consumefa(f);
	c=fgetc(f);
	d=fgetc(f);
	printf("      e9 is %02x %02x %02x\n",b,c,d);

if(logi) {
	// others repond to this too!
	/*  e.g.doc7-pxz-p440-ds-110.pdf-USAR-pixiePoint--Pointing.stcik.pdf
	    byte0:standard  1:3(buttons) 2:firmware release
	 */
	int devicetype;
	write(fd,&logi_id,6);
	a=consumefa(f);
        b=fgetc(f);
        c=fgetc(f);
	devicetype = ((a >> 4) & 0x07) | ((a << 3) & 0x78);
        printf("logi e9 is %02x %02x %02x vojtech-devicetype=%d =0x%2x\n",a,b,c,devicetype,devicetype);
}

	printf("\n");

	gete9(fd,f);


if(logi_extended) { int g;
	write(fd,&logi_extended_id,9);
	buf[0]=0xeb;
	write(fd,&buf,1);
        a=consumefa(f);
if(a==0xfe) { printf("FE\n"); write(fd,&buf,1); a=consumefa(f); }
        b=fgetc(f);
        c=fgetc(f);
        printf("logi extended poll 1 eb is %02x %02x %02x\n",a,b,c);

        write(fd,&logi_extended_idb,9);
        buf[0]=0xeb;
        write(fd,&buf,1);
        a=consumefa(f);
if(a==0xfe) { printf("FE\n"); write(fd,&buf,1); a=consumefa(f); }
        b=fgetc(f);
        c=fgetc(f);
        printf("logi extended poll 2 eb is %02x %02x %02x\n",a,b,c);

if(logi_extra) {  // marble mouse sends 2 extra triples
	a=fgetc(f);
        b=fgetc(f);
        c=fgetc(f);
        d=fgetc(f);
        e=fgetc(f);
	ee=fgetc(f);
        printf("further is %02x %02x %02x %02x %02x %02x\n",a,b,c,d,e,ee);
}

if(logi_extended_s48zilog) { int g;
        write(fd,&logi_extended_s48zilog_id,11);
        buf[0]=0xeb;
        write(fd,&buf,1);
        a=consumefa(f);
	b=fgetc(f);
        c=fgetc(f);
	printf("logi extended_s48 zilo poll is %02x %02x %02x\n",a,b,c);
}




}

printf("\n");

buf[0]=0xe9;
        write(fd,&buf,1);
        a=fgetc(f);
if(a==0xfe) { printf("FE\n"); write(fd,&buf,1); a=consumefa(f);}
        b=fgetc(f);
        c=fgetc(f);
        d=fgetc(f);
        printf("      e9 is %02x %02x %02x %02x\n",a,b,c,d);

printf("\n");

if(logicord) {
 write(fd,&logi_cordless_status,6);
        a=fgetc(f);
        b=fgetc(f);
        c=fgetc(f);
        d=fgetc(f);
        e=fgetc(f);
        ee=fgetc(f);
        printf("logicord e9 is %02x %02x %02x %02x %02x %02x\n",a,b,c,d,e,ee);

	gete9(fd,f);
}

buf[0]=0xe9;
        write(fd,&buf,1);
        a=fgetc(f);
        b=fgetc(f);
        c=fgetc(f);
        d=fgetc(f);
        printf("      e9 is %02x %02x %02x %02x\n",a,b,c,d);

if(testid) {
	printf("\n");
	write(fd,&test_id,4);
	a=consumefa(f);
        b=fgetc(f);
        c=fgetc(f);
        printf("test id e9 is %02x %02x %02x\n",a,b,c);
	gete9(fd,f);
	printf("\n");
}
	

if(genius) {
write(fd,&genius_id,6);
	a=consumefa(f);
        b=fgetc(f);
        c=fgetc(f);
        printf("genius id e9 is %02x %02x %02x\n",a,b,c);
	if(c!=0xc8) {

	if((b==0x33) && (c==0x44)) {
		printf("autodetect GEN3D\n");
		typ=GEN3D; // Netscroll groller 6bytes
	}
	else if((b==0x33) && (c==0x55)) {
		printf("autodetect GEN3U\n");
		typ=GEN3U; // Netmouse 4 bytes
	}	
	gete9(fd,f);
	}

	ratetest(fd,f);

}

if(alpsglidepoint) {
write(fd,&alps_glidepoint_id,8);
	a=consumefa(f);
        b=fgetc(f);
        c=fgetc(f);

	if((b!=0) && (c!=100)) printf("Alps Glidepoint detected\n");
	  /* My Genius Newscroll locks rate=$3c (!50) !? */

	printf("alps glidepoint id e9 is %02x %02x %02x\n",a,b,c);
	gete9(fd,f);
}

if(versapad) {
write(fd,&versapad_id,9);
        a=consumefa(f);
        b=fgetc(f);
        c=fgetc(f);

	if((b==0) && (c==10)) printf("versapad detected\n");

        printf("versapad id e9 is %02x %02x %02x\n",a,b,c);
        gete9(fd,f);
}

if(kensington) {
write(fd,&kensington_id,3);
        a=consumefa(f);
	printf("kensington f3 id is %d\n",a);
	if(a==2) printf("AUTODETECT Kensington\n");

	write(fd,&kensington_enable,21);
	a=consumefa(f);
        b=fgetc(f);
        c=fgetc(f);

	printf("kensingon enable =  %02x %02x %02x\n",a,b,c);

	// beware of GENIUS: 3U locks rate at 100; 3D at 20-60	
	if((b!=0) &&  (c!=20)) printf("AUTODETECT Kensington enable\n");


	printf("\n");
}



if(intellimouse){ 
write(fd,&intellimouse_id,6);
        buf[0]=0xf2;
        write(fd,&buf,1);
        b=consumefa(f);
        printf("Intellimouse ID(f2) is %x\n",b);
	if(b==3) {
	printf("autodetect is:typ=IM;\n");
	typ=IM;
	}
}
if(intellimouse_explorer) {
write(fd,&intellimouse_explorer_id,6);
        buf[0]=0xf2;
        write(fd,&buf,1);
        b=consumefa(f);
        printf("Intellimouse Explorer ID(f2) is %x\n",b);
	if(b==4) {
	 printf("autodetect is:typ=IM;\n");
	typ=IM;
	}
}

if(synaptics) {
write(fd,&synaptics_id,10);
        a=consumefa(f);
        b=fgetc(f);
        c=fgetc(f);

	printf("synaptics_id is %02x %02x %02x\n",a,b,c);
	if(b==0x47) printf("AUTODETECT Synaptics\n");
	gete9(fd,f);
}

if(cirque) {
	printf("Before cirque\n");
	gete9(fd,f);
sendbuf(fd,f,cirque_id,6);
        a=consumefa(f);
        b=fgetc(f);
        c=fgetc(f);

        printf("cirque_id is %02x %02x %02x\n",a,b,c);

	if(a==0x88) printf("AUTODETECT Cirque\n"); // my smartcat has:  88 03 02

        gete9(fd,f);
	gete9(fd,f);
}


if(a4tech) {
sendbuf(fd,f,a4tech_id,12);
	buf[0]=0xf2;
        write(fd,&buf,1);
        b=consumefa(f);
        printf("a4tech ID(f2) is %x\n",b);

	if(b==6 || b==8) printf("AUTODETECT: a4tech\n");
	// b=6: spiffy gyro-mouse "8D Profi-Mouse Point Stick"
	// b=8: boeder Smartmouse Pro (4Button, 2Scrollwheel, 520dpi) PSM_4DPLUS_ID MOUSE_MODEL_4DPLUS
}

	buf[0]=0xf2;
        write(fd,&buf,1);
        b=consumefa(f);
        printf(      " ID(f2) is %x\n",b);

streamnostream=1;
if(streamnostream) {
        buf[0]=0xea;
        write(fd,&buf,1);
        a=fgetc(f);
        printf("Activating stream mode %x\n",a);
}

        buf[0]=0xf4;
        write(fd,&buf,1);
        a=fgetc(f);
        printf("Enable mouse%x\n",a);

	//scale 
        buf[0]=0xe6;
        write(fd,&buf,1);
        a=fgetc(f);


	// resolution
        buf[0]=0xe8;
        write(fd,&buf,1);
        a=fgetc(f);
        buf[0]=2;
        write(fd,&buf,1);
      	a=fgetc(f);

	ratetest(fd,f);

        // rate
        buf[0]=0xf3;
        write(fd,&buf,1);
        a=fgetc(f);
        buf[0]=100;
        write(fd,&buf,1);
        a=fgetc(f);


buf[0]=0xe9;
        write(fd,&buf,1);
        a=fgetc(f);
        b=fgetc(f);
        c=fgetc(f);
        d=fgetc(f);
        printf("      e9 is %02x %02x %02x %02x\n",a,b,c,d);




 //printf("Simulating 2 lost bytes\n");
 //a=fgetc(f); a=fgetc(f);

// printf("Simulating 1 lost byte\n");
 //a=fgetc(f);

printf("autodetect has typ=%d\n",typ);

	while (1) {
		a=fgetc(f);
		x=fgetc(f);
		y=fgetc(f);
if(typ==GEN3U || typ==IM) z=fgetc(f);
		i++;
		printf("%5d -- %02x %02x %02x",i,a,x,y);
if(typ==GEN3U || typ==IM)  printf(" %02x",z);

		yo=a&0x80;
		xo=a&0x40;
		ys=a&0x20;
		xs=a&0x10;
		t=a&0x08;
		m=a&0x04;
		r=a&0x02;
		l=a&0x01;

		dx=x;
		dy=y;

		if(xs) dx= (~255) | dx; /* 9bit 2complement sign extension to signed integer */
		if(ys) dy= (~255) | dy;

		if (dx<xmin) xmin=dx;
		if (dy<ymin) ymin=dy;
		if (dx>xmax) xmax=dx;
		if (dy>ymax) ymax=dy;

	xtot=xtot+dx; /* total count */
	ytot=ytot+dy;


        // recognize mouseman+ packets aka PS2PP or PS2++ protocol
        if( ((a&0x48) == 0x48) && // bit3 and bit6 set
            ((x&3)  == 0x02) && // bit0 unset, bit1 set
            (((x>>1)&0x40) == (x&0x40)) && // bit 7 = bit 6
            ( ((x>>7)&0x01) != ((a>>4)&0x01)) && // neg of p2
            ( ((x>>2)&0x03) == (y&0x03))) {// these 2 bits equal
                int packettype;
         packettype= ((x>>4) &0x03) | ((a>>2)&0x0c);
        printf("     ps2++ packet type=0x%0x  ",packettype);

	switch (packettype) {
	      case 0:
		printf("ID Data packet, model=0x%02x (decimal=%d)",y,y);
		break;
              case 1: // wheel data  + BTN4+5 +Horizontal/vertical
		dz = (y&0x07) - (y&0x08); //2s complement
		printf("wheel: %d ",dz);
		if(y&0x10) printf("Btn4 ");
		if(y&0x20) printf("Btn3");
		break;
              case 2:         /* Logitech reserves this packet type */
                    /*   Xfree86 4.0.2
                     * IBM ScrollPoint uses this packet to encode its
                     * stick movement.
                     *
                    buttons |= (pMse->lastButtons & ~0x07);
                    dx = dy = 0;
                    dz = (pBuf[2] & 0x80) ? ((pBuf[2] >> 4) & 0x0f) - 16 :
                                            ((pBuf[2] >> 4) & 0x0f);
                    dw = (pBuf[2] & 0x08) ? (pBuf[2] & 0x0f) - 16 :
                                            (pBuf[2] & 0x0f);
			*/
                    break;

	      case 3: 
		printf("touchpad packet");
		break;
	      case 4:
	      case 5:
	      case 6:
	      case 8:
	      case 9:
		printf("cordless connect packet");
		break;
	      case 0x0b:
		printf("marble mouse or trackball packet");
        }
	printf("\n");
	}
else {


		printf(" ---   %4d : %4d   Buttons=%s%s%s Overflow/errors:%s%s%s ",
			dx,dy,m?"M":" ", r?"R":" ", l?"L":" ",
			xo?"X":" ", yo?"Y":" ", t?" ":"T");

		printf(" Min/Max x=(%d , %d) y=(%d , %d) total=%d,%d\n",xmin,xmax,ymin,ymax,xtot,ytot);
	}	


	    
	}	
	return 0;
}	
