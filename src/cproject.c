/**
 ******************************************************************************
 * @file      cproject.c
 * @brief     解析.cproject文件, 生成AutoMake参数
 * @details   This file including all API functions's implement of cproject.c.
 * @copyright Liuning
 ******************************************************************************
 */
 
/*-----------------------------------------------------------------------------
 Section: Includes
 ----------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include "types.h"
#include "maths.h"
#include "param.h"

/*-----------------------------------------------------------------------------
 Section: Type Definitions
 ----------------------------------------------------------------------------*/
/* NONE */

/*-----------------------------------------------------------------------------
 Section: Constant Definitions
 ----------------------------------------------------------------------------*/
#ifdef ERROR
#undef ERROR
#endif

#define ERROR               (-1)

#define FILENAME            "./.cproject"

#define DEFAULT_APP_NAME    "rtos"
#define DEFAULT_SRC_DIR     "./"        /**< 默认源码目录 */
#define DEFAULT_BUILD_DIR   "./_BUILD"  /**< 默认编译根目录 */

#define DEFAULT_COMPILE     "arm-none-eabi-"
#define DEFAULT_CCFLAGS     "-mcpu=cortex-m3 -mthumb -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Werror"
#define DEFAULT_I           "app/inc|bsp/inc|sys/inc|sys/inc/net"
#define DEFAULT_L           "sys/lib|app/meter"
#define DEFAULT_LIBS        "-lsxos -lmeter_sp5 -lm"
#define DEFAULT_LD          "app.ld"
#define DEFAULT_LDFLAGS     " --specs=nano.specs"
#define DEFAULT_EXCLUDE     "bsp/test|sys/test"

/** 定义读文件缓存大小 */
#define READ_BUF_SIZE       (1024u)

/*-----------------------------------------------------------------------------
 Section: Global Variables
 ----------------------------------------------------------------------------*/
/* NONE */

/*-----------------------------------------------------------------------------
 Section: Local Variables
 ----------------------------------------------------------------------------*/
static char the_buf[READ_BUF_SIZE];

/*-----------------------------------------------------------------------------
 Section: Local Function Prototypes
 ----------------------------------------------------------------------------*/
/* NONE */

/*-----------------------------------------------------------------------------
 Section: Global Function Prototypes
 ----------------------------------------------------------------------------*/
/* NONE */

/*-----------------------------------------------------------------------------
 Section: Function Definitions
 ----------------------------------------------------------------------------*/
/**
 ******************************************************************************
 * @brief   默认配置
 * @param[in]  *pfd  : 文件句柄
 * @param[out] *pcfg : 返回结构信息
 *
 * @retval  OK    : 成功
 * @retval  ERROR : 失败
 ******************************************************************************
 */
static status_t
cproject_get_default(pcfg_t *pcfg)
{
    memset(pcfg, 0x00, sizeof(pcfg_t));

    strncpy(pcfg->APP, DEFAULT_APP_NAME, sizeof(pcfg->APP));
    strncpy(pcfg->SRC_DIR, DEFAULT_SRC_DIR, sizeof(pcfg->SRC_DIR));
    strncpy(pcfg->BUILD_DIR, DEFAULT_BUILD_DIR, sizeof(pcfg->BUILD_DIR));

    strncpy(pcfg->CROSS_COMPILE, DEFAULT_COMPILE, sizeof(pcfg->CROSS_COMPILE));
    strncpy(pcfg->CCFLAGS, DEFAULT_CCFLAGS, sizeof(pcfg->CCFLAGS));
    strncpy(pcfg->I, DEFAULT_I, sizeof(pcfg->I));
    strncpy(pcfg->L, DEFAULT_L, sizeof(pcfg->L));
    strncpy(pcfg->LIBS, DEFAULT_LIBS, sizeof(pcfg->LIBS));
    strncpy(pcfg->LD, DEFAULT_LD, sizeof(pcfg->LD));
    strncpy(pcfg->LDFLAGS, DEFAULT_LDFLAGS, sizeof(pcfg->LDFLAGS));
    strncpy(pcfg->EXCLUDE, DEFAULT_EXCLUDE, sizeof(pcfg->EXCLUDE));
    return OK;
}

