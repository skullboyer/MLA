#!/usr/bin/bash
# Copyright © 2023 <copyright skull.gu@gmail.com>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software
# and associated documentation files (the “Software”), to deal in the Software without
# restriction, including without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all copies or
# substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
# BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
# DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

function help {
cat <<EOF
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
EOF
}

function modify_mla_c {
    sed -i 's/#include "mla.h"/#include "sv_mla.h"/' $1
    sed -i 's/#define TAG    "MLA"/#define TAG    "SV_MLA"/' $1
    sed -i 's/#define MLA_MONITOR_INFO    1/#define MLA_MONITOR_INFO    0/' $1
    sed -i 's/void \*MlaMalloc/void \*SV_MlaMalloc/' $1
    sed -i 's/void MlaFree/void SV_MlaFree/' $1
    sed -i 's/int MlaOutput/int SV_MlaOutput/' $1
    sed -i 's/void MlaInit/void SV_MlaInit/' $1
}

function modify_mla_h {
    sed -i 's/#include "adapter.h"/#include "mla.h"/' $1
    sed -i 's/malloc(size)/MlaMalloc(size, __FILENAME__, __func__, __LINE__)/' $1
    sed -i 's/free(addr)/MlaFree(addr, __FILENAME__, __func__, __LINE__)/' $1
    sed -i 's/PORT_MALLOC(size)    MlaMalloc/SV_PORT_MALLOC(size)    SV_MlaMalloc/' $1
    sed -i 's/PORT_FREE(addr)      MlaFree/SV_PORT_FREE(addr)      SV_MlaFree/' $1
    sed -i 's/void \*MlaMalloc/void \*SV_MlaMalloc/' $1
    sed -i 's/void MlaFree/void SV_MlaFree/' $1
    sed -i 's/int MlaOutput/int SV_MlaOutput/' $1
    sed -i 's/void MlaInit/void SV_MlaInit/' $1
}

function generate_selfverify {
cat >self_verify.c <<EOF
#include "sv_mla.h"

#define TAG    "SV"
#define CHECK_NUM    3

int main()
{
    void *pMla[CHECK_NUM] = {NULL};
    MlaInit();
    SV_MlaInit();
    LOGD("%s - %s : %u", __FILENAME__, __func__, __LINE__);
    for (uint8_t i = 0; i < CHECK_NUM; i++) {
        LOGW("+++Malloc address: 0x%08x", pMla[i] = SV_PORT_MALLOC(8));  // *(i+1)
    }
    LOGD("------------free-1");
    SV_PORT_FREE(pMla[0]);
    LOGD("------------free-2");
    SV_PORT_FREE(pMla[1]);
    LOGD("------------free-3");
    SV_PORT_FREE(pMla[2]);
    LOGV("\r\n-- SV_MlaOutput:\r\n");
    SV_MlaOutput();
    LOGV("\r\n-- MlaOutput:\r\n");
    MlaOutput();

    return 0;
}
EOF
}

function generate_sv {
    cp ./mla.c sv_mla.c
    modify_mla_c sv_mla.c
    cp ./mla.h sv_mla.h
    modify_mla_h sv_mla.h
    generate_selfverify
}

function generate_mla {
cat >demo.c <<EOF
#include "mla.h"

#define TAG    "DEMO"

int main()
{
    void *pMla = NULL;
    uint8_t i = 1;
    uint16_t cnt = 0;
    MlaInit();
    LOGD("%s - %s : %u", __FILENAME__, __func__, __LINE__);
    LOGI("%s : %u. Malloc address: 0x%08x", __func__, __LINE__, pMla = PORT_MALLOC(8));
    for (; i < 16; i++) {
        void *ptr = PORT_MALLOC((i%5 + 1)*16);
        LOGI("%s : %u. Malloc address: 0x%08x", __func__, __LINE__, ptr);
        if (!(i % 3)) {
            PORT_FREE(ptr);
            cnt++;
        }
    }
    cnt = i - 1 - cnt;
    LOGD("[MLC] %s : %u. Memory leakage count: %u", __func__, __LINE__, cnt);
    cnt = 0;
    for (i = 1; i < 8; i++) {
        void *ptr = PORT_MALLOC(i*32);
        LOGI("%s : %u. Malloc address: 0x%08x", __func__, __LINE__, ptr);
        PORT_FREE(ptr);
    }
    for (i = 1; i < 160; i++) {
        void *ptr = PORT_MALLOC((i%16 + 1)*32);
        LOGI("%s : %u. Malloc address: 0x%08x", __func__, __LINE__, ptr);
        if (!(i % 3)) {
            PORT_FREE(ptr);
            cnt++;
        } else if (!(i % 7)) {
            PORT_FREE(ptr);
            cnt++;
        }
    }
    cnt = i - 1 - cnt;
    LOGD("[MLC] %s : %u. Memory leakage count: %u", __func__, __LINE__, cnt);
    cnt = 0;

    MlaOutput();

   return 0;
}
EOF
}

