/*
 * (C) Copyright 2016
 * Richard, GuangDong Linux Center, <edaplayer@163.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <command.h>
#include <linux/ctype.h>

#ifndef CONFIG_START_SYS
#define CONFIG_START_SYS "run boot_args;nand read.i ${dtbloadaddr} dtb;" \
								"nand read.i ${kloadaddr} kernel;bootz ${kloadaddr} - ${dtbloadaddr}"
#endif

#ifndef SET_BOOTARGS
#define SET_BOOTARGS	"run set_args"
#endif

#define SET_NFS_BOOTARGS "setenv bootargs console=${console} root=/dev/nfs nfsroot=${nfsserverip}:${nfs_dir} rootwait=1" \
					"init=/init ip=${ipaddr}:${nfsserverip}:${gatewayip}:${netmask}::eth0:off"

#ifndef PRINTF_BOOTARGS
#define PRINTF_BOOTARGS	"run echo_boot_args"
#endif

enum menu_index{
	MAIN_MENU,
	TFTP_MENU
};

typedef struct menu_list{
	const char shortcut;
	const char *help;
	const char *cmd;
	void (*func)(void);
	const struct menu_item *next_menu_item;
//	int flag;
}menu_list_t;

typedef struct menu_item{
	const char *title;
	struct menu_list *menu;
	const int menu_arraysize; 
}menu_item_t;

typedef struct params{
	const char *envname;
	const char *promt;
}params_t;

static const params_t const tftp_params[];
static void set_params(const params_t *params, int array_size);
static void set_tftp_params(void);
static menu_list_t tftp_menu_list[];
static menu_list_t main_menu_list[];
static menu_list_t params_menu_list[];
static menu_list_t lcdtype_menu_list[];
static const menu_item_t main_menu_item;
static const menu_item_t tftp_menu_item;
static const menu_item_t params_menu_item;
static const menu_item_t lcdtype_menu_item;
static const menu_item_t uart_mux_menu_item;


extern char console_buffer[];
extern int menu_readline(const char *const prompt);

static const params_t tftp_params[] = {
	{"serverip",		"Enter the TFTP Server(PC) IP address:(xxx.xxx.xxx.xxx)"},
	{"ipaddr", 			"Enter the IP address:(xxx.xxx.xxx.xxx)"},
	{"netmask", 		"Enter the netmask address:(eg:255.255.255.0)"},
	{"mloimgname", 		"Enter the SPL image nanme"},
	{"ubootimgname", 	"Enter the u-boot image nanme"},
	{"logoimgname",		"Enter the logo image nanme"},
	{"kernelimgname", 	"Enter the kernel image nanme"},
	{"rootimgname", 	"Enter the rootfs image nanme"},
	{"dtbname", 		"Enter the dtb image nanme"},
};

static const params_t nfs_params[] = {
	{"nfsserverip",	"Enter the NFS Server(PC) IP address:(xxx.xxx.xxx.xxx)"},
	{"ipaddr", 		"Enter the IP address:(xxx.xxx.xxx.xxx)"},
	{"gatewayip", 	"Enter the NFS Gateway IP address:(eg:192.168.1.1)"},
	{"netmask", 	"Enter the Net Mask:(eg:255.255.255.1)"},
	{"nfs_dir", 	"Enter NFS directory:(eg: /opt/rootfs"},
};

static const params_t boot_params[] = {
	{"console",			"Enter (console=) for bootargs"},
	{"optargs", 		"Enter (mem=) for bootargs"},
	{"device_root", 	"Enter (root=) for bootargs"},
	{"root_fs_type", 	"Enter (rootfstype=) for bootargs"},
};

static void set_params(const params_t *params,int array_size)
{
	int i;
	char cmd_buf[200];
	char *env_pt = NULL;
	for(i = 0; i < array_size; i++) {
		printf("%s\n", params[i].promt);
		env_pt = getenv(params[i].envname);	
		menu_readline(env_pt);
		sprintf(cmd_buf, "setenv %s %s",params[i].envname,console_buffer);
		run_command(cmd_buf, 0);
	}
	printf("Save parameters?(y/n)\n");
	if (getc() == 'y' )
	{
		printf("y");
		if (getc() == '\r') {
			printf("\n");
			run_command("saveenv", 0);
		}
		else					
			puts("save aborted\n");
	}
	else
	{
		printf("Not Save it!!!\n");
	}
}

static void set_tftp_params(void)
{
	set_params(tftp_params, ARRAY_SIZE(tftp_params));
}

static void set_nfs_params(void)
{
	set_params(nfs_params, ARRAY_SIZE(nfs_params));
	run_command(SET_NFS_BOOTARGS, 0);
	run_command(PRINTF_BOOTARGS, 0);
}

static void set_boot_params(void)
{
	set_params(boot_params, ARRAY_SIZE(boot_params));
	run_command(SET_BOOTARGS, 0);
	run_command(PRINTF_BOOTARGS, 0);
}

static void set_a_env_params(void)
{
	char cmd_buf[100];
	printf("set environment variable \'name\' to \'value ...\'\n");
	menu_readline("setenv ");
	sprintf(cmd_buf, "%s",console_buffer);
	run_command(cmd_buf, 0);
}

static void print_a_env_params(void)
{
	char cmd_buf[100];
	printf("print value of environment variable \'name\'\n");
	menu_readline("printenv ");
	sprintf(cmd_buf, "%s",console_buffer);
	run_command(cmd_buf, 0);
}

/* common menu title and cmd */
#define DOWN_MLO_TITLE		"Download MLO to Nand Flash"
#define DOWN_UBOOT_TITLE	"Download u-boot.bin to Nand Flash"
#define DOWN_KERNEL_TITLE	"Download Linux Kernel (zImage.bin) to Nand Flash"
#define DOWN_LOGO_TITLE		"Download Logo image(logo.bin) to Nand Flash"
#define DOWN_UBIFS_TITLE	"Download UBIFS image (root.bin) to Nand Flash"
#define DOWN_YAFFS_TITLE	"Download YAFFS image (root.bin) to Nand Flash"
#define DOWN_PRAMGAM_TITLE	"Download Program to SDRAM and Run it"
#define BOOT_SYSTEM_TITLE	"Boot the system"
#define NAND_SCRUB_TITLE	"Format the Nand Flash"