/**
 ******************************************************************************
 * @brief   遍历文件每一行, 找出匹配字符串的行
 * @param[in]  *pfd  : 文件句柄
 * @param[in]  *pstr : 期望查到的字符串
 *
 * @retval  OK    : 成功
 * @retval  ERROR : 失败
 ******************************************************************************
 */
static status_t
file_find_str(FILE *pfd,
        const char *pstr)
{
    char flag = 0;
    status_t ret = ERROR;

    do
    {
        //1. 移动至头部
        fseek(pfd, 0, SEEK_SET);

        //2. 找到debug字符串
        memset(the_buf, 0x00, sizeof(the_buf));
        while (fgets(the_buf, READ_BUF_SIZE, pfd))
        {
            if (strstr(the_buf, "moduleId=\"org.eclipse.cdt.core.settings\" name=\"Debug\">"))
            {
                flag = 1;
                break;
            }
            memset(the_buf, 0x00, sizeof(the_buf));
        }
        if (flag == 0)
        {
            break;
        }
        flag = 0;

        //3. 找到期望的字符串
        memset(the_buf, 0x00, sizeof(the_buf));
        while (fgets(the_buf, READ_BUF_SIZE, pfd))
        {
            if (strstr(the_buf, pstr))
            {
                flag = 1;
                break;
            }
            memset(the_buf, 0x00, sizeof(the_buf));
        }
        if (flag == 0)
        {
            break;
        }

        //4. 已找到
        ret = OK;
    } while (0);

    return ret;
}

/**
 ******************************************************************************
 * @brief   读取-I
 * @param[in]  *pfd  : 文件句柄
 * @param[out] *pcfg : 返回结构信息
 *
 * @retval  OK    : 成功
 * @retval  ERROR : 失败
 *
 * <option id=... name="Include paths (-I)" ...
 *   <listOptionValue builtIn="false" value="&quot;${workspace_loc:/${ProjName}/app/inc}&quot;"/>
 *   <listOptionValue builtIn="false" value="&quot;${workspace_loc:/${ProjName}/bsp/inc}&quot;"/>
 *   <listOptionValue builtIn="false" value="&quot;${workspace_loc:/${ProjName}/sys/inc}&quot;"/>
 *   <listOptionValue builtIn="false" value="&quot;${workspace_loc:/${ProjName}/sys/inc/net}&quot;"/>
 * </option>
 ******************************************************************************
 */
static status_t
cproject_get_I(FILE *pfd,
        pcfg_t *pcfg)
{
    char *pstr;
    status_t ret = OK;//ERROR;

    do
    {
        if (OK != file_find_str(pfd, "name=\"Include paths (-I)\""))
        {
            break;
        }

        memset(pcfg->I, 0x00, sizeof(pcfg->I));
        memset(the_buf, 0x00, sizeof(the_buf));
        while (fgets(the_buf, sizeof(the_buf), pfd))
        {
            if (strstr(the_buf, "</option>"))
            {
                break;
            }
            pstr = strstr(the_buf, ";${workspace_loc:/${ProjName}/");
            if (!pstr)
            {
                continue; //找不到
            }

            pstr += STR_LEN(";${workspace_loc:/${ProjName}/");
            if (strstr(pstr, "}&quot;"))
            {
                strstr(pstr, "}&quot;")[0] = 0;
                //放到pcfg -I 里
                printf("找到的-I: %s\n", pstr);
                strcat(pcfg->I, pstr); //溢出
                strcat(pcfg->I, "|");
            }
            memset(the_buf, 0x00, sizeof(the_buf));
        }
        ret = OK;
    } while (0);

    return ret;
}