function generate_log {
cat >log.c <<EOF
#include <stdio.h>
#include <stdarg.h>
#include "adapter.h"

#define TAG    "LOG"
#define OUTPUT    output

FILE *logFile = NULL;

int initLogFile()
{
    logFile = fopen("Log.log", "w+");
    if (logFile == NULL) {
        printf("Failed to open log file.\n");
        return -1;
    }
    return 0;
}

int output(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    if (logFile != NULL) {
        vfprintf(logFile, format, args);
        fflush(logFile);
    }

    va_end(args);
    return 0;
}

int test1()
{
    LOGW("--- %s", __func__);
    return 369;
}

int test()
{
    LOGI("+++ %s, %d", __func__, test1());
    return 666;
}

int main()
{
    initLogFile();
    LOGD("=== %s, %d", __func__, test());
    fclose(logFile);
    return 0;
}
EOF
}

function clean {
    [ -f a.out ] && rm a.out
    [ -f build.log ] && rm build.log
    [ -f exec.log ] && rm exec.log
    [ -f Log.log ] && rm Log.log
    [ -f sv_mla.c ] && rm sv_mla.c
    [ -f sv_mla.h ] && rm sv_mla.h
    [ -f self_verify.c ] && rm self_verify.c
    [ -f demo.c ] && rm demo.c
    [ -f log.c ] && rm log.c
    sed -i '/char buffer\[256\];/d' adapter.h
    sed -i '/sprintf(buffer, __VA_ARGS__);/d' adapter.h
    sed -i '/OUTPUT(\"%s, >>>, %s\\n", \#level, \#__VA_ARGS__);/d' adapter.h
    sed -i '/OUTPUT(\"###\\n\");/d' adapter.h
    sed -i '/OUTPUT(\"@ %s\\n\", buffer);/d' adapter.h
    sed -i '/OUTPUT(\"%s, <<<, %s\\n", \#level, \#__VA_ARGS__);/d' adapter.h
    sed -i "s/OUTPUT(buffer);/OUTPUT(__VA_ARGS__);/" adapter.h
}

function generate {
    [ ! $1 ] && echo -e "\e[47m\e[31m!!Please enter option parameters\e[0m" && exit -1
    [ $1 = 'SV' ] && {
        clean
        generate_sv
        echo "Generate a self-validating version of the MLA file."
    } || {
        [ $1 = 'MLA' ] && {
            clean
            generate_mla
            echo "Generate a example version of the MLA file."
        } || {
            [ $1 = 'LOG' ] && {
                clean
                generate_log
                [[ $2 = '--debug' ]] && {
                    awk 'index($0, "level == V ? : OUTPUT(#level") {print "OUTPUT(\"%s, >>>, %s\\n\", #level, #__VA_ARGS__); \\"}1' adapter.h > adapter_tmp.h && mv adapter_tmp.h adapter.h
                    awk 'index($0, "level == V ? : OUTPUT(#level") {print "OUTPUT(\"###\\n\"); \\"}1' adapter.h > adapter_tmp.h && mv adapter_tmp.h adapter.h
                }
                awk 'index($0, "level == V ? : OUTPUT(#level") {print "char buffer[256]; \\"}1' adapter.h > adapter_tmp.h && mv adapter_tmp.h adapter.h
                awk 'index($0, "level == V ? : OUTPUT(#level") {print "sprintf(buffer, __VA_ARGS__); \\"}1' adapter.h > adapter_tmp.h && mv adapter_tmp.h adapter.h
                [[ $2 = '--debug' ]] && {
                    awk 'index($0, "level == V ? : OUTPUT(#level") {print "OUTPUT(\"@ %s\\n\", buffer); \\"}1' adapter.h > adapter_tmp.h && mv adapter_tmp.h adapter.h
                    awk 'index($0, "level == V ? : OUTPUT(#level") {print "OUTPUT(\"%s, <<<, %s\\n\", #level, #__VA_ARGS__); \\"}1' adapter.h > adapter_tmp.h && mv adapter_tmp.h adapter.h
                }
                space_num=`awk 'index($0, "level == V ? : OUTPUT(#level") {match($0, /^ */); print RLENGTH}' adapter.h`
                spaces=$(printf '%*s' $space_num ' ')
                sed -i "s/char buffer\[256\];/${spaces}&/" adapter.h
                sed -i "s/sprintf(buffer, __VA_ARGS__);/${spaces}&/" adapter.h
                sed -i "s/OUTPUT(\"%s, >>>/${spaces}&/" adapter.h
                sed -i "s/OUTPUT(\"###/${spaces}&/" adapter.h
                sed -i "s/OUTPUT(\"@ %s/${spaces}&/" adapter.h
                sed -i "s/OUTPUT(\"%s, <<</${spaces}&/" adapter.h
                sed -i "s/OUTPUT(__VA_ARGS__);/OUTPUT(buffer);/" adapter.h
                echo "Generate a file for analyzing the logging mechanism."
            } || echo "!!Please check input" && exit -1
        }
    }
}

function process {
    case ${user_arg[0]} in
        -g|-G|generate)
            generate ${user_arg[1]} ${user_arg[2]}
            ;;
        make)
            [[ ! -f self_verify.c && ! -f demo.c && ! -f log.c ]] && echo "!!Run the command './do.sh generate'" && exit -1
            gcc *.c
            gcc *.c 2>&1 |grep -e error: -e warning: >build.log
            grep -q error: build.log && echo -e "\nBuild Error!" && grep -e error: build.log
            ;;
        exec)
            [ ! -f a.out ] && echo "!!Run the command './do.sh make'" && exit -1
            ./a.out |tee exec.log
            ;;
        clean)
            clean
            ;;
        *)
            help
            ;;
    esac
}

user_arg=(${@:1})
process
