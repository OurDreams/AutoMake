/*
 ==============================================================================
 Name        : AutoMake.c
 Author      : LiuNing
 Version     :
 Copyright   : Your copyright notice
 Description : Auto Make Tool for SP4/5
 ==============================================================================
 */

/*-----------------------------------------------------------------------------
 Section: Includes
 ----------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <conio.h>
#include <io.h>

#include "types.h"
#include "maths.h"
#include "ini.h"

/*-----------------------------------------------------------------------------
 Section: Macro Definitions
 ----------------------------------------------------------------------------*/
#define VERSION             "1.0.0"
#define SOFTNAME            "AutoMake"

#define MAX_C_FILES         (128)       /**< ͨһĿ¼�����c�ļ����� */

#define DEFAULT_APP_NAME    "rtos"
#define DEFAULT_SRC_DIR     "./"        /**< Ĭ��Դ��Ŀ¼ */
#define DEFAULT_BUILD_DIR   "./_BUILD"  /**< Ĭ�ϱ����Ŀ¼ */

#define DEFAULT_COMPILE     "arm-none-eabi-"
#define DEFAULT_CCFLAGS     "-mcpu=cortex-m3 -mthumb -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Werror"
#define DEFAULT_I           "app/inc;bsp/inc;sys/inc;sys/inc/net"
#define DEFAULT_L           "sys/lib;app/meter"
#define DEFAULT_LIBS        "-lsxos -lmeter_sp5 -lm"
#define DEFAULT_LD          "app.ld"
#define DEFAULT_LDFLAGS     " --specs=nano.specs"
#define DEFAULT_EXCLUDE     ""  //bsp/test

#define FILE_HEAD           \
    "################################################################################\n"  \
    "# Automatically-generated file. Do not edit! by Liuning\n"                           \
    "################################################################################\n\n"

/*-----------------------------------------------------------------------------
 Section: Type Definitions
 ----------------------------------------------------------------------------*/
/** ������� */
typedef struct
{
    char SRC_DIR[256];          /**< Դ��Ŀ¼ */
    char BUILD_DIR[256];        /**< ����Ŀ¼ */
    char APP[128];              /**< Ӧ�ó������� */
    char CROSS_COMPILE[128];    /**< gcc */
    char I[2048];               /**< -Iͷ�ļ� */
    char CCFLAGS[512];          /**< gcc���� */
    char LDFLAGS[512];          /**< ld���� */
    char L[1024];               /**< -L��̬������·�� */
    char LIBS[512];             /**< -l��̬�� */
    char LD[128];               /**< ld�ļ� */
    char EXCLUDE[1024 * 2];     /**< ����������·�� */
    char OTHER_D[64];           /**< ������-D */
} make_cfg_t;

/*-----------------------------------------------------------------------------
 Section: Local Variables
 ----------------------------------------------------------------------------*/
static pcfg_t the_cfg;
static make_cfg_t make_cfg;

/*-----------------------------------------------------------------------------
 Section: Local Function Prototypes
 ----------------------------------------------------------------------------*/
/* None */

/*-----------------------------------------------------------------------------
 Section: Function Definitions
 ----------------------------------------------------------------------------*/
/**
 ******************************************************************************
 * @brief   ɾ���ļ�
 * @param[in]  *pfile : ��ɾ���ļ�
 *
 * @return  OK    : ɾ���ɹ�
 * @return  ERROR : ɾ��ʧ��
 ******************************************************************************
 */
status_t
file_delete(const char *pfile)
{
    if (remove(pfile))
    {
        return ERROR;
    }
    return OK;
}

/**
 ******************************************************************************
 * @brief   ɾ��Ŀ¼
 * @param[in]  *path : ��ɾ����Ŀ¼
 *
 * @retval  OK    : �ɹ�
 * @retval  ERROR : ʧ��
 ******************************************************************************
 */
static status_t
dir_delete(const char *pdir)
{
    remove(pdir); //todo: �жϷ���ֵ

    return OK;
}

/**
 ******************************************************************************
 * @brief   ����Ŀ¼(֧�ֶ༶Ŀ¼)
 * @param[in]  *path : ��������Ŀ¼
 *
 * @retval  OK    : �ɹ�
 * @retval  ERROR : ʧ��
 ******************************************************************************
 */
