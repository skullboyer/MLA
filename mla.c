/**
 * @file mla.c
 * @author skull (skull.gu@gmail.com)
 * @brief
 * @version 0.1
 * @date 2022-08-03
 *
 * @copyright Copyright (c) 2023 skull
 *
 */
#include "adapter.h"

#define TAG    "MLA"
#define MEM_ID_SIZE    (4)  // sizeof(Hash("file: line"))
#define MLA_MONITOR_INFO    1  // 监控所有内存使用信息
#define CFG_MLA_VERBOSE     1  // 记录释放位置，进一步定位泄漏位置
#define CFG_MLA_FUNCTION    1  // 内存使用信息携带函数名
#define MLA_HASH_VERIFY     1  // 检查hash值是否重复

#define MLA_OUTPUT(...)      LOGV(__VA_ARGS__); LOGV("\r\n");

#if CFG_MLA_FUNCTION && CFG_MLA_VERBOSE
#define BUFFER_SIZE    (80)
static const char * const MlaTitle = "****************************************************** Memory Leak Analyzer ******************************************************";
static const char * const SplitLine = "*--------------------------------------------------------------------------------------------------------------------------------*";
static const char * const OVSplitLine = "*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%  MLA  Verbose  %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*";
#elif CFG_MLA_FUNCTION
#define BUFFER_SIZE    (48)
static const char * const MlaTitle = "********************************************** Memory Leak Analyzer **********************************************";
#else
#define BUFFER_SIZE    (48)
static const char * const MlaTitle = "************************************** Memory Leak Analyzer **************************************";
static const char * const SplitLine = "*------------------------------------------------------------------------------------------------*";
static const char * const OVSplitLine = "*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%  MLA  Verbose  %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*";
#endif

#if CFG_MLA_VERBOSE
typedef struct {
    mla_list_node_t node;
    uint32_t hash;
    uint32_t line;
    char file[32];
#if CFG_MLA_FUNCTION
    char func[32];
#endif
    uint32_t freeCount;
} MlaFreeInfo_t;
#endif

/* 内存分配记录器结构，可用来记录内存使用状态，借助shell或文件方便查看是否存在内存泄漏 */
typedef struct {
    mla_list_node_t node;
    uint32_t hash;
    uint32_t line;
    char file[32];
#if CFG_MLA_FUNCTION
    char func[32];
#endif
    uint32_t mallocCount;
    uint32_t freeCount;
#if CFG_MLA_VERBOSE
    uint32_t size;
    mla_list_node_t freeInfo;
#endif
} Mla_t;

typedef struct {
    uint16_t verboseIndex;
    Mla_t mla;
} VerbosePrintInfo_t;

static mla_list_node_t recorderList;
static uint16_t mlaIndex;

static int8_t assert_abort(void)
{
    LOGE("xxxxxxxxxxx");
    for(;;);
}

#if 0
typedef void (*OutputRecorder)(Mla_t);
void MlaRegisterBackend()
{
    /* 实现信息记录重定位到文件等端口，可采用如下两种方式 */
    // 1. 按行记录，重映射宏MLA_OUTPUT，实现可变参数接口写入文件
    #define MLA_OUTPUT(...) \
        do { \
            FileWrite(__VA_ARGS__); \
        } while(0)
    // 2. 函数指针绑定，可实现shell动态修改输出方式

}
#endif

#if CFG_MLA_VERBOSE
static int MlaCheckFree(void **p_arg, mla_list_node_t **p_node)
{
    CHECK(*p_arg != NULL, -1);
    CHECK(*p_node != NULL, -1);
    MlaFreeInfo_t *recorder = (MlaFreeInfo_t *)(*p_node);
    if (recorder->hash == *((uint32_t *)(*p_arg))) {
        return 1;
    } else {
        return 0;
    }
}

static Mla_t *MlaFindFreeItem(mla_list_node_t *head, uint32_t hash)
{
    CHECK(head != NULL, NULL);
    return (MlaFreeInfo_t *)mla_slist_foreach(head, MlaCheckFree, &hash);
}
#if MLA_DEBUG
static int PrintListInfo(void **p_arg, mla_list_node_t **p_node)
{
    MlaFreeInfo_t *info = (MlaFreeInfo_t *)(*p_node);
    MLA_LOG("----------------------------------------------------------------");
    MLA_LOG("%p, %u, %u, %s, %u", &info->node, info->hash, info->line, info->file, info->freeCount);
    MLA_LOG("----------------------------------------------------------------");
    return 0;
}
#endif
static void MlaAddFreeItem(mla_list_node_t *head, MlaFreeInfo_t *item)
{
    CHECK(head != NULL);
    CHECK(item != NULL);
    LOGD("%s - %s. %s:%u", __FILENAME__, __func__, item->file, item->line);
    mla_list_add_tail(head, &item->node);
#if MLA_DEBUG
    mla_slist_foreach(head, PrintListInfo, NULL);
#endif
}

