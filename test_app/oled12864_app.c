#include "../oled12864_define.h"
#include "text_lib.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define DEV_PATH "/dev/OLED12864"

typedef struct cpu_state
{
	char name[10];
	unsigned int user;
	unsigned int nice;
	unsigned int system;
	unsigned int idle;
	unsigned int iowait;
	unsigned int irq;
	unsigned int softirq;
}cpu_state;

typedef struct mem_state
{
	char name[10];
	unsigned int total;
	unsigned int used;
	unsigned int free;
	unsigned int shared;
	unsigned int buffers;
	unsigned int cached;
}mem_state;

void write_en_8x16(int fd, char x, char y, char c);

int main(int argc, char **argv)
{
	int fd, i, ret, uptime;
	struct cpu_state cpu_state1, cpu_state2;
	struct mem_state mem_state1;
	int cpu_load, cpu_time, mem_used;
	FILE *f;
	char fill_data = 0x00;
	char read_buf[128];
	char uptime_str[17];
	char cpu_load_str[17];
	char mem_used_str[17];
	
	printf("oled12864 test\n");
	fd = open(DEV_PATH, O_RDWR);
	ioctl(fd, IOCTL_CMD_FILL_SCREEN, &fill_data);
	while(1)
	{
		memset(read_buf, 0, 128);
		f = popen("cat /proc/uptime | awk '{print $1}'", "r");
		fread(read_buf, 128, 1, f);
		pclose(f);
		uptime = atoi(read_buf);
		ret = snprintf(uptime_str, 17, "up: %dh%3dm%3ds", uptime / 3600, uptime % 3600 / 60, uptime % 60);
		for(i = 0; i < ret; ++i)
		{
			write_en_8x16(fd, i, 0, uptime_str[i]);
		}
		
		cpu_state1 = cpu_state2;
		memset(read_buf, 0, 128);
		f = popen("cat /proc/stat | sed -n '1p'", "r");
		fread(read_buf, 128, 1, f);
		pclose(f);
		sscanf(read_buf, "%s %u %u %u %u %u %u %u", cpu_state2.name, &cpu_state2.user,
			   &cpu_state2.nice, &cpu_state2.system, &cpu_state2.idle, &cpu_state2.iowait,
			   &cpu_state2.irq, &cpu_state2.softirq);
		cpu_time = cpu_state2.user - cpu_state1.user + cpu_state2.nice - cpu_state1.nice
					+ cpu_state2.system - cpu_state1.system + cpu_state2.idle - cpu_state1.idle
					+ cpu_state2.iowait - cpu_state1.iowait + cpu_state2.irq - cpu_state1.irq
					+ cpu_state2.softirq - cpu_state1.softirq;
		if(cpu_time != 0)
			cpu_load = 100 - 100 * (cpu_state2.idle - cpu_state1.idle) / cpu_time;
		ret = snprintf(cpu_load_str, 17, "cpu:%4u%% used", cpu_load);
		for(i = 0; i < ret; ++i)
		{
			write_en_8x16(fd, i, 1, cpu_load_str[i]);
		}
		
		memset(read_buf, 0, 128);
		f = popen("free | sed -n '2p'", "r");
		fread(read_buf, 128, 1, f);
		pclose(f);
		sscanf(read_buf, "%s %u %u %u %u %u %u", mem_state1.name, &mem_state1.total,
				&mem_state1.used, &mem_state1.free, &mem_state1.shared,
			  &mem_state1.buffers, &mem_state1.cached);
		if(mem_state1.total != 0)
		mem_used = 100 * (mem_state1.used + mem_state1.shared) / mem_state1.total;
		ret = snprintf(mem_used_str, 17, "mem:%4u%% used", mem_used);
		for(i = 0; i < ret; ++i)
		{
			write_en_8x16(fd, i, 2, mem_used_str[i]);
		}
		
		sleep(1);
	}
	close(fd);
	return 0;
}

void write_en_8x16(int fd, char x, char y, char c)
{
	char addr[2];
	if(c < 32)
		return ;
//	printf("write %c\n", c);
	addr[0] = x * 8;
	addr[1] = 2 * y;
	ioctl(fd, IOCTL_CMD_SET_ADDR, addr);
	write(fd, &EN_8X16[c - 32].lib_data[0], 8);
	++ addr[1];
	ioctl(fd, IOCTL_CMD_SET_ADDR, addr);
	write(fd, &EN_8X16[c - 32].lib_data[8], 8);
}