static status_t
dir_create(const char *pdir)
{

    int i = 0;
    int iRet;
    int iLen;
    char* pszDir;

    if (NULL == pdir)
    {
        return OK;
    }

    pszDir = strdup(pdir);
    iLen = strlen(pszDir);

//    printf("����Ŀ¼: %s\n", pszDir);
    // �����м�Ŀ¼
    for (i = 0; i < iLen; i++)
    {
        if (pszDir[i] == '\\' || pszDir[i] == '/')
        {
            pszDir[i] = '\0';

            //���������,����
            iRet = access(pszDir, 0);
            if (iRet != 0)
            {
                iRet = mkdir(pszDir);
                if (iRet != 0)
                {
                    free(pszDir);
                    return ERROR;
                }
            }
            //֧��linux,������\����/
            pszDir[i] = '/';
        }
    }

    iRet = mkdir(pszDir);
    free(pszDir);

    return OK;
}

/**
 ******************************************************************************
 * @brief   �ж�·���Ƿ�ΪĿ¼
 * @param[in]  *paddr : ·��
 *
 * @retval   1 : ��Ŀ¼
 * @retval   0 : ���ļ�
 * @retval  -1 : �Ƿ�·��
 ******************************************************************************
 */
int
check_path(const char *path)
{
    struct _stat buf;
    int result = _stat(path, &buf);

    (void)result;
    if (_S_IFDIR & buf.st_mode)
    {
        return 1; //folder
    }
    else if (_S_IFREG & buf.st_mode)
    {
        return 0; //file
    }

    return -1;
}

/**
 ******************************************************************************
 * @brief   ��չĿ¼(����-I -L)
 * @param[in]  *phead : ͷ
 * @param[in]  *pin   : ����Ŀ¼
 * @param[out] *pout  : �����ַ���
 * @param[in]  len    : ���泤��
 *
 * @retval  OK : �ɹ�
 ******************************************************************************
 */
status_t
path_ex(const char *phead,
        const char *pin,
        char *pout,
        int len)
{
    //����: "sys/lib;app/meter"
    //���: -L"../sys/lib" -L"../app/meter"
    char *p;
    char *delim = ";| ";
    char tmp[1024];

    strncpy(tmp, pin, sizeof(tmp));
    p = strtok(tmp, delim);

    while (p)
    {
        pout += sprintf(pout, " %s\"../%s\"", phead, p);
        p = strtok(NULL, delim);
    }

    return OK;
}

/**
 ******************************************************************************
 * @brief   ��ȡ�������
 * @param[out] *pcfg :
 *
 * @retval  OK    : �ɹ�
 * @retval  ERROR : ʧ��
 ******************************************************************************
 */
static status_t
make_cfg_init(make_cfg_t *pcfg)
{
    /*
     * 1. �������ļ��ж�ȡ
     * 2. ��.cproject�ж�ȡ
     * 3. ����Ĭ��ֵ
     */
    if (0 != ini_get_info(&the_cfg))
    {
        printf("ini get info err!\n");
        getch();
        return ERROR;
    }

#if 0
    strncpy(pcfg->APP, DEFAULT_APP_NAME, sizeof(pcfg->APP));
    strncpy(pcfg->CROSS_COMPILE, DEFAULT_COMPILE, sizeof(pcfg->CROSS_COMPILE));
    path_ex("-I", DEFAULT_I, pcfg->I, sizeof(pcfg->I));
    strncpy(pcfg->CCFLAGS, DEFAULT_CCFLAGS, sizeof(pcfg->CCFLAGS));
    path_ex("-L", DEFAULT_L, pcfg->L, sizeof(pcfg->L));
    strncpy(pcfg->LIBS, DEFAULT_LIBS, sizeof(pcfg->LIBS));
    strncpy(pcfg->LD, DEFAULT_LD, sizeof(pcfg->LD));
    strncpy(pcfg->LDFLAGS, DEFAULT_LDFLAGS, sizeof(pcfg->LDFLAGS));
    strncpy(pcfg->EXCLUDE, DEFAULT_EXCLUDE, sizeof(pcfg->EXCLUDE));
#else
    strncpy(pcfg->SRC_DIR, the_cfg.SRC_DIR, sizeof(pcfg->SRC_DIR));
    strncpy(pcfg->BUILD_DIR, the_cfg.BUILD_DIR, sizeof(pcfg->BUILD_DIR));
    strncpy(pcfg->APP, the_cfg.APP, sizeof(pcfg->APP));
    strncpy(pcfg->CROSS_COMPILE, the_cfg.CROSS_COMPILE, sizeof(pcfg->CROSS_COMPILE));
    path_ex("-I", the_cfg.I, pcfg->I, sizeof(pcfg->I));
    strncpy(pcfg->CCFLAGS, the_cfg.CCFLAGS, sizeof(pcfg->CCFLAGS));
    path_ex("-L", the_cfg.L, pcfg->L, sizeof(pcfg->L));
    strncpy(pcfg->LIBS, the_cfg.LIBS, sizeof(pcfg->LIBS));
    strncpy(pcfg->LD, the_cfg.LD, sizeof(pcfg->LD));
    strncpy(pcfg->LDFLAGS, the_cfg.LDFLAGS, sizeof(pcfg->LDFLAGS));
    strncpy(pcfg->EXCLUDE, the_cfg.EXCLUDE, sizeof(pcfg->EXCLUDE));
#endif

    return OK;
}

