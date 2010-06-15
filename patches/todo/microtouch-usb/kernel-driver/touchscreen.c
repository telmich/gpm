/* -*- linux-c -*- */

/* 
 * Driver for USB Touchscreen (Microtech - IBM SurePos 4820)
 *
 * Copyright (C) 2000 Wojciech Woziwodzki
 * Written by Radoslaw Garbacz
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Originally based upon scanner.c (David E. Nelson).
 *
 * History
 *
 *  0.1  06/05/2000 (RGA)
 *    Development work was begun.
 *
 *  0.2  09/05/2000 (RGA)
 *    Documentation about MicroTouch controller was arrived.
 *
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <asm/uaccess.h>
#include <linux/init.h>
#include <linux/malloc.h>
#include <linux/delay.h>
#include <linux/ioctl.h>
#include <linux/config.h>
#include <linux/interrupt.h>
#include <linux/ptrace.h>
#include <linux/poll.h>

#include <linux/in.h>
#include <linux/malloc.h>
#include <linux/tty.h>
#include <linux/errno.h>
#include <linux/string.h>	/* used in new tty drivers */
#include <linux/signal.h>	/* used in new tty drivers */
#include <asm/system.h>
#include <asm/bitops.h>
#include <asm/termios.h>
#include <linux/if.h>

#ifdef CONFIG_KERNELD
#include <linux/kerneld.h>
#endif

//#define DEBUG
#define TSCRN_IOCTL
//#define __TEST_NO_DEVICE__

#define TSCRN_MAX_MNR 16		/* We're allocated 16 minors */
#define TSCRN_BASE_MNR 48		/* USB Scanners start at minor 48 */


#define IS_EP_BULK(ep)  ((ep).bmAttributes == USB_ENDPOINT_XFER_BULK ? 1 : 0)
#define IS_EP_BULK_IN(ep) (IS_EP_BULK(ep) && ((ep).bEndpointAddress & USB_ENDPOINT_DIR_MASK) == USB_DIR_IN)
#define IS_EP_BULK_OUT(ep) (IS_EP_BULK(ep) && ((ep).bEndpointAddress & USB_ENDPOINT_DIR_MASK) == USB_DIR_OUT)
#define IS_EP_INTR(ep) ((ep).bmAttributes == USB_ENDPOINT_XFER_INT ? 1 : 0)
#define IS_EP_INTR_IN(ep) (IS_EP_INTR(ep) && ((ep).bEndpointAddress & USB_ENDPOINT_DIR_MASK) == USB_DIR_IN)
#define IS_EP_INTR_OUT(ep) (IS_EP_INTR(ep) && ((ep).bEndpointAddress & USB_ENDPOINT_DIR_MASK) == USB_DIR_OUT)

#define USB_TSCRN_MINOR(X) MINOR((X)->i_rdev) - TSCRN_BASE_MNR

#ifdef DEBUG
#define TSCRN_DEBUG(X) X
#else
#define TSCRN_DEBUG(X)
#endif

#include <linux/usb.h>
#include <linux/input.h>
#include "touchscreen.h"

struct tscrn_usb_data
{
  struct usb_device *dev;
  wait_queue_head_t wait;
  struct urb irq;
  struct urb ctrlin;
  struct urb ctrlout;
  devrequest *setup_packet;     // for control transfers 
  unsigned int ifnum;	        // Interface number of the USB device 
  kdev_t minor;	                // TouchScreen minor - used in disconnect() 
  unsigned char data[TSCRN_USB_RAPORT_SIZE_DATA];       // Data buffer X-Y, and button 
  int nLoopCounter;             // number of data packet last received +1
  char isopen;		        // Not zero if the device is open 
  char present;		        // Not zero if device is present 
  unsigned char *obuf, *ibuf;	// transfer buffers 
  unsigned char *pToRead,*pToWrite;// pointers to toRead cell of buffer and to toWrite one 
  char intr_ep;                 // Endpoint assignments 
  struct input_dev input_dev;   // to work as an input device driver
};

static struct tscrn_usb_data *p_tscrn_table[TSCRN_MAX_MNR] = { NULL, /* ... */};

MODULE_AUTHOR("Radoslaw Garbacz, garbacz@posexperts.com.pl");
MODULE_DESCRIPTION("USB touchscreen driver");

static __s32 vendor=-1, product=-1;
MODULE_PARM(vendor, "i");
MODULE_PARM_DESC(vendor, "User specified USB idVendor");

MODULE_PARM(product, "i");
MODULE_PARM_DESC(product, "User specified USB idProduct");

/* Forward declarations */
static struct usb_driver touchscreen_driver;

/* Procedures */
/**
 * Runs after ctrl requests.
 */
static void ctrl_touchscreen(struct urb *urb)
{
  struct tscrn_usb_data *tscrn = urb->context;
  dbg("ctrl_touchscreen(%d): status=%d", tscrn->minor, urb->status);

  if (waitqueue_active(&tscrn->wait))
    wake_up_interruptible(&tscrn->wait);
  return;
};