/**
 ******************************************************************************
 * @brief   读取-L
 * @param[in]  *pfd  : 文件句柄
 * @param[out] *pcfg : 返回结构信息
 *
 * @retval  OK    : 成功
 * @retval  ERROR : 失败
 *
 * <option id=... name="Library search path (-L)" ...
 *   <listOptionValue builtIn="false" value="&quot;${workspace_loc:/${ProjName}/sys/lib}&quot;"/>
 *   <listOptionValue builtIn="false" value="&quot;${workspace_loc:/${ProjName}/app/meter}&quot;"/>
 * </option>
 ******************************************************************************
 */
static status_t
cproject_get_L(FILE *pfd,
        pcfg_t *pcfg)
{
    char *pstr;
    status_t ret = OK;//ERROR;

    do
    {
        if (OK != file_find_str(pfd, "name=\"Library search path (-L)\""))
        {
            break;
        }

        memset(pcfg->L, 0x00, sizeof(pcfg->L));
        memset(the_buf, 0x00, sizeof(the_buf));
        while (fgets(the_buf, READ_BUF_SIZE, pfd))
        {
            if (strstr(the_buf, "</option>") || strstr(the_buf, "<option id"))
            {
                break;
            }
            pstr = strstr(the_buf, ";${workspace_loc:/${ProjName}/");
            if (!pstr)
            {
                continue; //找不到
            }

            pstr += STR_LEN(";${workspace_loc:/${ProjName}/");
            if (strstr(pstr, "}&quot;"))
            {
                strstr(pstr, "}&quot;")[0] = 0;
                //放到pcfg -L 里
                printf("找到的-L: %s\n", pstr);
                strcat(pcfg->L, pstr); //溢出
                strcat(pcfg->L, "|");
            }
            memset(the_buf, 0x00, sizeof(the_buf));
        }
        ret = OK;
    } while (0);

    return ret;
}

/**
 ******************************************************************************
 * @brief   读取-l
 * @param[in]  *pfd  : 文件句柄
 * @param[out] *pcfg : 返回结构信息
 *
 * @retval  OK    : 成功
 * @retval  ERROR : 失败
 *
 * <option id=... name="Libraries (-l)" ...
 *   <listOptionValue builtIn="false" value="sxos"/>
 *   <listOptionValue builtIn="false" value="meter_sp5"/>
 *   <listOptionValue builtIn="false" value="m"/>
 * </option>
 ******************************************************************************
 */
static status_t
cproject_get_l(FILE *pfd,
        pcfg_t *pcfg)
{
    char *pstr;
    status_t ret = OK;//ERROR;

    do
    {
        if (OK != file_find_str(pfd, "name=\"Libraries (-l)\""))
        {
            break;
        }

        memset(pcfg->LIBS, 0x00, sizeof(pcfg->LIBS));
        memset(the_buf, 0x00, sizeof(the_buf));
        while (fgets(the_buf, READ_BUF_SIZE, pfd))
        {
            if (strstr(the_buf, "</option>") || strstr(the_buf, "<option id"))
            {
                break;
            }
            pstr = strstr(the_buf, "builtIn=\"false\" value=\"");
            if (!pstr)
            {
                continue; //找不到
            }

            pstr += STR_LEN("builtIn=\"false\" value=\"");
            if (strstr(pstr, "\"/>"))
            {
                strstr(pstr, "\"/>")[0] = 0;
                //放到pcfg -l 里
                printf("找到的-l: %s\n", pstr);
                strcat(pcfg->LIBS, "-l");
                strcat(pcfg->LIBS, pstr); //溢出
                strcat(pcfg->LIBS, " ");
            }
            memset(the_buf, 0x00, sizeof(the_buf));
        }
        ret = OK;
    } while (0);

    return ret;
}
/**
 ******************************************************************************
 * @brief   读取ldflags/ld
 * @param[in]  *pfd  : 文件句柄
 * @param[out] *pcfg : 返回结构信息
 *
 * @retval  OK    : 成功
 * @retval  ERROR : 失败
 * --specs=nano.specs
 ******************************************************************************
 */