/**
 ******************************************************************************
 * @brief   �ж�Ŀ¼/�ļ��Ƿ񲻲������
 * @param[in]  *pcfg     : �������
 * @param[in]  *path : ·��
 *
 * @retval  TRUE  : �������
 * @retval  FALSE : ���������
 ******************************************************************************
 */
static bool_e
is_path_need_compile(const make_cfg_t *pcfg,
        const char *path)
{
    //bsp/test|sys/test

    int i;
    char *p;
    char *delim = ";| ";
    char path_tmp[MAX_PATH + 1];
    char exclude_tmp[MAX_PATH + 1];

    if (!strcmp("./", path))
    {
        return TRUE;
    }
    path += 3;

    for (i = 0; (i < MAX_PATH) && path[i]; i++)
    {
        path_tmp[i] = (path[i] == '\\') ? '/' : path[i];
    }
    path_tmp[i] = 0;

    for (i = 0; (i < MAX_PATH) && pcfg->EXCLUDE[i]; i++)
    {
        exclude_tmp[i] = (pcfg->EXCLUDE[i] == '\\') ? '/' : pcfg->EXCLUDE[i];
    }
    exclude_tmp[i] = 0;

    p = strtok(exclude_tmp, delim);

    while (p)
    {
        if (!strcmp(p, path_tmp))
        {
            printf("exclude path: %s\n", path_tmp);
            return FALSE;
        }
        p = strtok(NULL, delim);
    }
    return TRUE;
}

/**
 ******************************************************************************
 * @brief   ���subdir.mk�ļ�
 * @param[in]  *pcfg     : �������
 * @param[in]  *path     : ·��
 * @param[in]  *proot    : ������ʱ·��
 * @param[in]  file_name : �ļ��б�
 * @param[in]  file_cnt  : �ļ�����
 *
 * @retval  OK    : �ɹ�
 * @retval  ERROR : ʧ��
 *
 * @note
 *  1. C_SRCS += xx.c
 *  2. OBJS += xx.o
 *  3. C_DEPS += xx.d
 *  4. app/aid/%.o: ../app/aid/%.c
 *      @echo 'Building file: $<'
 *      @echo 'Invoking: Cross ARM C Compiler'
 *      arm-none-eabi-gcc .....
 *      @echo 'Finished building: $<'
 *      @echo ' '
 ******************************************************************************
 */
