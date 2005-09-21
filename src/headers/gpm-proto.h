
#ifndef __GPMPROTO_H__
#define __GPMPROTO_H__

/*
 * This is a draft for a protocol to use for a "generic input device"
 * that can feed packets to gpm. In general, this "generic input
 * device" will be a program that collects data from whatever source
 * fits its scope and pushes packet to gpm for handling. The same
 * protocol may be used to get events into X. My main motivation for
 * this is exploiting my screenless touchscreen to do handwriting
 * recognition and other silly stuff.
 *
 * While it may look complex, I think it's in fact easy; also, it
 * fits the fixed-size contraint that simplifies data transfer, features
 * aligned data items and does not pose any limit on the number of
 * axis/buttons/keys delivered with each packet. It can be
 * implemented slowly and each functionality can be verified in real
 * usage. It should be extensible enough to be pretty long-lived.
 *
 * Please note that I don't know of any similar effort, so I may just
 * be reinventing the wheel or doing some other stupid errors.
 *
 * Please note that timestamping is still missing, although it should
 * fit the design (just forgot about it, and the joystick api reminded
 * me about that).
 */

/*
 * This is the fixed-size packet, multi-byte fields use the network
 * byte order, so ntohl() and ntohs() are needed, but portability is
 * ensured. Note that it is 16 bytes, 4 of them are "control" info and
 * the rest is three 4-bytes data items.
 */

struct gpm_procotol_packet {
    __u8   flags;             /* 10xxxxFF  flags defined later */
    __u8   buttons;           /* 01BBBBBB  button-1 to button-6 */
    __u16  key;               /* how to read the following data */
    union {
        __u32  l;       /* "long" */
        __s32  sl;      /* "signed long" */
        __u16  w[2];    /* "word" */
        __s16  sw[2];   /* "signed word" */
        __u8   b[4];    /* "byte"  */
        __s8   sb[4];   /* "signed byte" */
    } data[3];
};

/*
 * these flags can be used to build up longer packets (thus placing no limit
 * on the number of dimensions, buttons, keypresses per packet). A complete
 * packet has both flags set, otherwise other packets must be retrieved to
 * have all the information available.  Most "simple" input devices will
 * just need one chunk of data (it offers three dimensions and 6 buttons)
 */
#define GPM_PROTOCOL_FLAG_FIRST      0x01
#define GPM_PROTOCOL_FLAG_LAST       0x02

/*
 * The "key" field is made up of three bitfields, each defines what the
 * corresponfing "data" item means. There are 15 meanings for each datum,
 * and the top four bits are currently unused and must be cleared.
 */
enum gpm_protocol_item {
    GPM_ITEM_UNUSED = 0,      /* the data field is unused */
    GPM_ITEM_RAXIS,           /* relative X, Y, Z or further (signed long) */
    GPM_ITEM_AAXIS,           /* absolute axis (unsigned long, full-range) */
    GPM_ITEM_BUTTONS,         /* 32 buttons (7-38, 39-70, ...) as u32 mask */
    GPM_ITEM_8BITKEYS,        /* ascii or so, as u8. 0xff == unused */
    GPM_ITEM_KEYCODES,        /* keycodes as s16: pos/neg, 0x8000 == unused */
    GPM_ITEM_KEYSYM,          /* X keysym. positive/negative (?) as s32 */
    GPM_ITEM_RESERVED,
    GPM_ITEM_RESERVED,
    GPM_ITEM_RESERVED,
    GPM_ITEM_RESERVED,
    GPM_ITEM_RESERVED,
    GPM_ITEM_RESERVED,
    GPM_ITEM_RESERVED,
    GPM_ITEM_RESERVED,
    GPM_ITEM_RESERVED
};

/*
    RAXIS: relative position. First axis is X, then Y, then Z and so
	      on.  Use of third-and-further axis is application
	      specific.  As for scaling, I think every motion must be
	      rescaled, and a mouse tick may be 1/1024 of the whole
	      screen or so, but I'm not sure about that. If it is not
	      rescaled to match absolute movements, then you can't mix
	      relative and absolute movements, something that I want
	      to be able to do.

    AAXIS: absolute position on an axis, scaled to the whole 32bit
              range.

    BUTTONS: if more than 6 buttons are there, a data field can host
	      32 extra buttons. If 38 are not enough other data fields
	      can follow.  If the complete packet is made up by more
	      than one protocol item, the "buttons" field of trailing
	      items is unused. Use of the buttons is application
	      specific.

    8BITKEYS: One to four ascii-or-so keys to push in the keyboard buffer.
              This is needed by the twiddler, for example. I have to study
	      how the X protocol works before I can use this in X.

    KEYCODES: Raw keyboard keycodes. I feel these must be discouraged, as
              users remap their keyboards (although smart use of dumpkeys
	      can unmap the keycodes properly). Positive for press and
	      negative for release; up to two keycodes can fit in one
	      data item

    KEYSYM: X keysym. (range 0 to 0xFFFFFF). Once again, I have to study X,
              but I think there is no need for press/release events. Each
	      data item can host a single keysym as they span 24 bits

*/
    
    
#endif /* __GPM_PROTO_H__ */