/**
 * Runs on new data received from device
 * The buffer should keep the last state of device, thus the buffer overflow
 * can occurre
 * The client can't read data when pToRead == pToWrite
 */
static void irq_touchscreen(struct urb *urb)
{
  /*
   * data raports...
   */
  struct tscrn_usb_data *tscrn = urb->context;
  struct tscrn_usb_data_report *data = (struct tscrn_usb_data_report *)tscrn->data;

  if (urb->status) return;

  //return when driver was clesed 
  if(!tscrn->isopen) return;

  // increase loop counter
  // !!! Unfortunately the data was lost to seldom !!!
  //if((int)GET_LOOP(data) != tscrn->nLoopCounter)
  //  warn("warn:Lost data new loop %d previous loop %d",(int)GET_LOOP(data),(int)tscrn->nLoopCounter);

  input_report_key(&tscrn->input_dev, BTN_LEFT, IS_TOUCHED(data));
  input_report_abs(&tscrn->input_dev, ABS_X, GET_XC(data));
  input_report_abs(&tscrn->input_dev, ABS_Y, GET_YC(data));

  tscrn->nLoopCounter = (GET_LOOP(data))+1; 

  // store data to buffer
  memcpy(tscrn->pToWrite,data,TSCRN_USB_RAPORT_SIZE_DATA);
  // next data to next cell
  if(tscrn->pToRead == NULL)
    tscrn->pToRead = tscrn->pToWrite;

  tscrn->pToWrite += TSCRN_USB_RAPORT_SIZE_DATA;
  if(tscrn->pToWrite >= tscrn->obuf+OBUF_SIZE)
    tscrn->pToWrite = tscrn->obuf;

  if (waitqueue_active(&tscrn->wait))
    wake_up_interruptible(&tscrn->wait);

  //dbg("irq_touchscreen(): GET_Data 0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x status=%d", (int)data[0],(int)data[1],(int)data[2],(int)data[3],(int)data[4],(int)data[5],(int)data[6],(int)data[7],(int)data[8],(int)data[9],(int)data[10], tscrn->ctrlout.status);

  return;
}


static int open_touchscreen(struct inode * inode, struct file * file)
{
  struct tscrn_usb_data *tscrn;
  struct usb_device *dev;

  kdev_t minor;

#ifdef __TEST_NO_DEVICE__
  return -ENODEV;
#endif 

  minor = USB_TSCRN_MINOR(inode);

  dbg("open_touchscreen: tscrn_minor:%d", minor);

  if (!p_tscrn_table[minor])
  {
    err("open_touchscreen(%d): invalid tscrn_minor", minor);
    return -ENODEV;
  }

  tscrn = p_tscrn_table[minor];

  dev = tscrn->dev;

  if (!dev)
  {
    return -ENODEV;
  }

  if (!tscrn->present) {
    return -ENODEV;
  }

  if (tscrn->isopen) {
    return -EBUSY;
  }

  //clear up the data buffer
  memset(tscrn->obuf, 0x0, OBUF_SIZE);
  tscrn->pToRead = NULL;
  tscrn->pToWrite = tscrn->obuf;

  tscrn->isopen = 1;

  file->private_data = tscrn; /* Used by the read and write metheds */

  MOD_INC_USE_COUNT;

  return 0;
}

static int close_touchscreen(struct inode * inode, struct file * file)
{
  struct tscrn_usb_data *tscrn;

  kdev_t minor;
#ifdef __TEST_NO_DEVICE__
  return -ENODEV;
#endif 

  minor = USB_TSCRN_MINOR (inode);

  dbg("close_touchscreen: tscrn_minor:%d", minor);

  if (!p_tscrn_table[minor]) {
    err("close_touchscreen(%d): invalid tscrn_minor", minor);
    return -ENODEV;
  }

  tscrn = p_tscrn_table[minor];

  tscrn->isopen = 0;

  file->private_data = NULL;

  MOD_DEC_USE_COUNT;

  return 0;
}

static ssize_t read_touchscreen(struct file * file, char * buffer,
    size_t count, loff_t *ppos)
{
  struct tscrn_usb_data *tscrn;
  struct usb_device *dev;

  ssize_t bytes_read;	/* Overall count of bytes_read */
  ssize_t ret;

  kdev_t minor;

  int partial;		/* Number of bytes successfully read */
  int this_read;		/* Max number of bytes to read */
  //int result;

  unsigned char *buf, *pToRead;

#ifdef __TEST_NO_DEVICE__
  return -ENODEV;
#endif 

  tscrn = file->private_data;
  minor = tscrn->minor;
  buf = tscrn->obuf;
  pToRead = tscrn->pToRead;
  dev = tscrn->dev;

  bytes_read = 0;
  ret = 0;

  if((tscrn->pToWrite == tscrn->pToRead) || (tscrn->pToRead == NULL))
    return 0;   // no new data

  if (signal_pending(current))
  {
    ret = -EINTR;
    return ret;
  }

  this_read = (count >= TSCRN_USB_RAPORT_SIZE_DATA) ? TSCRN_USB_RAPORT_SIZE_DATA: count;

  partial = this_read;
  //dbg("read stats(%d): result:%d this_read:%d partial:%d", minor, result, this_read, partial);
  dbg("read count=%d", count);

  if (partial)
  { /* Data returned */
    if (copy_to_user(buffer, tscrn->pToRead, this_read))
    {
      ret = -EFAULT;
      return ret;//break;
    }
    count -= partial;
    bytes_read += partial;
    buffer += partial;
    tscrn->pToRead += partial;
    if(tscrn->pToRead >= tscrn->obuf+OBUF_SIZE)
      tscrn->pToRead = tscrn->obuf;
  }

  dbg("read ended bytes_read(%d)", bytes_read);

  return ret ? ret : bytes_read;
}

