介绍
====
MLA 即 Memory Leak Analyzer，是一个排查内存泄漏的分析器<br />
实现机制是在malloc时记录分配位置信息，在free时记录释放位置信息，通过两者计数作差可得是否存在泄漏<br />

快速开始
-------
你可以使用提供的脚本`do.sh`来快速使用本代码库<br />
>可以使用`./do.sh help`命令<br />
```bash
-*- help -*-
usage: ./do.sh [generate] [make] [exec] [clean] [help]
    [generate]: -g -G generate

Example usage of the MLA mechanism
$ ./do.sh -g MLA
$ ./do.sh make

Self-validation of MLA mechanisms
$ ./do.sh -g SV
$ ./do.sh make

LOG mechanism implementation analysis
$ ./do.sh -g LOG
$ ./do.sh make

Execute the program to view the results
$ ./do.sh exec

Remove unnecessary code
$ ./do.sh clean
```

如何使用
-------
你只需两步就可以开始使用了<br />
>1、适配`mla.h`文件中的两个接口malloc和free
```c
/* MLA内部使用的内存管理接口 */
#define MLA_MALLOC(size)    malloc(size)
#define MLA_FREE(addr)      free(addr)

/* 对外提供使用的内存泄漏检查的分配释放接口 */
#define PORT_MALLOC(size)    MlaMalloc(size, __FILENAME__, __func__, __LINE__)
#define PORT_FREE(addr)      MlaFree(addr, __FILENAME__, __func__, __LINE__)
```
>2、在你的代码初始化部分加入接口`MlaInit`，在查看内存泄漏信息的地方调用接口`MlaOutput`即可

### 示例：
通过自证清白来演示MLA的用法
```bash
$ ./do.sh -g SV
Generate a example version of the MLA file.
```
执行上述命令后会生成一些文件，这些是`MLA`自证的测试文件
```bash
$ ./do.sh make
$ ./do.sh exec
```
执行上述命令后会输出`MLA`的分析信息，借助`Diff`字段可以清晰看出有没有内存泄漏<br />
在`MLA Verbose`部分可以看到详细的内存分配和释放信息，包括代码文件名、行数、函数以及分配大小、释放次数等信息
```txt
-- SV_MlaOutput:
*                                                                                                                                *
****************************************************** Memory Leak Analyzer ******************************************************
*                                                                                                                                *
*                                                         M L A  N O N E                                                         *

-- MlaOutput:
*                                                                                                                                *
****************************************************** Memory Leak Analyzer ******************************************************
*                                                                                                                                *
 Caller                                                                Hash            Malloc          Free            Diff
 sv_mla.c:316 SV_MlaMalloc                                             52f06d09        3               3               0
 sv_mla.c:214 MlaMallocRecorder                                        1239e656        1               1               0
 sv_mla.c:286 MlaFreeRecorder                                          49583dd0        2               2               0

*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%  MLA  Verbose  %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*

>1
 Caller                                                                Size            Malloc          Free            Diff
 sv_mla.c:316 SV_MlaMalloc                                             12              3               3               0
*--------------------------------------------------------------------------------------------------------------------------------*
|Verbose:        malloc                          free                                                                            |
|  1.            (12)B - [3]                     sv_mla.c:357 SV_MlaFree - [3]                                                   |
*--------------------------------------------------------------------------------------------------------------------------------*
>2
 Caller                                                                Size            Malloc          Free            Diff
 sv_mla.c:214 MlaMallocRecorder                                        104             1               1               0
*--------------------------------------------------------------------------------------------------------------------------------*
|Verbose:        malloc                          free                                                                            |
|  1.            (104)B - [1]                    sv_mla.c:201 MlaDelItem - [1]                                                   |
*--------------------------------------------------------------------------------------------------------------------------------*
>3
 Caller                                                                Size            Malloc          Free            Diff
 sv_mla.c:286 MlaFreeRecorder                                          88              2               2               0
*--------------------------------------------------------------------------------------------------------------------------------*
|Verbose:        malloc                          free                                                                            |
|  1.            (88)B - [2]                     sv_mla.c:153 MlaProcessFreeNode - [2]                                           |
*--------------------------------------------------------------------------------------------------------------------------------*
```
共同进步
-------
欢迎大家使用并`issue`反馈