/* tftp menu title and cmd */
#define DOWN_DTB_TITLE		"Download Device tree blob(*.dtb)"
#define TEST_NET_TITLE		"Test network (Ping PC's IP)"
#define SET_TFTP_TITLE		"Set TFTP parameters(serverip,ipaddr,imagename...)"
#define TEST_KERNEL_TITLE	"Test kernel Image (zImage)"

/* tftp menu cmd */
#define CMD_DOWN_MLO_TFTP		"tftp ${cpaddr} ${mloimgname}; nand erase.part SPL; nand write.i ${cpaddr} SPL ${filesize}"
#define CMD_DOWN_UBOOT_TFTP 	"tftp ${cpaddr} ${ubootimgname}; nand erase.part u-boot; nand write.i ${cpaddr} u-boot ${filesize}"
#define CMD_DOWN_KERNEL_TFTP	"tftp ${cpaddr} ${kernelimgname}; nand erase.part kernel; nand write.i ${cpaddr} kernel ${filesize}"
#define CMD_DOWN_LOGO_TFTP		"tftp ${cpaddr} ${logoimgname}; nand erase.part logo; nand write.i ${cpaddr} logo ${filesize}"
#define CMD_DOWN_UBIFS_TFTP		"tftp ${cpaddr} ${rootimgname}; nandecc hw 1; nand erase.part root;" \
									"nand write.i ${cpaddr} root ${filesize}; nandecc hw 8"