static status_t
cproject_get_ldflags(FILE *pfd,
        pcfg_t *pcfg)
{
    status_t ret = OK;

    do
    {
        if (OK == file_find_str(pfd, "--specs=nano.specs"))
        {
            break;
        }
        printf("无--specs=nano.specs\n");
        memset(pcfg->LDFLAGS, 0x00, sizeof(pcfg->LDFLAGS));
    } while (0);

    return ret;
}

/**
 ******************************************************************************
 * @brief   读取ld
 * @param[in]  *pfd  : 文件句柄
 * @param[out] *pcfg : 返回结构信息
 *
 * @retval  OK    : 成功
 * @retval  ERROR : 失败
 *
 * <option id=... name="Script files (-T)" ...
 *   <listOptionValue builtIn="false" value="&quot;${workspace_loc:/${ProjName}/app.ld}&quot;"/>
 * </option>
 ******************************************************************************
 */
static status_t
cproject_get_ld(FILE *pfd,
        pcfg_t *pcfg)
{
    char *pstr;
    status_t ret = OK;//ERROR;

    do
    {
        if (OK != file_find_str(pfd, "name=\"Script files (-T)\""))
        {
            break;
        }

        strncpy(pcfg->LD, "app.ld", sizeof(pcfg->LD));
        memset(the_buf, 0x00, sizeof(the_buf));
        while (fgets(the_buf, READ_BUF_SIZE, pfd))
        {
            pstr = strstr(the_buf, ";${workspace_loc:/${ProjName}/");
            if (!pstr)
            {
                break; //找不到
            }

            pstr += STR_LEN(";${workspace_loc:/${ProjName}/");
            if (strstr(pstr, "}&quot;"))
            {
                strstr(pstr, "}&quot;")[0] = 0;
                //放到pcfg -T 里
                printf("找到的-T: %s\n", pstr);
                strncpy(pcfg->LD, pstr, sizeof(pcfg->LD));
            }
            memset(the_buf, 0x00, sizeof(the_buf));
        }
        ret = OK;
    } while (0);

    return ret;
}
//warnings.toerrors" useByScannerDiscovery="true" value="true" valueType="boolean"/>

/**
 ******************************************************************************
 * @brief   读取ccflags
 * @param[in]  *pfd  : 文件句柄
 * @param[out] *pcfg : 返回结构信息
 *
 * @retval  OK    : 成功
 * @retval  ERROR : 失败
 *
 * warnings.toerrors" useByScannerDiscovery="true" value="true" valueType="boolean"/>
 ******************************************************************************
 */
static status_t
cproject_get_ccflags(FILE *pfd,
        pcfg_t *pcfg)
{
    status_t ret = OK;

    do
    {
        if (OK == file_find_str(pfd, "warnings.toerrors\" useByScannerDiscovery=\"true\" value=\"true\" valueType=\"boolean\"/>"))
        {
            break;
        }
        printf("无-Werror\n");
        if (strstr(pcfg->CCFLAGS, "-Werror"))
        {
            strstr(pcfg->CCFLAGS, "-Werror")[0] = 0;
        }
    } while (0);

    return ret;
}

/**
 ******************************************************************************
 * @brief   读取-D
 * @param[in]  *pfd  : 文件句柄
 * @param[out] *pcfg : 返回结构信息
 *
 * @retval  OK    : 成功
 * @retval  ERROR : 失败
 *
 * <option id=... name="Defined symbols (-D)" ...
 *   <listOptionValue builtIn="false" value="__CORE_SELECTION_=2"/>
 * </option>
 ******************************************************************************
 */