status_t
subdir_mk_create(const make_cfg_t *pcfg,
        const char *path,
        const char *proot,
        char file_name[MAX_C_FILES][MAX_PATH],
        int file_cnt)
{
    int i;
    int j;
    bool_e have_S = FALSE;
    char tmp[MAX_PATH];
    FILE *pfd = NULL;
    status_t ret = ERROR;

//    printf("path: %s, root:%s\n", path, proot);
    do
    {
        //����Ŀ¼
        snprintf(tmp, sizeof(tmp), "%s%s", proot, path + 2);
        if (OK != dir_create(tmp))
        {
            break;
        }

        //����subdir.mk�ļ�
        snprintf(tmp, sizeof(tmp), "%s%s/subdir.mk", proot, path + 2);
        pfd = fopen(tmp, "w+");
        if (!pfd)
        {
            break;
        }
        fprintf(pfd, FILE_HEAD);
        fprintf(pfd, "# Add inputs and outputs from these tool invocations to the build variables \n");
        //1. C_SRCS
        fprintf(pfd, "C_SRCS += ");
        for (i = 0; i < file_cnt; i++)
        {
            for (j = 0; (j < MAX_PATH) && file_name[i][j + 3]; j++)
            {
                tmp[j] = (file_name[i][j + 3] == '\\') ? '/' : file_name[i][j + 3];
            }
            if (tmp[j - 1] != 'c') continue; //ֻ��.c�ļ�
            tmp[j - 1] = 0;
            fprintf(pfd, "\\\n../%sc ", tmp); //*.c
        }

        //2. S_UPPER_SRCS
        fprintf(pfd, "\n\nS_UPPER_SRCS += ");
        for (i = 0; i < file_cnt; i++)
        {
            for (j = 0; (j < MAX_PATH) && file_name[i][j + 3]; j++)
            {
                tmp[j] = (file_name[i][j + 3] == '\\') ? '/' : file_name[i][j + 3];
            }
            if (tmp[j - 1] != 'S') continue; //ֻ��.S�ļ�
            tmp[j - 1] = 0;
            fprintf(pfd, "\\\n../%sS ", tmp); //*.S
            have_S = TRUE;
        }

        //3. OBJS
        fprintf(pfd, "\n\nOBJS += ");
        for (i = 0; i < file_cnt; i++)
        {
            for (j = 0; (j < MAX_PATH) && file_name[i][j + 3]; j++)
            {
                tmp[j] = (file_name[i][j + 3] == '\\') ? '/' : file_name[i][j + 3];
            }
            tmp[j - 1] = 0;
            fprintf(pfd, "\\\n./%so ", tmp); //*.o
        }

        //4. S_UPPER_DEPS
        fprintf(pfd, "\n\nS_UPPER_DEPS += ");
        for (i = 0; i < file_cnt; i++)
        {
            for (j = 0; (j < MAX_PATH) && file_name[i][j + 3]; j++)
            {
                tmp[j] = (file_name[i][j + 3] == '\\') ? '/' : file_name[i][j + 3];
            }
            if (tmp[j - 1] != 'S') continue; //ֻ��.S�ļ�
            tmp[j - 1] = 0;
            fprintf(pfd, "\\\n./%sd ", tmp); //*.d
        }

        //5. C_DEPS
        fprintf(pfd, "\n\nC_DEPS += ");
        for (i = 0; i < file_cnt; i++)
        {
            for (j = 0; (j < MAX_PATH) && file_name[i][j + 3]; j++)
            {
                tmp[j] = (file_name[i][j + 3] == '\\') ? '/' : file_name[i][j + 3];
            }
            if (tmp[j - 1] != 'c') continue; //ֻ��.c�ļ�
            tmp[j - 1] = 0;
            fprintf(pfd, "\\\n./%sd ", tmp); //*.d
        }

        fprintf(pfd, "\n\n\n# Each subdirectory must supply rules for building sources it contributes\n");

        for (j = 0; (j < MAX_PATH) && path[j + 3]; j++)
        {
            tmp[j] = (path[j + 3] == '\\') ? '/' : path[j + 3];
        }
        tmp[j] = 0;

        if (have_S) //�л���ļ�
        {
            fprintf(pfd, "%s/%%.o: ../%s/%%.S\n", tmp, tmp);
            fprintf(pfd, "\t@echo 'Building file: $<'\n");
            fprintf(pfd, "\t@echo 'Invoking: Cross ARM GNU Assembler'\n");
            fprintf(pfd, "\t%sgcc %s ", pcfg->CROSS_COMPILE, pcfg->CCFLAGS);
            fprintf(pfd, "-x assembler-with-cpp -MMD -MP -MF\"$(@:%%.o=%%.d)\" -MT\"$(@)\" -c -o \"$@\" \"$<\"\n");
            fprintf(pfd, "\t@echo 'Finished building: $<'\n");
            fprintf(pfd, "\t@echo ' '\n\n");
        }

        fprintf(pfd, "%s/%%.o: ../%s/%%.c\n", tmp, tmp);
        fprintf(pfd, "\t@echo 'Building file: $<'\n");
        fprintf(pfd, "\t@echo 'Invoking: Cross ARM C Compiler'\n");
        if (pcfg->OTHER_D[0])
        {
            fprintf(pfd, "\t%sgcc %s %s%s", pcfg->CROSS_COMPILE, pcfg->CCFLAGS, pcfg->OTHER_D, pcfg->I);
        }
        else
        {
            fprintf(pfd, "\t%sgcc %s%s", pcfg->CROSS_COMPILE, pcfg->CCFLAGS, pcfg->I);
        }
        fprintf(pfd, " -std=gnu11 -MMD -MP -MF\"$(@:%%.o=%%.d)\" -MT\"$(@)\" -c -o \"$@\" \"$<\"\n");
        fprintf(pfd, "\t@echo 'Finished building: $<'\n");
        fprintf(pfd, "\t@echo ' '\n\n\n");

        fclose(pfd);

        ret = OK;
    } while (0);

    return ret;
}

/**
 ******************************************************************************
 * @brief   ��ʼ��sources.mk
 * @param[in]  *proot : ������ʱ·��
 *
 * @retval  NULL : ʧ��
 * @retval !NULL : �ļ����
 *
 * @note    ��Ҫ����SUBDIRS
 ******************************************************************************
 */