#define CMD_DOWN_YAFFS_TFTP		"nandecc hw 1;tfyaffs ${cpaddr} ${rootimgname} root;nandecc hw 8"
#define CMD_DOWN_PRAMGAM_TFTP	"echo DOWN_PRAMGAM_TFTP: not implemented"
#define CMD_DOWN_DTB_TFTP		"nand erase.part dtb;tftp ${dtbloadaddr} ${dtbname};nand write ${dtbloadaddr} dtb"
#define CMD_TEST_KERNEL_TFTP	"run boot_args;nand read.i ${dtbloadaddr} dtb;tftp ${kloadaddr} ${kernelimgname}; \
									bootz ${kloadaddr} - ${dtbloadaddr}"
#define CMD_NAND_SCRUB			"nand scrub.chip"
#define CMD_TEST_NET			"ping ${serverip}"

/* main menu title */
#define SET_PARAMS_TITLE 	"Set the boot parameters"
#define ENTER_TFTP_MENU_TITLE "Enter TFTP download mode menu"

/* main menu cmd */
#define CMD_DOWN_MLO_SD		"mmc rescan;fatload mmc 0:1 ${cpaddr} ${mloimgname};nand erase.part SPL; nand write.i ${cpaddr} SPL ${filesize}"
#define CMD_DOWN_UBOOT_SD	"mmc rescan;fatload mmc 0 ${cpaddr} ${ubootimgname};nand erase.part u-boot; nand write.i ${cpaddr} u-boot ${filesize}"
#define CMD_DOWN_KERNEL_SD	"mmc rescan;fatload mmc 0 ${cpaddr} ${kernelimgname};nand erase.part kernel; nand write.i ${cpaddr} kernel ${filesize}"
#define CMD_DOWN_LOGO_SD	"mmc rescan;fatload mmc 0 ${cpaddr} logo_${lcdtype}.bin;nand erase.part logo; nand write.i ${cpaddr} logo ${filesize}"
#define CMD_DOWN_UBIFS_SD	"mmc rescan;fatload mmc 0 ${cpaddr} ${rootimgname};nandecc hw 1; nand erase.part root;" \
								"nand write.i ${cpaddr} root ${filesize}; nandecc hw 8"
#define CMD_DOWN_YAFFS_SD	"mmc rescan;fatload mmc 0 ${cpaddr} ${rootimgname};nandecc hw 1;" \
								"nand erase.part root;nand write.yaffs2 ${cpaddr} root ${filesize};nandecc hw 8"
#define CMD_DOWN_PRAMGAM_SD	"echo DOWN_PRAMGAM_SD: not implemented"
#define CMD_DOWN_DTB_SD		"mmc rescan;nand erase.part dtb;fatload mmc 0 ${dtbloadaddr} ${dtbname};nand write ${dtbloadaddr} dtb"
#define CMD_TEST_KERNEL_SD	"mmc rescan;fatload mmc 0 ${kloadaddr} ${kernelimgname}; bootz ${kloadaddr}"

/* params menu title and cmd */
#define CMD_YAFFS_PARAMS "setenv bootargs console=${console} root=${yaffs_root} rootfstype=yaffs2 rw rootwait=1 init=/init ip=off"
#define CMD_UBIFS_PARAMS "setenv bootargs console=${console} root=${ubi_root} rootfstype=${ubi_root_fs_type} init=/init ip=off"

