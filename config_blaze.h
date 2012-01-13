
#define CONFIG_BOARD_MACH_TYPE		3429
#define CONFIG_IS_OMAP4

#define CONFIG_BAUDRATE			115200

#define CONFIG_SERIAL_BASE		OMAP44XX_UART3
#define CONFIG_SERIAL_CLK_HZ		48000000

#define CONFIG_RAM_HANDLERS		0x4030D020
#define CONFIG_RAM_VECTORS		0x4030D000
#define CONFIG_STACK_TOP		0x4030D000

#define CONFIG_ADDR_DOWNLOAD		0x82000000

#define CONFIG_ADDR_ATAGS		0x80000100
#define CONFIG_ADDR_KERNEL		0x80008000
#define CONFIG_ADDR_RAMDISK		0x81000000