/**
* Zmiana dla j±dra 2.2.x z <code>select</code> na <code>poll</code>
*/
static unsigned int poll_touchscreen(struct file *file, struct poll_table_struct *_wait)
{
  struct usb_device *dev;
  struct tscrn_usb_data *tscrn;
  struct inode *inode;
  struct dentry *dentry;

  kdev_t minor;

  dentry = file->f_dentry;
  inode  = dentry->d_inode;
  minor = USB_TSCRN_MINOR(inode);
  

  dbg("poll_touchscreen(%d): entered", minor);

  if (!p_tscrn_table[minor])
  {
    err("poll_touchscreen(%d): invalid minor", minor);
    return 0;
    //return -ENODEV;
  };

  tscrn = p_tscrn_table[minor];
  dev   = tscrn->dev;

  //if(_selType != SEL_IN)
  //  return -EPERM;

  if((tscrn->pToWrite == tscrn->pToRead) || (tscrn->pToRead == NULL))
  {
    poll_wait(file, &tscrn->wait, _wait);
    return 0;
  };

  return 1;
};

static int ioctl_touchscreen(struct inode *inode, struct file *file,
    unsigned int cmd, unsigned long arg)
{
  struct usb_device *dev;
  struct tscrn_usb_data *tscrn;

  int nRet;

  kdev_t minor;

#ifdef __TEST_NO_DEVICE__
  return -ENODEV;
#endif 
  minor = USB_TSCRN_MINOR(inode);

  dbg("ioctl_touchscreen(%d): entered cmd=%d", minor,cmd);

  if (!p_tscrn_table[minor])
  {
    err("ioctl_touchscreen(%d): invalid minor", minor);
    return -ENODEV;
  }

  tscrn = p_tscrn_table[minor];
  dev   = tscrn->dev;

  switch (cmd)
  {
    case TSCRN_USER_REQUEST:
      {
        /* sends reset command */
        struct
        {
          __u8  data;
          __u8  request;
          __u16 value;
          __u16 index;
        } args;

        if (copy_from_user(&args, (void *)arg, sizeof(args)))
          return -EFAULT;

        // for the present a soft reset
        tscrn->setup_packet->requesttype = args.data;
        tscrn->setup_packet->request = args.request;
        tscrn->setup_packet->value = args.value;
        tscrn->setup_packet->index = args.index;
        tscrn->setup_packet->length = 0;
        FILL_CONTROL_URB(&tscrn->ctrlout, dev, 
            usb_sndctrlpipe(dev, 0), (unsigned char *)tscrn->setup_packet,
            NULL, 0, ctrl_touchscreen, tscrn);

        if ((nRet=usb_submit_urb(&tscrn->ctrlout)))
        {
          err("ioctl_touchscreen(%d): errror=%d status=%d.", minor, nRet, tscrn->ctrlout.status);
          return nRet;
        }
        dbg("ioctl_touchscreen(%d): cmd status=%d nRet=%d",minor,tscrn->ctrlout.status,nRet);
        interruptible_sleep_on(&tscrn->wait);
        dbg("ioctl_touchscreen(%d): cmd status=%d", minor, tscrn->ctrlout.status);
        return tscrn->ctrlout.status;
      };
    case TSCRN_CLEAR_HALT:
      {
        // when end_point is STALLED always returned -1 at first 
        // and OK on second exec, then end_point returns to normal state
        return usb_clear_halt(dev,0x00);//0x80
      };
    case TSCRN_HARD_RESET:
      {
        tscrn->setup_packet->requesttype = USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE;
        tscrn->setup_packet->request = TSCRN_USB_REQUEST_RESET;
        tscrn->setup_packet->value = TSCRN_USB_PARAM_HARD_RESET;
        tscrn->setup_packet->index = 0;
        tscrn->setup_packet->length = 0;
        FILL_CONTROL_URB(&tscrn->ctrlout, dev, 
            usb_sndctrlpipe(dev, 0), (unsigned char *)tscrn->setup_packet,
            NULL, 0, ctrl_touchscreen, tscrn);

        if ((nRet=usb_submit_urb(&tscrn->ctrlout)))
        {
          err("ioctl_touchscreen(%d): errror=%d status=%d.", minor, nRet, tscrn->ctrlout.status);
          return nRet;
        }
        dbg("ioctl_touchscreen(%d): status=%d nRet=%d",minor,tscrn->ctrlout.status,nRet);
        interruptible_sleep_on(&tscrn->wait);
        dbg("ioctl_touchscreen(%d): status=%d", minor, tscrn->ctrlout.status);

        return tscrn->ctrlout.status;
      };
    case TSCRN_SOFT_RESET:
      {
        tscrn->setup_packet->requesttype = USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE;
        tscrn->setup_packet->request = TSCRN_USB_REQUEST_RESET;
        tscrn->setup_packet->value = TSCRN_USB_PARAM_SOFT_RESET;
        tscrn->setup_packet->index = 0;
        tscrn->setup_packet->length = 0;
        FILL_CONTROL_URB(&tscrn->ctrlout, dev, 
            usb_sndctrlpipe(dev, 0), (unsigned char *)tscrn->setup_packet,
            NULL, 0, ctrl_touchscreen, tscrn);

        if ((nRet=usb_submit_urb(&tscrn->ctrlout)))
        {
          err("ioctl_touchscreen(%d): errror=%d status=%d.", minor, nRet, tscrn->ctrlout.status);
          return nRet;
        }
        dbg("ioctl_touchscreen(%d): status=%d nRet=%d",minor,tscrn->ctrlout.status,nRet);
        interruptible_sleep_on(&tscrn->wait);
        dbg("ioctl_touchscreen(%d): status=%d", minor, tscrn->ctrlout.status);

        return tscrn->ctrlout.status;
      };
    case TSCRN_CONTROLLER_STATUS:
      {
        tscrn->setup_packet->requesttype = USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE;
        tscrn->setup_packet->request = TSCRN_USB_REQUEST_STATUS;
        tscrn->setup_packet->value = 0;
        tscrn->setup_packet->index = 0;
        tscrn->setup_packet->length = TSCRN_USB_RAPORT_SIZE_STATUS;
        memset(tscrn->ibuf,0x0, IBUF_SIZE);
        FILL_CONTROL_URB(&tscrn->ctrlin, dev, 
            usb_rcvctrlpipe(dev, 0x80), (unsigned char *)tscrn->setup_packet,
            tscrn->ibuf, TSCRN_USB_RAPORT_SIZE_STATUS, ctrl_touchscreen, tscrn);

        if ((nRet=usb_submit_urb(&tscrn->ctrlin)))
        {
          err("ioctl_touchscreen(%d): errror=%d status=%d.", minor, nRet, tscrn->ctrlin.status);
          return nRet;
        }
        dbg("ioctl_touchscreen(%d): status=%d", minor, tscrn->ctrlin.status);
        interruptible_sleep_on(&tscrn->wait);
        dbg("ioctl_touchscreen(%d): gets 0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x status=%d nRet=%d", minor,(int)tscrn->ibuf[0],(int)tscrn->ibuf[1],(int)tscrn->ibuf[2],(int)tscrn->ibuf[3],(int)tscrn->ibuf[4],(int)tscrn->ibuf[5],(int)tscrn->ibuf[6],(int)tscrn->ibuf[7], tscrn->ctrlin.status, nRet);

        if(tscrn->ctrlin.status == 0)
          if (copy_to_user((void *)arg, tscrn->ibuf, TSCRN_USB_RAPORT_SIZE_STATUS))
            return -EFAULT;

        return tscrn->ctrlin.status;
      };
    case TSCRN_CONTROLLER_ID:
      {
        tscrn->setup_packet->requesttype = USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE;
        tscrn->setup_packet->request = TSCRN_USB_REQUEST_CONTROLLER_ID;
        tscrn->setup_packet->value = 0;
        tscrn->setup_packet->index = 0;
        tscrn->setup_packet->length = TSCRN_USB_RAPORT_SIZE_ID;
        memset(tscrn->ibuf,0x0, IBUF_SIZE);
        FILL_CONTROL_URB(&tscrn->ctrlin, dev, 
            usb_rcvctrlpipe(dev, 0x80), (unsigned char *)tscrn->setup_packet,
            tscrn->ibuf, TSCRN_USB_RAPORT_SIZE_ID, ctrl_touchscreen, tscrn);

        if ((nRet=usb_submit_urb(&tscrn->ctrlin)))
        {
          err("ioctl_touchscreen(%d): errror=%d status=%d.", minor, nRet, tscrn->ctrlin.status);
          return nRet;
        }
        dbg("ioctl_touchscreen(%d): status=%d", minor, tscrn->ctrlin.status);
        interruptible_sleep_on(&tscrn->wait);
        dbg("ioctl_touchscreen(%d): gets 0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x status=%d nRet=%d", minor,(int)tscrn->ibuf[0],(int)tscrn->ibuf[1],(int)tscrn->ibuf[2],(int)tscrn->ibuf[3],(int)tscrn->ibuf[4],(int)tscrn->ibuf[5],(int)tscrn->ibuf[6],(int)tscrn->ibuf[7], tscrn->ctrlin.status, nRet);

        if(tscrn->ctrlin.status == 0)
          if (copy_to_user((void *)arg, tscrn->ibuf, TSCRN_USB_RAPORT_SIZE_ID))
            return -EFAULT;

        return tscrn->ctrlin.status;
      };
    case TSCRN_CALIBRATION:
      {
        __u16  type;

        if (copy_from_user(&type, (void *)arg, sizeof(type)))
        {
          err("ioctl_touchscreen(%d): copy from user error arg=%d.", minor, *((int *)arg));
          return -EFAULT;
        };

        switch(type)
        {
          case TSCRN_EXTENDED_CALIBRATION_TYPE:
          case TSCRN_CORNER_CALIBRATION_TYPE:
            break;
          default:
            err("ioctl_touchscreen(%d): unknown calibration type %d.", minor, type);
            return -EFAULT;

        };
        tscrn->setup_packet->requesttype = USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE;
        tscrn->setup_packet->request = TSCRN_USB_REQUEST_CALIBRATION;
        tscrn->setup_packet->value = type;
        tscrn->setup_packet->index = 0;
        tscrn->setup_packet->length = 0;
        FILL_CONTROL_URB(&tscrn->ctrlout, dev, 
            usb_sndctrlpipe(dev, 0x0), (unsigned char *)tscrn->setup_packet,
            NULL, 0, ctrl_touchscreen, tscrn);

        if ((nRet=usb_submit_urb(&tscrn->ctrlout)))
        {
          err("ioctl_touchscreen(%d): errror=%d status=%d.", minor, nRet, tscrn->ctrlout.status);
          return nRet;
        }
        dbg("ioctl_touchscreen(%d): status=%d nRet=%d", minor, tscrn->ctrlout.status, nRet);
        interruptible_sleep_on(&tscrn->wait);
        dbg("ioctl_touchscreen(%d): status=%d", minor, tscrn->ctrlout.status);

        return tscrn->ctrlout.status;
      };
    case TSCRN_RESTORE_DEFAULTS:
      {
        tscrn->setup_packet->requesttype = USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE;
        tscrn->setup_packet->request = TSCRN_USB_REQUEST_RESTORE_DEFAULTS;
        tscrn->setup_packet->value = 0;
        tscrn->setup_packet->index = 0;
        tscrn->setup_packet->length = 0;
        FILL_CONTROL_URB(&tscrn->ctrlout, dev, 
            usb_sndctrlpipe(dev, 0x0), (unsigned char *)tscrn->setup_packet,
            NULL, 0, ctrl_touchscreen, tscrn);

        if ((nRet=usb_submit_urb(&tscrn->ctrlout)))
        {
          err("ioctl_touchscreen(%d): errror=%d status=%d.", minor, nRet, tscrn->ctrlout.status);
          return nRet;
        }
        dbg("ioctl_touchscreen(%d): status=%d nRet=%d", minor, tscrn->ctrlout.status, nRet);
        interruptible_sleep_on(&tscrn->wait);
        dbg("ioctl_touchscreen(%d): status=%d", minor, tscrn->ctrlout.status);

        return tscrn->ctrlout.status;
      };
    default:
      return -ENOIOCTLCMD;
  }
  return 0;
}

