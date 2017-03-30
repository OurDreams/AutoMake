/**
 ******************************************************************************
 * @file      ini.c
 * @brief     C Source file of ini.c.
 * @details   This file including all API functions's 
 *            implement of ini.c.	
 *
 * @copyright
 ******************************************************************************
 */
 
/*-----------------------------------------------------------------------------
 Section: Includes
 ----------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include "iniparser.h"
#include "ini.h"
#include "types.h"

/*-----------------------------------------------------------------------------
 Section: Type Definitions
 ----------------------------------------------------------------------------*/
/* NONE */

/*-----------------------------------------------------------------------------
 Section: Constant Definitions
 ----------------------------------------------------------------------------*/
#ifndef DEFAULT_INI_FILE
# define DEFAULT_INI_FILE   "./AutoMake.ini"
#endif

/*-----------------------------------------------------------------------------
 Section: Global Variables
 ----------------------------------------------------------------------------*/
/* NONE */

/*-----------------------------------------------------------------------------
 Section: Local Variables
 ----------------------------------------------------------------------------*/
/* NONE */

/*-----------------------------------------------------------------------------
 Section: Local Function Prototypes
 ----------------------------------------------------------------------------*/
/* NONE */

/*-----------------------------------------------------------------------------
 Section: Global Function Prototypes
 ----------------------------------------------------------------------------*/
extern status_t
cproject_cfg_get(pcfg_t *pcfg);

/*-----------------------------------------------------------------------------
 Section: Function Definitions
 ----------------------------------------------------------------------------*/
/**
 ******************************************************************************
 * @brief   创建默认配置文件
 * @return  None
 ******************************************************************************
 */
static void
create_example_ini_file(pcfg_t *pinfo)
{
    FILE * ini;

    ini = fopen(DEFAULT_INI_FILE, "w");

    if (!pinfo)
    fprintf(ini,
            "#AutoMake配置By LiuNing\n"
            "[cfg]\n"

            "#源码目录\n"
            "SRC_DIR            = ./\n\n"

            "#编译目录\n"
            "BUILD_DIR          = ./_BUILD\n\n"

            "#应用程序名字\n"
            "APP                = rtos\n\n"

            "#交叉编译器\n"
            "CROSS_COMPILE      = arm-none-eabi-\n\n"

            "#-I头文件(用|分割)\n"
            "I                  = app/inc|bsp/inc|sys/inc|sys/inc/net\n\n"

            "#gcc参数\n"
            "CCFLAGS            = -mcpu=cortex-m3 -mthumb -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Werror\n\n"

            "#ld参数\n"
            "LDFLAGS            = --specs=nano.specs\n\n"

            "#-L静态库搜索路径(用|分割)\n"
            "L                  = sys/lib|app/meter\n\n"

            "#-l静态库\n"
            "LIBS               = -lsxos -lmeter_sp5 -lm\n\n"

            "#ld文件\n"
            "LD                 = app.ld\n\n"

            "#不参与编译的路径(用|分割)\n"
            "EXCLUDE            = sys/test|bsp/test\n\n"
            );
    else
    {
        fprintf(ini,
            "#AutoMake配置By LiuNing\n"
            "[cfg]\n"

            "#源码目录\n"
            "SRC_DIR            = ./\n\n"

            "#编译目录\n"
            "BUILD_DIR          = ./_BUILD\n\n"

            "#应用程序名字\n"
            "APP                = rtos\n\n"

            "#交叉编译器\n"
            "CROSS_COMPILE      = arm-none-eabi-\n\n"

            "#-I头文件(用|分割)\n"
            "I                  = %s\n\n"

            "#gcc参数\n"
            "CCFLAGS            = %s\n\n"

            "#ld参数\n"
            "LDFLAGS            = %s\n\n"

            "#-L静态库搜索路径(用|分割)\n"
            "L                  = %s\n\n"

            "#-l静态库\n"
            "LIBS               = %s\n\n"

            "#ld文件\n"
            "LD                 = %s\n\n"

            "#不参与编译的路径(用|分割)\n"
            "EXCLUDE            = %s\n\n",

            pinfo->I,
            pinfo->CCFLAGS,
            pinfo->LDFLAGS,
            pinfo->L,
            pinfo->LIBS,
            pinfo->LD,
            pinfo->EXCLUDE
            );
    }
    fclose(ini);
}

/**
 ******************************************************************************
 * @brief   从配置文件中获取文件合并信息
 * @param[out] *pinfo   : 返回info
 *
 * @retval     -1 失败
 * @retval      0 成功
 ******************************************************************************
 */