FILE *
sources_mk_init(const char *proot)
{
    FILE *pfd = NULL;
    char tmp[MAX_PATH];

    do
    {
        snprintf(tmp, sizeof(tmp), "%s/sources.mk", proot);
        pfd = fopen(tmp, "w+");
        if (!pfd)
        {
            break;
        }
        fprintf(pfd, FILE_HEAD);
        fprintf(pfd, "ELF_SRCS := \n"
                     "OBJ_SRCS := \n"
                     "ASM_SRCS := \n"
                     "C_SRCS := \n"
                     "S_UPPER_SRCS := \n"
                     "O_SRCS := \n"
                     "OBJS := \n"
                     "SECONDARY_FLASH := \n"
                     "SECONDARY_SIZE := \n"
                     "ASM_DEPS := \n"
                     "S_UPPER_DEPS := \n"
                     "C_DEPS := \n"
                     "\n"
                     "# Every subdirectory with source files must be described here\n"
                     "SUBDIRS := \\\n"
                );
    } while (0);

    return pfd;
}

/**
 ******************************************************************************
 * @brief   ��sources.mk�����һ��Ŀ¼
 * @param[in]  *pfd  : �ļ����
 * @param[in]  *path : Ŀ¼
 *
 * @retval  OK    : �ɹ�
 * @retval  ERROR : ʧ��
 ******************************************************************************
 */
status_t
sources_mk_add(FILE *pfd,
        const char *path)
{
    int i;
    char tmp[MAX_PATH + 1] = {0};

    if (pfd)
    {
        path += 3; //ǰ��3�ַ���"./\"
        for (i = 0; (i < MAX_PATH) && path[i]; i++)
        {
            tmp[i] = (path[i] == '\\') ? '/' : path[i];
        }
        fprintf(pfd, "%s \\\n", tmp);
        return OK;
    }
    return ERROR;
}

/**
 ******************************************************************************
 * @brief   �ر�sources.mk
 * @param[in]  *pfd  : �ļ����
 *
 * @retval  OK    : �ɹ�
 * @retval  ERROR : ʧ��
 ******************************************************************************
 */
status_t
sources_mk_end(FILE *pfd)
{
    if (pfd)
    {
        fprintf(pfd, "\n");
        fclose(pfd);
    }
    return OK;
}

/**
 ******************************************************************************
 * @brief   ���objects.mk
 * @param[in]  *pcfg  : �������
 * @param[in]  *proot : ������ʱ·��
 *
 * @retval  OK    : �ɹ�
 * @retval  ERROR : ʧ��
 *
 * @note    ��Ҫ����LIBS := -lsxos -lmeter_sp5 -lm
 ******************************************************************************
 */
status_t
objects_mk_create(const make_cfg_t *pcfg,
        const char *proot)
{
    FILE *pfd = NULL;
    char tmp[MAX_PATH];
    status_t ret = ERROR;

    do
    {
        snprintf(tmp, sizeof(tmp), "%s/objects.mk", proot);
        pfd = fopen(tmp, "w+");
        if (!pfd)
        {
            break;
        }
        fprintf(pfd, FILE_HEAD);

        fprintf(pfd, "USER_OBJS :=\n\nLIBS := %s\n\n", pcfg->LIBS);
        fclose(pfd);
        ret = OK;
    } while (0);

    return ret;
}

/**
 ******************************************************************************
 * @brief   ����makefile
 * @param[in]  *pcfg  : �������
 * @param[in]  *psrc  : Դ��·��
 * @param[in]  *proot : ������ʱ·��
 *
 * @retval  NULL : ʧ��
 * @retval !NULL : �ļ����
 ******************************************************************************
 */
FILE *
makefile_init(const char *proot)
{
    FILE *pfd = NULL;
    char tmp[MAX_PATH];

    do
    {
        snprintf(tmp, sizeof(tmp), "%s/makefile", proot);
        pfd = fopen(tmp, "w+");
        if (!pfd)
        {
            break;
        }
        fprintf(pfd, FILE_HEAD);

        fprintf(pfd, "-include ../makefile.init\n\n"
                     "RM := cs-rm -rf\n\n"
                     "# All of the sources participating in the build are defined here\n"
                     "-include sources.mk\n");
    } while (0);

    return pfd;
}

/**
 ******************************************************************************
 * @brief   ��makefile�м���һ��*.mk�ļ�
 * @param[in]  *pfd  : �ļ����
 * @param[in]  *path : Ŀ¼
 *
 * @retval  OK    : �ɹ�
 * @retval  ERROR : ʧ��
 ******************************************************************************
 */