///////////////////////////////////////////////////////////////////////////
static void * probe_touchscreen(struct usb_device *dev, unsigned int ifnum)
{
  struct tscrn_usb_data *tscrn;
  struct usb_interface_descriptor *interface;
  struct usb_endpoint_descriptor *endpoint;

#ifdef DEBUG
  int nRet,nCounter;
#endif
  //devrequest *setup_packet;

  int ep_cnt;

  kdev_t minor;

  char valid_device = 0;
  char have_intr;

  if (vendor != -1 && product != -1) 
  {
    info("probe_touchscreen: User specified USB touchscreen -- Vendor:Product - %x:%x", vendor, product);
  }

  /*
   * 1. Check Vendor/Product
   * 2. Determine/Assign Bulk Endpoints
   * 3. Determine/Assign Intr Endpoint
   */

  /* 
   *
   * NOTE: Just because a product is supported here does not mean that
   * applications exist that support the product.  It's in the hopes
   * that this will allow developers a means to produce applications
   * that will support USB products.
   *
   * Until we detect a device which is pleasing, we silently punt.
   */

  if(	dev->descriptor.idVendor == MICROTOUCH_VENDOR_ID
      && dev->descriptor.idProduct == MICROTOUCH_PRODUCT_ID
    )
    valid_device = 1;
  // no user specification devices !!!
  // --   else
  // --   /* User specified */
  // --   if (dev->descriptor.idVendor == vendor
  // --       &&  dev->descriptor.idProduct == product)
  // --       valid_device = 1;

  if (!valid_device)	
    return NULL;	/* We didn't find anything pleasing */


  if(ifnum != 0)
  {
    info("probe_touchscreen: not correct interface no.");
    return NULL;
  }
  /*
   * After this point we can be a little noisy about what we are trying to
   *  configure.
   */

  if (dev->descriptor.bNumConfigurations != 1)
  {
    info("probe_touchscreen: Only one device configuration is supported.");
    return NULL;
  }

  if (dev->config[0].bNumInterfaces != 1)
  {
    info("probe_touchscreen: Only one device interface is supported.");
    return NULL;
  }

  interface = dev->config[0].interface[ifnum].altsetting;
  //interface = dev->actconfig[0].interface[ifnum].altsetting;
  endpoint = interface[ifnum].endpoint;

  /* 
   * Start checking for two bulk endpoints OR two bulk endpoints *and* one
   * interrupt endpoint. If we have an interrupt endpoint go ahead and
   * setup the handler. FIXME: This is a future enhancement...
   */

  dbg("probe_touchscreen: Number of Endpoints:%d", (int) interface->bNumEndpoints);

  if (interface->bNumEndpoints != 1)
  {
    info("probe_touchscreen: Only one endpoints supported.");
    return NULL;
  }

  //ep_cnt = have_bulk_in = have_bulk_out = have_intr = 0;
  ep_cnt = have_intr = 0;

  if (IS_EP_INTR(endpoint[ep_cnt]))
  {
    have_intr = ++ep_cnt;
    dbg("probe_touchscreen: intr_ep:%d", have_intr);
  }
  else
  {
    info("probe_touchscreen: Undetected endpoint. Notify the maintainer.");
    return NULL;	/* Shouldn't ever get here unless we have something weird */
  }

  /* 
   * Determine a minor number and initialize the structure associated
   * with it.  The problem with this is that we are counting on the fact
   * that the user will sequentially add device nodes for the touchscreen
   * devices.  */

  /*
   * Use firs free file devices
   */
  for (minor = 0; minor < TSCRN_MAX_MNR; minor++)
  {
    if (!p_tscrn_table[minor])
      break;
  }

  /* Check to make sure that the last slot isn't already taken */
  if (p_tscrn_table[minor])
  {
    err("probe_touchscreen: No more minor devices remaining.");
    return NULL;
  }

  dbg("probe_touchscreen: Allocated minor:%d", minor);

  if (!(tscrn = kmalloc (sizeof (struct tscrn_usb_data), GFP_KERNEL)))
  {
    err("probe_touchscreen: Out of memory.");
    return NULL;
  }
  memset (tscrn, 0, sizeof(struct tscrn_usb_data));
  dbg ("probe_touchscreen(%d): Address of tscrn:%p", minor, tscrn);

  /* Ok, now initialize all the relevant values */
  if (!(tscrn->obuf = (char *)kmalloc(OBUF_SIZE, GFP_KERNEL)))
  {
    err("probe_touchscreen(%d): Not enough memory for the output buffer.", minor);
    kfree(tscrn);
    return NULL;
  }
  dbg("probe_touchscreen(%d): obuf address:%p", minor, tscrn->obuf);

  if (!(tscrn->ibuf = (char *)kmalloc(IBUF_SIZE, GFP_KERNEL)))
  {
    err("probe_touchscreen(%d): Not enough memory for the input buffer.", minor);
    kfree(tscrn->obuf);
    kfree(tscrn);
    return NULL;
  }
  dbg("probe_touchscreen(%d): ibuf address:%p", minor, tscrn->ibuf);

  if (!(tscrn->setup_packet = (void *)kmalloc(sizeof(*tscrn->setup_packet), GFP_KERNEL)))
  {
    err("probe_touchscreen(%d): Not enough memory for the statuc.", minor);
    kfree(tscrn->ibuf);
    kfree(tscrn->obuf);
    kfree(tscrn);
    return NULL;
  }
  dbg("probe_touchscreen(%d): setup_packet address:%p, size=%d size2=%d", minor, tscrn->setup_packet, sizeof(*tscrn->setup_packet),sizeof(devrequest));
  memset((char *)tscrn->setup_packet, 0 , sizeof(*tscrn->setup_packet));

  // Initiation input_dev structure 
  tscrn->input_dev.evbit[0] = BIT(EV_KEY) | BIT(EV_ABS);
  tscrn->input_dev.keybit[LONG(BTN_MOUSE)] = BIT(BTN_LEFT);//| BIT(BTN_RIGHT) | BIT(BTN_MIDDLE);
  tscrn->input_dev.absbit[0] = BIT(ABS_X) | BIT(ABS_Y);

  // Initialization toRead and toWrite variables
  tscrn->pToWrite = tscrn->obuf;
  tscrn->pToRead  = NULL;

  // Initiation wait queue
  dbg("probe_touchscreen(%d): initialization wait_queue", minor);
  init_waitqueue_head(&tscrn->wait);
  dbg("probe_touchscreen(%d): wait_queue initialized", minor);

#ifdef DEBUG
  ///////////////////////////////////////////////////////////////////////////////////
  // Only for testing, not for normal useing of device driver
  // Register the interface
  if(!(nRet = usb_interface_claimed(&dev->actconfig[0].interface[ifnum])))
  {
    dbg("probe_touchscreen(%d): interface_claimed=%d", minor, nRet);
    usb_driver_claim_interface(&touchscreen_driver,&dev->actconfig[0].interface[ifnum],tscrn);
  }
  else
    dbg("probe_touchscreen(%d): error interface_claimed=%d", minor, nRet);
  dbg("probe_touchscreen(%d): Configuring CTRL handler for intr EP:%d", minor, have_intr);
  /* sends reset command */
  // receive answer data
  tscrn->setup_packet->requesttype = USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_DEVICE;
  tscrn->setup_packet->request = USB_REQ_GET_CONFIGURATION;
  tscrn->setup_packet->value = 0x0;
  tscrn->setup_packet->index = 0x0;//ifnum;
  tscrn->setup_packet->length = 1;
  FILL_CONTROL_URB(&tscrn->ctrlin, dev, 
      usb_rcvctrlpipe(dev, 0x80), (unsigned char *)tscrn->setup_packet, 
      tscrn->ibuf, sizeof(tscrn->ibuf), ctrl_touchscreen, tscrn);

  if ((nRet=usb_submit_urb(&tscrn->ctrlin)))
  {
    err("probe_touchscreen(%d): GET_CONFIGURATION:Get smth error=%d status=%d.", minor, nRet, tscrn->ctrlin.status);
    kfree(tscrn->ibuf);
    kfree(tscrn->obuf);
    kfree(tscrn->setup_packet);
    kfree(tscrn);
    return NULL;
  }
  nCounter = 10;
  dbg("probe_touchscreen(%d): GET_CONFIGURATION:wait for data: status=%d nRet=%d", minor,tscrn->ctrlin.status, nRet);
  interruptible_sleep_on(&tscrn->wait);
  dbg("probe_touchscreen(%d): GET_CONFIGURATION:gets 0x%x,0x%x status=%d nRet=%d", minor,(int)tscrn->ibuf[0],(int)tscrn->ibuf[1], tscrn->ctrlin.status, nRet);
  if(tscrn->ctrlin.status == (-32))
  {
    dbg("probe_touchscreen(%d): get error -32 status=%d - reset pipe", minor,tscrn->ctrlin.status);
    nRet = usb_clear_halt(dev,0x00);
    dbg("probe_touchscreen(%d): get error -32 status=%d - reset pipe nRet=%d", minor,tscrn->ctrlin.status, nRet);
  }
  //
  // receive answer data
  tscrn->setup_packet->requesttype = USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_ENDPOINT;
  tscrn->setup_packet->request = USB_REQ_GET_STATUS;
  tscrn->setup_packet->value = 0x0;
  tscrn->setup_packet->index = 0x81;
  tscrn->setup_packet->length = 2;
  FILL_CONTROL_URB(&tscrn->ctrlin, dev, 
      usb_rcvctrlpipe(dev, 0x80), (unsigned char *)tscrn->setup_packet, 
      tscrn->ibuf, sizeof(tscrn->ibuf), ctrl_touchscreen, tscrn);

  if ((nRet=usb_submit_urb(&tscrn->ctrlin)))
  {
    err("probe_touchscreen(%d): GET_STATUS:Get smth error=%d status=%d .", minor, nRet, tscrn->ctrlin.status);
    kfree(tscrn->ibuf);
    kfree(tscrn->obuf);
    kfree(tscrn->setup_packet);
    kfree(tscrn);
    return NULL;
  }
  dbg("probe_touchscreen(%d): GET_STATUS: status=%d nRet=%d", minor,tscrn->ctrlin.status, nRet);
  interruptible_sleep_on(&tscrn->wait);
  dbg("probe_touchscreen(%d): GET_STATUS:gets 0x%x,0x%x,0x%x status=%d nRet=%d", minor,(int)tscrn->ibuf[0],(int)tscrn->ibuf[1],(int)tscrn->ibuf[2], tscrn->ctrlin.status, nRet);

  // --  // get controller id report
  tscrn->setup_packet->requesttype = USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE;
  tscrn->setup_packet->request = TSCRN_USB_REQUEST_CONTROLLER_ID;
  tscrn->setup_packet->value = 0;
  tscrn->setup_packet->index = 0;
  tscrn->setup_packet->length = TSCRN_USB_RAPORT_SIZE_ID;

  memset((char *)tscrn->ibuf,0,IBUF_SIZE);

  FILL_CONTROL_URB(&tscrn->ctrlout, dev, 
      usb_rcvctrlpipe(dev, 0x80), (unsigned char *)tscrn->setup_packet, 
      tscrn->ibuf, TSCRN_USB_RAPORT_SIZE_ID, ctrl_touchscreen, tscrn);

  if ((nRet=usb_submit_urb(&tscrn->ctrlout)))
  {
    err("probe_touchscreen(%d): GET_ID:error=%d status=%d .", minor, nRet, tscrn->ctrlout.status);
    kfree(tscrn->ibuf);
    kfree(tscrn->obuf);
    kfree(tscrn->setup_packet);
    kfree(tscrn);
    return NULL;
  }
  dbg("probe_touchscreen(%d): GET_ID gets 0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x status=%d nRet=%d", minor,(int)tscrn->ibuf[0],(int)tscrn->ibuf[1],(int)tscrn->ibuf[2],(int)tscrn->ibuf[3],(int)tscrn->ibuf[4],(int)tscrn->ibuf[5],(int)tscrn->ibuf[6],(int)tscrn->ibuf[7], tscrn->ctrlout.status, nRet);
  interruptible_sleep_on(&tscrn->wait);
  dbg("probe_touchscreen(%d): GET_ID gets 0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x status=%d nRet=%d", minor,(int)tscrn->ibuf[0],(int)tscrn->ibuf[1],(int)tscrn->ibuf[2],(int)tscrn->ibuf[3],(int)tscrn->ibuf[4],(int)tscrn->ibuf[5],(int)tscrn->ibuf[6],(int)tscrn->ibuf[7], tscrn->ctrlout.status, nRet);
#endif
  ////////////////////////////////////////////////////////////////////////////////////
  /* Ok, if we detected an interrupt EP, setup a handler for it */
  if (have_intr)
  {
    dbg("probe_touchscreen(%d): Configuring IRQ handler for intr EP:%d", minor, have_intr);
    FILL_INT_URB(&tscrn->irq, dev, 
        usb_rcvintpipe(dev, 0x81),//endpoint[(int )(have_intr-1)].bEndpointAddress),
    tscrn->data, TSCRN_USB_RAPORT_SIZE_DATA, irq_touchscreen, tscrn,
    endpoint[(int)(have_intr-1)].bInterval);
    //2);

    if (usb_submit_urb(&tscrn->irq))
    {
      err("probe_touchscreen(%d): Unable to allocate INT URB.", minor);
      kfree(tscrn->ibuf);
      kfree(tscrn->obuf);
      kfree(tscrn->setup_packet);
      kfree(tscrn);
      return NULL;
    }
    //the status will be -115 -> thist means EINPROGRESS, but it should be that.
    dbg("probe_touchscreen(%d): IRQ done interval=%d status=%d", minor,endpoint[(int)(have_intr-1)].bInterval, tscrn->irq.status);
  }

  tscrn->intr_ep = have_intr;
  tscrn->present = 1;
  tscrn->dev = dev;
  tscrn->minor = minor;
  tscrn->isopen = 0;

  input_register_device(&tscrn->input_dev);

  return p_tscrn_table[minor] = tscrn;
}