int
ini_get_info(pcfg_t *pinfo)
{
    dictionary * pini;
    char *pstr = NULL;

    memset(pinfo, 0x00, sizeof(*pinfo));

    pini = iniparser_load(DEFAULT_INI_FILE);
    if (NULL == pini)
    {
        //尝试从.cproject中读取配置
        if (OK == cproject_cfg_get(pinfo))
        {
            //创建默认ini
            create_example_ini_file(pinfo);
            memset(pinfo, 0x00, sizeof(*pinfo));
        }
        else
        {
            create_example_ini_file(NULL);
        }
        pini = iniparser_load(DEFAULT_INI_FILE);
        if (pini == NULL)
        {
            return -1;
        }

//        //尝试从.cproject中读取配置
//        if (OK == cproject_cfg_get(pinfo))
//        {
//            //写入文件
//            iniparser_set(pini, "cfg:I", pinfo->I);
//            iniparser_set(pini, "cfg:L", pinfo->L);
//            iniparser_set(pini, "cfg:LIBS", pinfo->LIBS);
//            iniparser_set(pini, "cfg:LD", pinfo->LD);
//            iniparser_set(pini, "cfg:LDFLAGS", pinfo->LDFLAGS);
//            iniparser_set(pini, "cfg:EXCLUDE", pinfo->EXCLUDE);
//            iniparser_dump_ini(pini)
//        }
    }

    iniparser_dump(pini, NULL);//stderr

    pstr = iniparser_getstring(pini, "cfg:SRC_DIR", NULL);
    if (pstr == NULL)
    {
        iniparser_freedict(pini);
        return -1;
    }
    strncpy(pinfo->SRC_DIR, pstr, sizeof(pinfo->SRC_DIR));

    pstr = iniparser_getstring(pini, "cfg:BUILD_DIR", NULL);
    if (pstr == NULL)
    {
        iniparser_freedict(pini);
        return -1;
    }
    strncpy(pinfo->BUILD_DIR, pstr, sizeof(pinfo->BUILD_DIR));

    pstr = iniparser_getstring(pini, "cfg:APP", NULL);
    if (pstr == NULL)
    {
        iniparser_freedict(pini);
        return -1;
    }
    strncpy(pinfo->APP, pstr, sizeof(pinfo->APP));

    pstr = iniparser_getstring(pini, "cfg:CROSS_COMPILE", NULL);
    if (pstr == NULL)
    {
        iniparser_freedict(pini);
        return -1;
    }
    strncpy(pinfo->CROSS_COMPILE, pstr, sizeof(pinfo->CROSS_COMPILE));

    pstr = iniparser_getstring(pini, "cfg:I", NULL);
    if (pstr == NULL)
    {
        iniparser_freedict(pini);
        return -1;
    }
    strncpy(pinfo->I, pstr, sizeof(pinfo->I));

    pstr = iniparser_getstring(pini, "cfg:CCFLAGS", NULL);
    if (pstr == NULL)
    {
        iniparser_freedict(pini);
        return -1;
    }
    strncpy(pinfo->CCFLAGS, pstr, sizeof(pinfo->CCFLAGS));

    pstr = iniparser_getstring(pini, "cfg:LDFLAGS", NULL);
    if (pstr == NULL)
    {
        iniparser_freedict(pini);
        return -1;
    }
    strncpy(pinfo->LDFLAGS, pstr, sizeof(pinfo->LDFLAGS));

    pstr = iniparser_getstring(pini, "cfg:L", NULL);
    if (pstr == NULL)
    {
        iniparser_freedict(pini);
        return -1;
    }
    strncpy(pinfo->L, pstr, sizeof(pinfo->L));

    pstr = iniparser_getstring(pini, "cfg:LIBS", NULL);
    if (pstr == NULL)
    {
        iniparser_freedict(pini);
        return -1;
    }
    strncpy(pinfo->LIBS, pstr, sizeof(pinfo->LIBS));

    pstr = iniparser_getstring(pini, "cfg:LD", NULL);
    if (pstr == NULL)
    {
        iniparser_freedict(pini);
        return -1;
    }
    strncpy(pinfo->LD, pstr, sizeof(pinfo->LD));

    pstr = iniparser_getstring(pini, "cfg:EXCLUDE", NULL);
    if (pstr == NULL)
    {
        iniparser_freedict(pini);
        return -1;
    }
    strncpy(pinfo->EXCLUDE, pstr, sizeof(pinfo->EXCLUDE));

    iniparser_freedict(pini);

    return 0;
}

/*---------------------------------ini.c-------------------------------------*/