/****** tftp menu list *******/
static menu_list_t tftp_menu_list[] = {
	{'1', DOWN_MLO_TITLE, 		CMD_DOWN_MLO_TFTP, NULL, NULL},
	{'2', DOWN_UBOOT_TITLE, 	CMD_DOWN_UBOOT_TFTP, NULL, NULL},
	{'3', DOWN_KERNEL_TITLE,	CMD_DOWN_KERNEL_TFTP, NULL, NULL},
	{'4', DOWN_LOGO_TITLE, 		CMD_DOWN_LOGO_TFTP, NULL, NULL},
	{'5', DOWN_UBIFS_TITLE,		CMD_DOWN_UBIFS_TFTP, NULL, NULL},
	{'6', DOWN_YAFFS_TITLE, 	CMD_DOWN_YAFFS_TFTP, NULL, NULL},
	{'7', DOWN_PRAMGAM_TITLE,	CMD_DOWN_PRAMGAM_TFTP, NULL, NULL},
	{'8', BOOT_SYSTEM_TITLE,	CONFIG_START_SYS, NULL, NULL},
	{'9', NAND_SCRUB_TITLE,		CMD_NAND_SCRUB, NULL, NULL},
	{'d', DOWN_DTB_TITLE,		CMD_DOWN_DTB_TFTP, NULL, NULL},
	{'n', SET_TFTP_TITLE,		NULL, set_tftp_params, NULL},
	{'p', TEST_NET_TITLE,		CMD_TEST_NET, NULL, NULL},
	{'r', "Reboot u-boot", 		"reset", NULL, NULL},
	{'t', TEST_KERNEL_TITLE, 	CMD_TEST_KERNEL_TFTP, NULL, NULL},
	{'q', "Return Main menu", 	NULL, NULL, NULL}
};

/****** set params menu *******/
static menu_list_t params_menu_list[] = {
	{'1', "Set NFS boot parameter", NULL, set_nfs_params, NULL},
	{'2', "Set YAffS boot parameter for Android or Standard Linux", CMD_YAFFS_PARAMS, NULL, NULL},
	{'3', "Set UBIfs boot parameter for Android or Standard Linux",	CMD_UBIFS_PARAMS, NULL, NULL},
	{'4', "Set boot parameter", 	NULL, set_boot_params, NULL},
	{'5', "Set parameter",			NULL, set_a_env_params, NULL},
	{'6', "View the parameters", 	NULL, print_a_env_params, NULL},
	{'7', "Select driver and init",	NULL, NULL, &uart_mux_menu_item},
	{'8', "Save the parameters to Nand Flash" ,	"saveenv", NULL, NULL},
	{'c', "Choice lcd type" ,		NULL, NULL, &lcdtype_menu_item},
	{'r', "Reboot u-boot", 			"reset", NULL, NULL},
	{'q', "Return Main menu", 		NULL, NULL, NULL}
};

/****** set lcdtype menu *******/
static menu_list_t lcdtype_menu_list[] = {
	{'1', "T43\" screen.", 			"setenv lcdtype X480Y272;", NULL, NULL},
	{'2', "A70\" screen.", 			"setenv lcdtype X800Y480;", NULL, NULL},
	{'3', "A104\" screen.", 		"setenv lcdtype X800Y600;", NULL, NULL},
	{'4', "H50\" screen.", 			"setenv lcdtype X800Y480_H50;", NULL, NULL},
	{'5', "H70\" screen.", 			"setenv lcdtype X1024Y600_H70;", NULL, NULL},
	{'6', "A133\" screen.", 		"setenv lcdtype X1280Y800;", NULL, NULL},
	{'7', "W35\" screen.", 			"setenv lcdtype X320Y240;", NULL, NULL},
	{'8', "VGA1280X800\" screen.", 	"setenv lcdtype VGA1280X800;", NULL, NULL},
	{'r', "Reboot u-boot",			"reset", NULL, NULL},
	{'q', "Return to the previous menu", NULL, NULL, NULL}
};

/****** set uart mux menu *******/
static menu_list_t uart_mux_menu_list[] = {
	{'1', "select init uart2 driver.", 	"setenv uart2_i2c2 uart2;", NULL, NULL},
	{'2', "select init i2c2 driver.", 	"setenv uart2_i2c2 i2c2;", NULL, NULL},
	{'r', "Reboot u-boot",				"reset", NULL, NULL},
	{'q', "Return to the previous menu", NULL, NULL, NULL}
};