status_t
makefile_add(FILE *pfd,
        const char *path)
{
    int i;
    char tmp[MAX_PATH + 1] = {0};

    if (pfd)
    {
        path += 3; //ǰ��3�ַ���"./\"
        for (i = 0; (i < MAX_PATH) && path[i]; i++)
        {
            tmp[i] = (path[i] == '\\') ? '/' : path[i];
        }
        fprintf(pfd, "-include %s/subdir.mk\n", tmp);
        return OK;
    }
    return ERROR;
}

/**
 ******************************************************************************
 * @brief   �ر�makefile
 * @param[in]  *pfd   : �ļ����
 * @param[in]  *pcfg  : �������
 *
 * @retval  OK    : �ɹ�
 * @retval  ERROR : ʧ��
 ******************************************************************************
 */
status_t
makefile_end(FILE *pfd,
        const make_cfg_t *pcfg)
{
    if (pfd)
    {
        fprintf(pfd,
                "-include subdir.mk\n"
                "-include objects.mk\n\n"
                "ifneq ($(MAKECMDGOALS),clean)\n"
                "ifneq ($(strip $(ASM_DEPS)),)\n"
                "-include $(ASM_DEPS)\n"
                "endif\n"
                "ifneq ($(strip $(S_UPPER_DEPS)),)\n"
                "-include $(S_UPPER_DEPS)\n"
                "endif\n"
                "ifneq ($(strip $(C_DEPS)),)\n"
                "-include $(C_DEPS)\n"
                "endif\n"
                "endif\n\n"
                "-include ../makefile.defs\n\n"
                "# Add inputs and outputs from these tool invocations to the build variables \n"
                "SECONDARY_FLASH += \\\n"
                "%s.bin \\\n\n"
                "SECONDARY_SIZE += \\\n"
                "%s.siz \\\n\n\n"
                "# All Target\n"
                "all: %s.elf secondary-outputs\n\n",
                pcfg->APP, pcfg->APP, pcfg->APP
                );

        fprintf(pfd,
                "# Tool invocations\n"
                "%s.elf: $(OBJS) $(USER_OBJS)\n"
                "\t@echo 'Building target: $@'\n"
                "\t@echo 'Invoking: Cross ARM C Linker'\n"
                "\t%sgcc %s -T \"../%s\" -Xlinker --gc-sections%s "
                "-Wl,-Map,\"%s.map\" %s -o \"%s.elf\" $(OBJS) $(USER_OBJS) $(LIBS)\n"
                "\t@echo 'Finished building target: $@'\n"
                "\t@echo ' '\n\n",
                pcfg->APP, pcfg->CROSS_COMPILE, pcfg->CCFLAGS, pcfg->LD,
                pcfg->L, pcfg->APP, pcfg->LDFLAGS, pcfg->APP
                );

        fprintf(pfd,
                "%s.bin: %s.elf\n"
                "\t@echo 'Invoking: Cross ARM GNU Create Flash Image'\n"
                "\t%sobjcopy -O binary \"%s.elf\"  \"%s.bin\"\n"
                "\t@echo 'Finished building: $@'\n"
                "\t@echo ' '\n\n",
                pcfg->APP, pcfg->APP, pcfg->CROSS_COMPILE, pcfg->APP, pcfg->APP
                );

        fprintf(pfd,
                "%s.siz: %s.elf\n"
                "\t@echo 'Invoking: Cross ARM GNU Print Size'\n"
                "\t%ssize --format=berkeley \"%s.elf\"\n"
                "\t@echo 'Finished building: $@'\n"
                "\t@echo ' '\n\n",
                pcfg->APP, pcfg->APP, pcfg->CROSS_COMPILE, pcfg->APP
                );

        fprintf(pfd,
                "# Other Targets\n"
                "clean:\n"
                "\t-$(RM) $(OBJS)$(SECONDARY_FLASH)$(SECONDARY_SIZE)$(ASM_DEPS)$(S_UPPER_DEPS)$(C_DEPS) %s.elf\n"
                "\t-@echo ' '\n\n"
                "secondary-outputs: $(SECONDARY_FLASH) $(SECONDARY_SIZE)\n\n"
                ".PHONY: all clean dependents\n"
                ".SECONDARY:\n\n"
                "-include ../makefile.targets",
                pcfg->APP
                );
        fclose(pfd);
    }
    return OK;
}

/**
 ******************************************************************************
 * @brief   ����build.bat
 * @param[in]  *pcfg  : �������
 * @param[in]  *proot : ������ʱ·��
 *
 * @retval  OK    : �ɹ�
 * @retval  ERROR : ʧ��
 ******************************************************************************
 */
