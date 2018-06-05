#ifndef __OLED12864_H_
#define __OLED12864_H_

/*
* x addr is range from 0 to 127
* y addr is range from 0 to 7
*/

#define IOCTL_CMD_FILL_SCREEN		0x11
#define IOCTL_CMD_SET_ADDR			0x12
#define IOCTL_CMD_SET_CONTRAST		0x13
#define IOCTL_CMD_DISPLAY_ON		0x14
#define IOCTL_CMD_DISPLAY_OFF		0x15

#endif