static void disconnect_touchscreen(struct usb_device *dev, void *ptr)
{
  struct tscrn_usb_data *tscrn = (struct tscrn_usb_data *) ptr;

  //if(tscrn->intr_ep)
  //{
  if(tscrn->irq.status != 0)
  {
    dbg("disconnect_touchscreen(%d): Unlinking IRQ URB", tscrn->minor);
    usb_unlink_urb(&tscrn->irq);
  }
  //}
  if(tscrn->ctrlin.status != 0)
  {
    dbg("disconnect_touchscreen(%d): Unlinking CTRLIN URB", tscrn->minor);
    usb_unlink_urb(&tscrn->ctrlin);
  }
  if(tscrn->ctrlout.status != 0)
  {
    dbg("disconnect_touchscreen(%d): Unlinking CTRLOUT URB", tscrn->minor);
    usb_unlink_urb(&tscrn->ctrlout);
  }

  input_unregister_device(&tscrn->input_dev);
  usb_driver_release_interface(&touchscreen_driver,
      &tscrn->dev->actconfig->interface[tscrn->ifnum]);

  kfree(tscrn->ibuf);
  kfree(tscrn->obuf);
  kfree(tscrn->setup_packet);

  dbg("disconnect_touchscreen: De-allocating minor:%d", tscrn->minor);
  p_tscrn_table[tscrn->minor] = NULL;
  kfree (tscrn);
}

static struct file_operations usb_touchscreen_fops = 
{
read:		read_touchscreen,
                //write:		write_touchscreen,
poll:         poll_touchscreen,                
ioctl:		ioctl_touchscreen,
open:		open_touchscreen,
release:	close_touchscreen,
};

static struct usb_driver touchscreen_driver = 
{
  "touchscreen",
  probe_touchscreen,
  disconnect_touchscreen,
  { NULL, NULL },
  &usb_touchscreen_fops,
  TSCRN_BASE_MNR
};

void __exit usb_touchscreen_exit(void)
{
  usb_deregister(&touchscreen_driver);
}

int __init usb_touchscreen_init (void)
{
  if (usb_register(&touchscreen_driver) < 0)
    return -1;

  info("USB Touchscreen support registered.");
  return 0;
}

module_init(usb_touchscreen_init);
module_exit(usb_touchscreen_exit);