status_t
build_bat_create(const make_cfg_t *pcfg,
        const char *proot)
{
    FILE *pfd = NULL;
    char tmp[MAX_PATH];
    status_t ret = ERROR;

    do
    {
        snprintf(tmp, sizeof(tmp), "%s/build.bat", proot);
        pfd = fopen(tmp, "w+");
        if (!pfd)
        {
            break;
        }
        //fprintf(pfd, FILE_HEAD);
        //fprintf(pfd, "cs-make clean\ncs-make all -j8\npause\n");
        fprintf(pfd,
         "@echo off\n"
         "set _time_start=%%time%%\n"
         "set /a hour_start=%%_time_start:~0,2%%\n"
         "set /a minute_start=1%%_time_start:~3,2%%-100\n"
         "set /a second_start=1%%_time_start:~6,2%%-100\n"
         "\n"
         "cs-make clean\n"
         "cs-make all -j8\n"
         "\n"
         "set _time_end=%%time%%\n"
         "set /a hour_end=%%_time_end:~0,2%%\n"
         "set /a minute_end=1%%_time_end:~3,2%%-100\n"
         "set /a second_end=1%%_time_end:~6,2%%-100\n"
         "\n"
         "if %%second_end%% lss %%second_start%% (\n"
         "set /a second_end=%%second_end%%+60\n"
         "set /a minute_end=%%minute_end%%-1\n"
         ")\n"
         "set /a second=%%second_end%%-%%second_start%%\n"
         "if %%minute_end%% lss %%minute_start%% (\n"
         "set /a minute_end=%%minute_end%%+60\n"
         "set /a hour_end=%%hour_end%%-1\n"
         ")\n"
         "set /a minute=%%minute_end%%-%%minute_start%%\n"
         "if %%hour_end%% lss %%hour_start%% (\n"
         "set /a hour_end=%%hour_end%%+24\n"
         ")\n"
         "set /a hour=%%hour_end%%-%%hour_start%%\n"
         "echo ��ʱ: %%hour%%:%%minute%%:%%second%%\n"
         "pause\n"
          );
        fclose(pfd);
        ret = OK;
    } while (0);

    return ret;
}

/**
 ******************************************************************************
 * @brief   �ݹ����Դ��·��, ����subdir.mk ͬʱ����makefile��sources.mk
 * @param[in]  *pcfg        : �������
 * @param[in]  *dir         : Դ��·��
 * @param[in]  *proot       : ������ʱ·��
 * @param[in]  *pmakefile   : makefile�ļ����
 * @param[in]  *psources_mk : sources_mk�ļ����
 *
 * @retval  OK    : �ɹ�
 * @retval  ERROR : ʧ��
 ******************************************************************************
 */
status_t
traverse_src(const make_cfg_t *pcfg,
        const char *dir,
        const char *proot,
        FILE *pmakefile,
        FILE *psources_mk)
{
    if (TRUE != is_path_need_compile(pcfg, dir))
    {
        return OK; //����Ҫ�������
    }

    status_t ret = ERROR;
    char szFind[MAX_PATH], szFile[MAX_PATH];
    WIN32_FIND_DATA FindFileData;
    strcpy(szFind, dir);
    strcat(szFind, "\\*.*");
    HANDLE hFind = FindFirstFile(szFind, &FindFileData);
    if (INVALID_HANDLE_VALUE == hFind)
    {
        return ERROR;
    }

    int file_cnt = 0; //��Ŀ¼��c�ļ�����
    char file_name[MAX_C_FILES][MAX_PATH]; //todo: ����Ǹ���malloc

    memset(file_name, 0x00, sizeof(file_name));

    while (TRUE)
    {
        if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (FindFileData.cFileName[0] != '.') //�Ҳ�Ϊproot
            {
                strcpy(szFile, dir);
                strcat(szFile, "\\");
                strcat(szFile, FindFileData.cFileName);
                if (strcmp(szFile + 3, pcfg->BUILD_DIR + 2)) //fixme: �պ�����
                {
                    if (OK != traverse_src(pcfg, szFile, proot, pmakefile, psources_mk))
                    {
                        return ERROR;
                    }
                }
            }
        }
        else
        {
            if (FindFileData.cFileName[0] != '.')
            {
                //ֻ����c/S�ļ�
                int len = strlen(FindFileData.cFileName);
                if ((FindFileData.cFileName[len - 2] == '.')
                        && ((FindFileData.cFileName[len - 1] == 'c')
                                || (FindFileData.cFileName[len - 1] == 'S')))
                {
                    strcpy(szFile, dir);
                    strcat(szFile, "\\");
                    strcat(szFile, FindFileData.cFileName);
                    if (TRUE == is_path_need_compile(pcfg, szFile))
                    {
                        strncpy(file_name[file_cnt], szFile, MAX_PATH);
                        file_cnt++;
                    }
                }
            }
        }
        if (!FindNextFile(hFind, &FindFileData))
            break;
    }

    do
    {
        if (file_cnt <= 0)
        {
            ret = OK;
            break;
        }
        //����subdir.mk
        if (OK != subdir_mk_create(pcfg, dir, proot, file_name, file_cnt))
        {
            break;
        }
        //����sources.mk
        if (OK != sources_mk_add(psources_mk, dir))
        {
            break;
        }
        //����makefile
        if (OK != makefile_add(pmakefile, dir))
        {
            break;
        }
        ret = OK;
    } while (0);

    return ret;
}