static status_t
cproject_get_D(FILE *pfd,
        pcfg_t *pcfg)
{
    char *pstr;
    status_t ret = OK;//ERROR;

    do
    {
        if (OK != file_find_str(pfd, "name=\"Defined symbols (-D)\""))
        {
            break;
        }

        memset(the_buf, 0x00, sizeof(the_buf));
        while (fgets(the_buf, sizeof(the_buf), pfd))
        {
            if (strstr(the_buf, "</option>") || strstr(the_buf, "<option id"))
            {
                break;
            }
            pstr = strstr(the_buf, "listOptionValue builtIn=\"false\" value=\"");
            if (!pstr)
            {
                continue; //找不到
            }

            pstr += STR_LEN("listOptionValue builtIn=\"false\" value=\"");
            if (strstr(pstr, "\"/>"))
            {
                strstr(pstr, "\"/>")[0] = 0;
                //放到pcfg -D 里
                printf("找到的-D: %s\n", pstr);
                strcat(pcfg->CCFLAGS, "-D"); //溢出
                strcat(pcfg->CCFLAGS, pstr); //溢出
                strcat(pcfg->CCFLAGS, " ");
            }
            memset(the_buf, 0x00, sizeof(the_buf));
        }
        ret = OK;
    } while (0);

    return ret;
}

/**
 ******************************************************************************
 * @brief   读取exlude
 * @param[in]  *pfd  : 文件句柄
 * @param[out] *pcfg : 返回结构信息
 *
 * @retval  OK    : 成功
 * @retval  ERROR : 失败
 *
 * <sourceEntries>
 *   <entry excluding="sys/test|bsp/test|app/dstor/test" .....
 * </sourceEntries>
 ******************************************************************************
 */
static status_t
cproject_get_exlude(FILE *pfd,
        pcfg_t *pcfg)
{
    char *pstr;
    status_t ret = OK;//ERROR;

    do
    {
        if (OK != file_find_str(pfd, "<sourceEntries>"))
        {
            break;
        }

        memset(pcfg->EXCLUDE, 0x00, sizeof(pcfg->EXCLUDE));
        memset(the_buf, 0x00, sizeof(the_buf));
        while (fgets(the_buf, READ_BUF_SIZE, pfd))
        {
            pstr = strstr(the_buf, "<entry excluding=\"");
            if (!pstr)
            {
                break; //找不到
            }

            pstr += STR_LEN("<entry excluding=\"");
            if (strstr(pstr, "\""))
            {
                strstr(pstr, "\"")[0] = 0;
                //放到pcfg -exclude 里
                printf("找到的excludeing: %s\n", pstr);
                strncpy(pcfg->EXCLUDE, pstr, sizeof(pcfg->EXCLUDE));
            }
            memset(the_buf, 0x00, sizeof(the_buf));
        }
        ret = OK;
    } while (0);

    return ret;
}

/**
 ******************************************************************************
 * @brief   尝试获取.cproject创建pcfg_t结构
 * @param[out] *pcfg : 返回结构信息
 *
 * @retval  OK    : 成功
 * @retval  ERROR : 失败
 ******************************************************************************
 */
status_t
cproject_cfg_get(pcfg_t *pcfg)
{
    status_t ret = ERROR;
    FILE *pfd = NULL;

    do
    {
        pfd = fopen(FILENAME, "r");
        if (!pfd)
        {
            break;
        }
        if (OK != cproject_get_default(pcfg))
        {
            break;
        }
        if (OK != cproject_get_I(pfd, pcfg))
        {
            break;
        }
        if (OK != cproject_get_L(pfd, pcfg))
        {
            break;
        }
        if (OK != cproject_get_l(pfd, pcfg))
        {
            break;
        }
        if (OK != cproject_get_ldflags(pfd, pcfg))
        {
            break;
        }
        if (OK != cproject_get_ld(pfd, pcfg))
        {
            break;
        }
        if (OK != cproject_get_exlude(pfd, pcfg))
        {
            break;
        }
        if (OK != cproject_get_ccflags(pfd, pcfg))
        {
            break;
        }
        if (OK != cproject_get_D(pfd, pcfg))
        {
            break;
        }
        ret = OK;
    } while (0);

    if (pfd)
    {
        fclose(pfd);
    }

    return ret;
}


/*-------------------------------cproject.c----------------------------------*/
