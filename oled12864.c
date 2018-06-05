#include "oled12864_define.h"
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/delay.h>

#define DEVICE_NAME "OLED12864"

static int oled_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int oled_remove(struct i2c_client *client);

static ssize_t oled_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos);
static long oled_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

static void oled_reg_init(void);
static void oled_fill(char data);
static void oled_set_addr(char x, char y);
static void oled_set_contrast(char contrast);
static void oled_on(void);
static void oled_off(void);

static struct file_operations oled_fops = {
	.owner = THIS_MODULE,
	.write = oled_write,
	.unlocked_ioctl = oled_ioctl,
};

static struct miscdevice oled_miscdevice = {
	.name	= DEVICE_NAME,
	.fops	= &oled_fops,
	.minor	= MISC_DYNAMIC_MINOR,
};

static struct i2c_client *g_client;
static int oled_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int err = 0;
	
	printk("%s device is detected\n", DEVICE_NAME);
	g_client = client;
	err = misc_register(&oled_miscdevice);
	if(err)
	{
		printk("%s: register miscdevice failed!\n", __FUNCTION__);
	}
	
	oled_reg_init();
	return 0;
}

static int oled_remove(struct i2c_client *client)
{
	printk("%s\n", __FUNCTION__);
	misc_deregister(&oled_miscdevice);
	return 0;
}

static ssize_t oled_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	char *send_buf = NULL;
	int copy_err_count = 0;
	printk("%s\n", __FUNCTION__);
	send_buf = (char *)kmalloc(count + 1, GFP_KERNEL);
	if(send_buf == NULL)
	{
		printk("oled write failed!\n");
		return -1;
	}
	
	copy_err_count = copy_from_user(&send_buf[1], buf, count);
	send_buf[0] = 0x40;
	i2c_master_send(g_client, send_buf, count - copy_err_count + 1);/* addr would auto increase horizonally*/
	
	kfree(send_buf);
	return (count - copy_err_count);
}

static long oled_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	char data_tmp[2];
	int copy_result = 0;
	printk("%s\n", __FUNCTION__);
	
	switch(cmd)
	{
		case IOCTL_CMD_FILL_SCREEN:
			printk("IOCTL_CMD_FILL_SCREEN\n");
			copy_result = copy_from_user(data_tmp, (char *)arg, 1);
			oled_fill(data_tmp[0]);
			break;
		
		case IOCTL_CMD_SET_ADDR:
			printk("IOCTL_CMD_SET_ADDR\n");
			copy_result = copy_from_user(data_tmp, (char *)arg, 2);
			oled_set_addr(data_tmp[0], data_tmp[1]);
			break;
			
		case IOCTL_CMD_SET_CONTRAST:
			printk("IOCTL_CMD_SET_CONTRAST\n");
			copy_result = copy_from_user(data_tmp, (char *)arg, 1);
			oled_set_contrast(data_tmp[0]);
			break;
			
		case IOCTL_CMD_DISPLAY_ON:
			printk("IOCTL_CMD_DISPLAY_ON\n");
			oled_on();
			break;
			
		case IOCTL_CMD_DISPLAY_OFF:
			printk("IOCTL_CMD_DISPLAY_OFF\n");
			oled_off();
			break;
		default:
			return -1;
	}
	return 0;
}

static void oled_reg_init(void)
{
	char init_cmd[] = {0x00, 0xae, 0x20, 0x10, 0xb0, 0xc8, 0x00, 0x10, 0x40, 0x81, 0xff,
					  0xa1, 0xa6, 0xa8, 0x3F, 0xa4, 0xd3, 0x00, 0xd5, 0xf0, 0xd9,
					  0x22, 0xda, 0x12, 0xdb, 0x20, 0x8d, 0x14, 0xaf};
	msleep(100);
	i2c_master_send(g_client, init_cmd, sizeof(init_cmd));
}

static void oled_fill(char data)
{
	char send_buf[129];
	char page;
	send_buf[0] = 0x40;
	memset(send_buf + 1, data, 128);
	for(page = 0; page < 8; page++)
	{
		oled_set_addr(0, page);
		i2c_master_send(g_client, send_buf, 129);
	}
}

static void oled_set_addr(char x, char y)
{
	char send_buf[2];
	send_buf[0] = 0x00;
	
	send_buf[1] = 0xb0 | (y & 0x07);/* set y addr */
	i2c_master_send(g_client, send_buf, 2);
	
	send_buf[1] = 0x10 | ((x / 16) & 0x0f);/* set x addr */
	i2c_master_send(g_client, send_buf, 2);
	
	send_buf[1] = x % 16;/* set x offset */
	i2c_master_send(g_client, send_buf, 2);
}

static void oled_set_contrast(char contrast)
{
	char send_buf[3];
	send_buf[0] = 0x00;
	send_buf[1] = 0x81;
	send_buf[2] = contrast;
	i2c_master_send(g_client, send_buf, 3);
}

static void oled_on(void)
{
	char send_buf[2];
	send_buf[0] = 0x00;
	send_buf[1] = 0xaf;/* display on in normal mode */
	i2c_master_send(g_client, send_buf, 2);
}

static void oled_off(void)
{
	char send_buf[2];
	send_buf[0] = 0x00;
	send_buf[1] = 0xae;/* display off(sleep mode) */
	i2c_master_send(g_client, send_buf, 2);
}

static const struct i2c_device_id oled_id[] = {
	{ DEVICE_NAME, 0 },
	{ }
};
//MODULE_DEVICE_TABLE(i2c, oled_id);

static struct i2c_driver oled_driver = {
	.driver		= {
		.name	= DEVICE_NAME,
		.owner	= THIS_MODULE,
	},
	.probe		= oled_probe,
	.remove		= oled_remove,
	.id_table	= oled_id,
};

static int __init oled_init(void)
{
	printk("%s\n", __FUNCTION__);
	return i2c_add_driver(&oled_driver);
}

static void __exit oled_exit(void)
{
	printk("%s\n", __FUNCTION__);
	i2c_del_driver(&oled_driver);
}

module_init(oled_init);
module_exit(oled_exit);
MODULE_AUTHOR("hokamyuen");
MODULE_DESCRIPTION("oled12864 drv");
MODULE_LICENSE("GPL");
MODULE_VERSION("V0.1");