/**
 ******************************************************************************
 * @brief   AutoMake������
 * @param[in]  *pcfg  : �������
 * @param[in]  *psrc  : Դ��·��
 * @param[in]  *proot : ������ʱ·��
 *
 * @retval  OK    : �ɹ�
 * @retval  ERROR : ʧ��
 ******************************************************************************
 */
status_t
auto_make_bulid(const make_cfg_t *pcfg,
        const char *psrc,
        const char *proot)
{
    status_t ret = ERROR;
    FILE *pmakefile = NULL;
    FILE *psources_mk = NULL;

    do
    {
        //1. ɾ��������ʱ·��(�����ͨ�������ж�)
        if (OK != dir_delete(proot))
        {
            break;
        }

        //2. ����������ʱ·��
        if (OK != dir_create(proot))
        {
            break;
        }

        //3. ����objects.mk�ļ�
        if (OK != objects_mk_create(pcfg, proot))
        {
            break;
        }

        //4. ��ʼ��makefile�ļ�
        pmakefile = makefile_init(proot);
        if (!pmakefile)
        {
            break;
        }

        //5. ��ʼ��sources.mk�ļ�
        psources_mk = sources_mk_init(proot);
        if (!psources_mk)
        {
            break;
        }

        //6. �ݹ����Դ����Ŀ¼, ����subdir.mk ͬʱ����makefile��sources.mk
        if (OK != traverse_src(pcfg, psrc, proot, pmakefile, psources_mk))
        {
            break;
        }

        //7. ��βsources.mk
        if (OK != sources_mk_end(psources_mk))
        {
            break;
        }

        //8. ��βmakefile
        if (OK != makefile_end(pmakefile, pcfg))
        {
            break;
        }

        //9. ����������build.bat�ļ�
        if (OK != build_bat_create(pcfg, proot))
        {
            break;
        }

        ret = OK;
    } while (0);

    if (pmakefile)
    {
        fclose(pmakefile);
    }
    if (psources_mk)
    {
        fclose(psources_mk);
    }

    return ret;
}

/**
 ******************************************************************************
 * @brief   �Զ�����������
 * @param[in]  argc   : �����в�������
 * @param[in]  **argv : �����в����б�
 *
 * @retval  EXIT_SUCCESS : �ɹ�
 * @retval  EXIT_FAILURE : ʧ��
 ******************************************************************************
 */
int main(int argc,
        char **argv)
{
    int ret = EXIT_FAILURE;
    time_t start;

    make_cfg.OTHER_D[0] = 0;
    if ((argc == 2) && (argv[1][0] == '-') && (argv[1][1] == 'D'))
    {
        strncpy(make_cfg.OTHER_D, argv[1], sizeof(make_cfg.OTHER_D));
    }

	printf("!!!SP4 Auto Make v%s by LiuNing!!!\n", VERSION);

    //���û���ʾȷ��
    printf("����ǰ���ñ��ݹ������밴�����������\n");
//    getch();
    printf("��ʼ����makefile...\n");
    start = time(NULL);

    //1. ��svn���ش���

    //2. ��ȡ�������
    if (OK != make_cfg_init(&make_cfg))
    {
        printf("��ȡ�������ʧ�ܣ�\n");
        goto __exit;
    }

    //3. ����makefile
    if (OK != auto_make_bulid(&make_cfg, make_cfg.SRC_DIR, make_cfg.BUILD_DIR))
    {
        printf("����makefileʧ�ܣ�\n");
        goto __exit;
    }

    //4. ִ��make

    //5. ��������ļ�

    printf("�������ܺ�ʱ:%ds\n", abs(time(NULL) - start));
    ret = EXIT_SUCCESS;
__exit:
//    printf("�밴������˳�!");
//    getch();
	return ret;
}

/*------------------------------AutoMake.c-----------------------------------*/
