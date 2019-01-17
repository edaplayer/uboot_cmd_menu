/*
 * (C) Copyright 2016
 * Richard, GuangDong Linux Center, <edaplayer@163.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#define CONFIG_PC
/* #define CONFIG_UBOOT */

#ifdef CONFIG_PC
#include <stdio.h>
#include <ctype.h>
#elif defined(CONFIG_UBOOT)
#include <common.h>
#include <command.h>
#include <linux/ctype.h>
#endif

#ifdef CONFIG_PC
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

typedef struct menu_list{
    const char shortcut;
    const char *help;
    const char *cmd;
    void (*func)(void);
    const struct menu_item *next_menu_item;
} menu_list_t;

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
static const menu_item_t tftp_menu_item;
static const menu_item_t main_menu_item;

#if 0
static void set_tftp_params(void)
{
    system("echo set_tftp_params");
}
#else
extern char console_buffer[];
extern int menu_readline(const char *const prompt);

static const params_t tftp_params[] = {
    {"serverip","Enter the TFTP Server(PC) IP address:(xxx.xxx.xxx.xxx)"},
    {"ipaddr", "Enter the IP address:(xxx.xxx.xxx.xxx)"},
    {"netmask", "Enter the netmask address:(xxx.xxx.xxx.xxx)"},
    {"mloimgname", "Enter the SPL image nanme"},
    {"ubootimgname", "Enter the u-boot image nanme"},
    {"kernelimgname", "Enter the kernel image nanme"},
    {"rootimgname", "Enter the rootfs image nanme"},
    {"dtbimgname", "Enter the dtb image nanme"},
};

static void set_params(const params_t *const params, int array_size)
{
    int i;
#ifdef CONFIG_UBOOT
    console_buffer[0] = '\0';
#endif
    char cmd_buf[200];
    char *env_pt = NULL;
    for(i = 0; i < array_size; i++){
        printf("%s\n", params[i].promt);
#ifdef CONFIG_UBOOT
        env_pt = getenv(params[i].envname);
        menu_readline(env_pt);
        sprintf(cmd_buf, "setenv %s %s",params[i].envname,console_buffer);
        run_command(cmd_buf, 0);
#else
        printf("%s\n", params[i].envname);
#endif
    }

    char key;
    printf("Save parameters?(y/n)\n");
    key = getchar();
    printf("first getchar() key = %d\n", key);//test
    if (key == 'y' )
    {
        key = getchar();//回车符
        printf("second getchar() key = %d\n", key);//test
        if(key == '\r' || key == '\n'){
#ifdef CONFIG_UBOOT
            run_command("saveenv", 0);
#elif defined(CONFIG_PC)
            printf("saveenv\n");
#endif
        }
        else
            puts("save aborted\n");
    }
    else
    {
        getchar();//回车符
        printf("Not Save it!!!\n");
    }

}

static void set_tftp_params(void)
{
    set_params(tftp_params, ARRAY_SIZE(tftp_params));
}
#endif

/****** tftp menu *******/
static menu_list_t tftp_menu_list[] = {
    {'1', "submenu help 1", "echo menu1 cmd", NULL, NULL},
    {'2', "submenu help 2", "echo menu2 cmd", NULL, NULL},
    {'3', "Set TFTP parameters(serverip,ipaddr,imagename...)", NULL, set_tftp_params, NULL},
    {'q', "Return Main menu", NULL, NULL, NULL}
};
/****** main menu *******/
static menu_list_t main_menu_list[] = {
    {'1', "Go to SD Download menu", NULL, NULL, NULL},
    {'2', "menu help 2", "echo menu2 cmd", NULL, NULL},
    {'3', "Go to Tftp Download menu", NULL, NULL, &tftp_menu_item},
    {'q', "Return console", NULL, NULL, NULL}
};

static const menu_item_t main_menu_item={
    "MAIN menu"
        , main_menu_list, ARRAY_SIZE(main_menu_list)};

static const menu_item_t tftp_menu_item={
    "TFTP menu"
        , tftp_menu_list, ARRAY_SIZE(tftp_menu_list)};

int count = 0;
static int parse_menu(const menu_item_t *x, int array_size)
{
    int i;
    char key;

    menu_list_t *menu = x->menu;

    while(1) {
        if(x->title)
        {
            printf("############################\n");
            printf("         %s\n", x->title);
            printf("############################\n");
        }
        for(i = 0; i < array_size; i++)
        {
            printf("[%c] %s\n", menu[i].shortcut, menu[i].help);
        }
#ifdef CONFIG_PC
        key = getc(stdin);
        //key = getc();
        //setbuf(stdin,NULL);//清空stdin
        //setbuf(stdin,NULL);
        getc(stdin);//吸收回车符
        //scanf("%d", &key);
        count++;
		//printf("key = %d, count = %d\n", key, count);//test
#elif defined(CONFIG_UBOOT)
        key = getc();
#endif
        key = tolower(key);
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
                else if(menu[i].cmd !=NULL)
#ifdef CONFIG_PC
                    system(menu[i].cmd);
#else
                    run_command_list(menu[i].cmd, -1, 0);
#endif
                else if(menu[i].func !=NULL)
                    menu[i].func();
                break;
            }
        }
    }
}

int main(int argc, char *const argv[])
{
    parse_menu(&main_menu_item, main_menu_item.menu_arraysize);
    return 0;
}