/****** main menu *******/
static menu_list_t main_menu_list[] = {
	{'1', DOWN_MLO_TITLE, 		CMD_DOWN_MLO_SD, NULL, NULL},
	{'2', DOWN_UBOOT_TITLE, 	CMD_DOWN_UBOOT_SD, NULL, NULL},
	{'3', DOWN_KERNEL_TITLE,	CMD_DOWN_KERNEL_SD, NULL, NULL},
	{'4', DOWN_LOGO_TITLE, 		CMD_DOWN_LOGO_SD, NULL, NULL},
	{'5', DOWN_UBIFS_TITLE,		CMD_DOWN_UBIFS_SD, NULL, NULL},
	{'6', DOWN_YAFFS_TITLE, 	CMD_DOWN_YAFFS_SD, NULL, NULL},
	{'7', DOWN_PRAMGAM_TITLE,	CMD_DOWN_PRAMGAM_SD, NULL, NULL},
	{'8', BOOT_SYSTEM_TITLE,	CONFIG_START_SYS, NULL, NULL},
	{'9', NAND_SCRUB_TITLE,		CMD_NAND_SCRUB, NULL, NULL},
	{'0', SET_PARAMS_TITLE,		NULL, NULL, &params_menu_item},
	{'d', DOWN_DTB_TITLE,		CMD_DOWN_DTB_SD, NULL, NULL},
	{'n', ENTER_TFTP_MENU_TITLE, NULL, NULL, &tftp_menu_item},
	{'r', "Reboot u-boot",		"reset", NULL, NULL},
	{'t', TEST_KERNEL_TITLE,	CMD_TEST_KERNEL_SD, NULL, NULL},
	{'q', "Return console", NULL, NULL, NULL}
};

static const menu_item_t main_menu_item={
	"SDCARD MENU"
	, main_menu_list, ARRAY_SIZE(main_menu_list) };
	
static const menu_item_t tftp_menu_item={
	"TFTP MENU"
	, tftp_menu_list, ARRAY_SIZE(tftp_menu_list) };
	
static const menu_item_t params_menu_item={
	"SET PARAMS"
	, params_menu_list, ARRAY_SIZE(params_menu_list) };
	
static const menu_item_t lcdtype_menu_item={
	"SET LCDTYPE"
	, lcdtype_menu_list, ARRAY_SIZE(lcdtype_menu_list) };

static const menu_item_t uart_mux_menu_item={
	"UART MUX MENU"
	, uart_mux_menu_list, ARRAY_SIZE(uart_mux_menu_list) };
	
	
static int parse_menu(const menu_item_t *x, int array_size)
{
	int i;
	char key;
	menu_list_t *menu = x->menu;
	while(1) {
		if(x->title)
		{
			printf("\n#####   " CONFIG_MENU_NAME " U-boot MENU    #####\n");
			printf(  "            %s\n", x->title);
			printf(  "###################################\n\n");
		}
		for(i = 0; i < array_size; i++)
		{
			printf("[%c] %s\n", menu[i].shortcut, menu[i].help);
		}
		key = tolower(getc());
		if(key == 'q')
		{
				return  0;
		}
		for(i = 0; i < array_size; i++)
		{
			if(key == menu[i].shortcut)
			{
				if(menu[i].next_menu_item !=NULL)
					parse_menu(menu[i].next_menu_item, menu[i].next_menu_item->menu_arraysize);		
				else if (menu[i].cmd !=NULL) {
					//printf("%s\n", menu[i].cmd);
					run_command_list(menu[i].cmd, -1, 0);
				}
				else if (menu[i].func !=NULL)
						menu[i].func();
				else
					printf("Nothing to do.\n");
				break;
			}
		}
	}
}

int do_menu(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	parse_menu(&main_menu_item, main_menu_item.menu_arraysize);
	return 0;
}

U_BOOT_CMD(
	menu, 1, 1, do_menu,
	"Terminal command menu",
	"show terminal menu"
);
