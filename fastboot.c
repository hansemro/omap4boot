/*
 * Copyright (c) 2010, The Android Open Source Project.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Neither the name of The Android Open Source Project nor the names
 *    of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <aboot/aboot.h>
#include <aboot/types.h>
#include <aboot/io.h>
#include <omap4/hw.h>
#include <common/omap_rom.h>
#include <libc/string.h>
#include "config.h"

#ifdef DEBUG
#define DBG(x...) printf(x)
#else
#define DBG(x...)
#endif /* DEBUG */

#if defined CONFIG_FASTBOOT
#include <common/fastboot.h>

struct usb usb;

char *get_serial_number(void)
{
	static char serialno[20];
	u32 val[4] = { 0 };
	u32 reg;

	reg = CONTROL_STD_FUSE_DIE_ID_0;
	val[0] = readl(reg);
	val[1] = readl(reg + 0x8);
	val[2] = readl(reg + 0xC);
	val[3] = readl(reg + 0x10);
	DBG("Device Serial Number: %08X%08X\n", val[3], val[2]);
	sprintf(serialno, "%08X%08X", val[3], val[2]);

	return serialno;
}

static char *get_proc_type(void)
{
	static char proc_type[8];
	int proc = get_omap_type();

	switch (proc) {
	case OMAP_TYPE_EMU:
		strcpy(proc_type, "EMU");
		break;
	case OMAP_TYPE_SEC:
		strcpy(proc_type, "HS");
		break;
	case OMAP_TYPE_GP:
		strcpy(proc_type, "GP");
		break;
	default:
		strcpy(proc_type, "unknown");
		break;
	}

	return proc_type;
}

static char *get_cpurevision(void)
{
	static char cpu_rev[8];
	int cpu = get_omap_rev();

	switch (cpu) {
	case OMAP_4430_ES1_DOT_0:
		strcpy(cpu_rev, "ES1.0");
		break;
	case OMAP_4430_ES2_DOT_0:
		strcpy(cpu_rev, "ES2.0");
		break;
	case OMAP_4430_ES2_DOT_1:
		strcpy(cpu_rev, "ES2.1");
		break;
	case OMAP_4430_ES2_DOT_2:
		strcpy(cpu_rev, "ES2.2");
		break;
	case OMAP_4430_ES2_DOT_3:
		strcpy(cpu_rev, "ES2.3");
		break;
	case OMAP_4460_ES1_DOT_0:
		strcpy(cpu_rev, "ES1.0");
		break;
	case OMAP_4460_ES1_DOT_1:
		strcpy(cpu_rev, "ES1.1");
		break;
	default:
		printf("OMAP_REV_INVALID\n");
		strcpy(cpu_rev, "invalid");
		break;
	}

	return cpu_rev;
}

static char *get_procversion(void)
{
	static char proc_ver[8];
	int cpu = get_omap_rev();

	switch (cpu) {
	case OMAP_4430_ES1_DOT_0:
		strcpy(proc_ver, "4430");
		break;
	case OMAP_4430_ES2_DOT_0:
		strcpy(proc_ver, "4430");
		break;
	case OMAP_4430_ES2_DOT_1:
		strcpy(proc_ver, "4430");
		break;
	case OMAP_4430_ES2_DOT_2:
		strcpy(proc_ver, "4430");
		break;
	case OMAP_4430_ES2_DOT_3:
		strcpy(proc_ver, "4430");
		break;
	case OMAP_4460_ES1_DOT_0:
		strcpy(proc_ver, "4460");
		break;
	case OMAP_4460_ES1_DOT_1:
		strcpy(proc_ver, "4460");
		break;
	default:
		printf("OMAP_REV_INVALID\n");
		strcpy(proc_ver, "invalid");
		break;
	}

	return proc_ver;
}

static int fastboot_tx_status(const char *buffer, unsigned int buffer_size)
{
	/* send response back to host */
	static char response[65];
	strcpy(response, buffer);
	usb_write(&usb, response, strlen(response));
	return 0;
}

static int fastboot_getvar(const char *rx_buffer, char *tx_buffer)
{
	char serial[20];
	char proctype[8];
	char cpurev[8];
	char procver[8];
	char response[65];

	strcpy(response, "OKAY");

	DBG("fastboot_getvar()\n");

	if (!memcmp(rx_buffer, "version", 7))
		strcpy(response + 4, FASTBOOT_VERSION);
	else if (!memcmp(rx_buffer, "product", 7))
		strcpy(response + 4, PRODUCT_NAME);
	else if (!memcmp(rx_buffer, "serialno", 8)) {
		strcpy(serial, get_serial_number());
		strcpy(response + 4, serial);
	} else if (!memcmp(rx_buffer, "cpurev", 6)) {
		strcpy(cpurev, get_cpurevision());
		strcpy(response + 4, cpurev);
	} else if (!memcmp(rx_buffer, "secure", 6)) {
		strcpy(proctype, get_proc_type());
		strcpy(response + 4, proctype);
	} else if (!memcmp(rx_buffer, "cpu", 3)) {
		strcpy(procver, get_procversion());
		strcpy(response + 4, procver);
	} else
		printf("fastboot_getvar():unsupported variable\n");

	fastboot_tx_status(response, strlen(response));

	return 0;
}

void do_fastboot(void)
{
	int ret = 0;
	char cmd[65];
	int cmdsize = 0;

	/* Use 65 instead of 64, null gets dropped
	strcpy's need the extra byte */
	char response[65];

	/* enable irqs */
	enable_irqs();

	while (1) {

		cmdsize = 0;
		memset(&cmd, 0, 64);
		memset(&response, 0, 64);

		/* receive the fastboot command size from host*/
		ret = usb_read(&usb, &cmdsize, 4);
		if (ret < 0) {
			printf("failed to read the fastboot command size\n");
			strcpy(response, "FAIL");
			goto fail;
		}

		/* receive the fastboot command from host */
		ret = usb_read(&usb, &cmd, cmdsize);
		if (ret < 0) {
			printf("failed to read the fastboot command\n");
			strcpy(response, "FAIL");
			goto fail;
		}

		if (memcmp(cmd, "getvar:", 7) == 0) {

			strcpy(response, "OKAY");

			fastboot_getvar(cmd + 7, response);

		} /* getvar if loop ends */

	} /* while(1) loop ends */

fail:
	/* send response back to host */
	fastboot_tx_status(response, strlen(response));
	printf("\nsomething bad happened\n");
	while (1)	/* stay here */
		;
} /* do_fastboot() ends */


#endif