static int MlaProcessFreeNode(void **p_arg, mla_list_node_t **p_node)
{
    CHECK(*p_arg != NULL, -1);
    CHECK(*p_node != NULL, -1);
    mla_list_del(*p_arg, *p_node);
    MLA_FREE(*p_node);
    *p_node = *p_arg;
    return 0;
}

static void MlaDelFreeItem(Mla_t *item)
{
    CHECK(item != NULL);
    LOGD("%s - %s. %s:%u", __FILENAME__, __func__, item->file, item->line);
    mla_slist_foreach(&item->freeInfo, MlaProcessFreeNode, &item->freeInfo);
}
#endif

static int MlaProcess(void **p_arg, mla_list_node_t **p_node)
{
    CHECK(*p_arg != NULL, -1);
    CHECK(*p_node != NULL, -1);
    Mla_t *recorder = (Mla_t *)(*p_node);
    if (recorder->hash == *((uint32_t *)(*p_arg))) {
        return 1;
    } else {
        return 0;
    }
}

static Mla_t *MlaFindItem(mla_list_node_t *head, uint32_t hash)
{
    CHECK(head != NULL, NULL);
    return (Mla_t *)mla_slist_foreach(head, MlaProcess, &hash);
}

static void MlaAddItem(mla_list_node_t *head, Mla_t *item)
{
    CHECK(head != NULL);
    CHECK(item != NULL);
    LOGD("%s - %s. %s:%u", __FILENAME__, __func__, item->file, item->line);
    mla_list_add_tail(head, &item->node);
}

static void MlaDelItem(mla_list_node_t *head, Mla_t *item)
{
    CHECK(head != NULL);
    CHECK(item != NULL);
    LOGD("%s - %s. %s:%u", __FILENAME__, __func__, item->file, item->line);
#if CFG_MLA_VERBOSE
    MlaDelFreeItem(item);
#endif
    mla_list_del(head, &item->node);
    MLA_FREE(item);
}

static int MlaMallocRecorder(char *file, char *func, uint16_t line, uint32_t size, uint32_t hash)
{
    CHECK(file != NULL);
#if CFG_MLA_FUNCTION
    CHECK(func != NULL, NULL);
#else
    UNUSED(func);
#endif
    Mla_t *item = MlaFindItem(&recorderList, hash);
    if (item == NULL) {
        Mla_t *mrecorder = (Mla_t *)MLA_MALLOC(sizeof(Mla_t));
        if (mrecorder == NULL) {
            LOGE("%s - %s : %u. malloc fail!", __FILENAME__, __func__, __LINE__);
            return -1;
        }
        mrecorder->hash = hash;
        mrecorder->line = line;
        mrecorder->mallocCount = 1;
        mrecorder->freeCount = 0;
#if CFG_MLA_VERBOSE
        mrecorder->size = size;
        mla_list_init(&mrecorder->freeInfo);
#endif
        memcpy(mrecorder->file, file, sizeof(mrecorder->file));
#if CFG_MLA_FUNCTION
        memcpy(mrecorder->func, func, sizeof(mrecorder->func));
#endif
        MlaAddItem(&recorderList, &mrecorder->node);
    } else {
        item->mallocCount += 1;
#if MLA_HASH_VERIFY
#if CFG_MLA_VERBOSE
#if CFG_MLA_FUNCTION
        if (item->line != line || item->size != size || !!strcmp(item->file, file) || !!strcmp(item->func, func)) {
#else
        if (item->line != line || item->size != size || !!strcmp(item->file, file)) {
#endif
#else
#if CFG_MLA_FUNCTION
        if (item->line != line || !!strcmp(item->file, file) || !!strcmp(item->func, func)) {
#else
        if (item->line != line || !!strcmp(item->file, file)) {
#endif
#endif
            LOGE("hint hash dumplicate: %x, %s:%u %s, %u", hash, file, line, func, size);
            return -2;
        }
#endif
    }
    return 0;
}

static int MlaFreeRecorder(char *file, char *func, uint16_t line, uint32_t hash)
{
    Mla_t *item = MlaFindItem(&recorderList, hash);
    if (item == NULL) {
        LOGE("%s - %s. The freed memory does not exist", __FILENAME__, __func__);
        return -1;
    } else {
        item->freeCount += 1;
#if !MLA_MONITOR_INFO
        // 分配与释放次数一致的节点会从内存泄漏检查表中移除
        if (item->freeCount == item->mallocCount) {
#if MLA_DEBUG
            mla_slist_foreach(&item->freeInfo, PrintListInfo, NULL);
#endif
            MlaDelItem(&recorderList, item);
            return 0;
        }
#endif
#if CFG_MLA_VERBOSE
        char buf[80] = {0};
#if CFG_MLA_FUNCTION
        snprintf(buf, sizeof(buf) - 1, "%s:%u %s", file, line, func);
#else
        snprintf(buf, sizeof(buf) - 1, "%s:%u", file, line);
#endif
        uint32_t hash = BKDRHash(buf);
        MlaFreeInfo_t *freeInfo = MlaFindFreeItem(&item->freeInfo, hash);
        if (freeInfo != NULL) {
            freeInfo->freeCount += 1;
        } else {
            freeInfo = (Mla_t *)MLA_MALLOC(sizeof(MlaFreeInfo_t));
            if (freeInfo == NULL) {
                LOGE("%s - %s : %u. malloc fail!", __FILENAME__, __func__, __LINE__);
                return -3;
            }
            freeInfo->hash = hash;
            freeInfo->line = line;
            freeInfo->freeCount = 1;
            memcpy(freeInfo->file, file, sizeof(freeInfo->file));
#if CFG_MLA_FUNCTION
            memcpy(freeInfo->func, func, sizeof(freeInfo->func));
#endif
            MlaAddFreeItem(&item->freeInfo, freeInfo);
        }
#endif
    }

    return 0;
}

/* 申请内存时额外多申请MEM_ID_SIZE，用以存放hash字段(file:line)，在free时检查释放的是谁申请的，可以统计申请释放次数 */
void *MlaMalloc(uint32_t size, char *file, char *func, uint16_t line)
{
    CHECK(file != NULL, NULL);
#if CFG_MLA_FUNCTION
    CHECK(func != NULL, NULL);
#else
    UNUSED(func);
#endif
    LOGD("%s - %s. Malloc caller %s:%u %s", __FILENAME__, __func__, file, line, func);
    void *ptr = MLA_MALLOC(size + MEM_ID_SIZE);
    if (ptr == NULL) {
        LOGE("%s - %s : %u. malloc fail!", __FILENAME__, __func__, __LINE__);
        return NULL;
    } else {
        char buf[BUFFER_SIZE] = {0};
#if CFG_MLA_VERBOSE
#if CFG_MLA_FUNCTION
        snprintf(buf, sizeof(buf) - 1, "%s:%u %s-%u", file, line, func, size);
#else
        snprintf(buf, sizeof(buf) - 1, "%s:%u-%u", file, line, size);
#endif
#else
#if CFG_MLA_FUNCTION
        snprintf(buf, sizeof(buf) - 1, "%s:%u %s", file, line, func);
#else
        snprintf(buf, sizeof(buf) - 1, "%s:%u", file, line);
#endif
#endif
        uint32_t hash = BKDRHash(buf);
        *((uint32_t *)ptr) = hash;
        MlaMallocRecorder(file, func, line, size, hash);
        return ptr + MEM_ID_SIZE;
    }
}

void MlaFree(void *addr, char *file, char *func, uint16_t line)
{
    ASSERT(addr != NULL);
    CHECK(file != NULL);
#if CFG_MLA_FUNCTION
    CHECK(func != NULL, NULL);
#else
    UNUSED(func);
#endif
    LOGD("%s - %s. Free caller %s:%u %s", __FILENAME__, __func__, file, line, func);
    uint32_t hash = *((uint32_t *)(addr - MEM_ID_SIZE));
    if ((addr - MEM_ID_SIZE) == NULL) {
        LOGE("%s - %s : %u. malloc fail!:", __FILENAME__, __func__, __LINE__);
        return;
    } else {
        MLA_FREE(addr - MEM_ID_SIZE);
    }
    MlaFreeRecorder(file, func, line, hash);
}

#if CFG_MLA_VERBOSE
static int MlaCollectVerboseInfo(void **p_arg, mla_list_node_t **p_node)
{
    CHECK(*p_node != NULL, -1);
    CHECK(*p_arg != NULL, -1);
    VerbosePrintInfo_t *printInfo = (VerbosePrintInfo_t *)(*p_arg);
    MlaFreeInfo_t *recorder = (MlaFreeInfo_t *)(*p_node);
    char bufMalloc[32] = {0};
    if (printInfo->verboseIndex == 1) {
        MLA_OUTPUT("%s", SplitLine);
        MLA_OUTPUT("|""%-16s%-32s%-*s""|", "Verbose:", "malloc", BUFFER_SIZE, "free");
        snprintf(bufMalloc, sizeof(bufMalloc) - 1, "(%u)B - [%u]", printInfo->mla.size, printInfo->mla.mallocCount);
    }
    char bufFree[BUFFER_SIZE] = {0};
#if CFG_MLA_FUNCTION
    snprintf(bufFree, sizeof(bufFree) - 1, "%s:%u %s - [%u]", recorder->file, recorder->line, recorder->func, recorder->freeCount);
#else
    snprintf(bufFree, sizeof(bufFree) - 1, "%s:%u - [%u]", recorder->file, recorder->line, recorder->freeCount);
#endif
    MLA_OUTPUT("|""%3u.%-12s%-32s%-*s""|", printInfo->verboseIndex, "", bufMalloc, BUFFER_SIZE, bufFree);
    printInfo->verboseIndex++;

    return 0;
}
#endif

static int MlaCollectInfo(void **p_arg, mla_list_node_t **p_node)
{
    CHECK(*p_node != NULL, -1);

    char buf[BUFFER_SIZE] = {0};
    Mla_t *recorder = (Mla_t *)(*p_node);
#if CFG_MLA_FUNCTION
    snprintf(buf, sizeof(buf) - 1 , "%s:%u %s", recorder->file, recorder->line, recorder->func);
#else
    snprintf(buf, sizeof(buf) - 1, "%s: %u", recorder->file, recorder->line);
#endif
    if (*p_arg != NULL && *((bool *)(*p_arg))) {
        MLA_OUTPUT(" ""%-*s%-16x%-16u%-16u%d", BUFFER_SIZE - 10, buf, recorder->hash, recorder->mallocCount, recorder->freeCount,
            recorder->mallocCount - recorder->freeCount);
        return 0;
    }
#if CFG_MLA_VERBOSE
    MLA_OUTPUT(">%u", ++mlaIndex);
    MLA_OUTPUT(" ""%-*s%-16s%-16s%-16s%s", BUFFER_SIZE - 10, "Caller", "Size", "Malloc", "Free", "Diff");
    MLA_OUTPUT(" ""%-*s%-16u%-16u%-16u%d", BUFFER_SIZE - 10, buf, recorder->size, recorder->mallocCount, recorder->freeCount,
        recorder->mallocCount - recorder->freeCount);
    VerbosePrintInfo_t printInfo;
    printInfo.verboseIndex = 1;
    memcpy(&printInfo.mla, recorder, sizeof(Mla_t));
    mla_slist_foreach(&recorder->freeInfo, MlaCollectVerboseInfo, &printInfo);
    MLA_OUTPUT("%s", SplitLine);
#endif

    return 0;
}

int MlaOutput(void)
{
#if CFG_MLA_FUNCTION && CFG_MLA_VERBOSE
    uint8_t alignWidth = 128;
#elif CFG_MLA_FUNCTION
    uint8_t alignWidth = 112;
#else
    uint8_t alignWidth = 96;
#endif
    MLA_OUTPUT("*""%-*s""*", alignWidth, "");
    MLA_OUTPUT("%s", MlaTitle);
    MLA_OUTPUT("*""%-*s""*", alignWidth, "");
    if (mla_list_node_count_get(&recorderList) == 0) {
        char *mlaNone = "M L A  N O N E";
        uint8_t mlaNoneWidth = (alignWidth - strlen(mlaNone)) / 2;
        MLA_OUTPUT("*""%-*s%s%-*s""*", mlaNoneWidth, "", mlaNone, mlaNoneWidth, "");
    } else {
        bool overview = true;
        MLA_OUTPUT(" ""%-*s%-16s%-16s%-16s%s", BUFFER_SIZE - 10, "Caller", "Hash", "Malloc", "Free", "Diff");
        mla_slist_foreach(&recorderList, MlaCollectInfo, &overview);
#if CFG_MLA_VERBOSE
        mlaIndex = 0;
        MLA_OUTPUT("\r\n%s\r\n", OVSplitLine);
        mla_slist_foreach(&recorderList, MlaCollectInfo, NULL);
#endif
    }
}

void MlaInit(void)
{
    mla_list_init(&recorderList);
}
