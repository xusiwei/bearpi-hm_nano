/*
 * Copyright (c) 2013-2020, Huawei Technologies Co., Ltd. All rights reserved.
 * Copyright (c) 2020, Huawei Device Co., Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 *    conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 *    of conditions and the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 *    to endorse or promote products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdarg.h>
#include "los_interrupt.h"
#include "los_task.h"
#include "los_debug.h"
#include "riscv_hal.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

static ExcInfoArray g_excArray[OS_EXC_TYPE_MAX];

LosExcInfo g_excInfo;
#define RISCV_EXC_TYPE_NUM 16
const CHAR g_excInformation[RISCV_EXC_TYPE_NUM][50] = {
    { "Instruction address misaligned!" },
    { "Instruction access fault!" },
    { "Illegal instruction" },
    { "Breakpoint!" },
    { "Load address misaligned!" },
    { "Load access fault!" },
    { "Store/AMO address misaligned!" },
    { "Store/AMO access fault!" },
    { "Environment call form U-mode!" },
    { "Environment call form S-mode!" },
    { "Reserved!" },
    { "Environment call form M-mode!" },
    { "Instruction page fault!" },
    { "Load page fault!" },
    { "Reserved!" },
    { "Store/AMO page fault!" },
};

#define RA_OFFSET         4
#define FP_OFFSET         8
#define OS_MAX_BACKTRACE  15
#define FP_ALIGN(value)   (((UINT32)(value) & (UINT32)(LOSCFG_STACK_POINT_ALIGN_SIZE - 1)) == 0)
#define FP_CHECK(value)   (OsBackTraceFpCheck(value) && ((UINT32)(value) != FP_INIT_VALUE) && FP_ALIGN(value))

LITE_OS_SEC_BSS UINT32  g_intCount = 0;
LITE_OS_SEC_BSS UINT32 g_hwiFormCnt[OS_HWI_MAX_NUM];
LITE_OS_SEC_DATA_INIT HWI_HANDLE_FORM_S g_hwiForm[OS_HWI_MAX_NUM] = {
    { .pfnHook = NULL, .uwParam = 0 }, // 0 User software interrupt handler
    { .pfnHook = NULL, .uwParam = 0 }, // 1 Supervisor software interrupt handler
    { .pfnHook = NULL, .uwParam = 0 }, // 2 Reserved
    { .pfnHook = OsHwiDefaultHandler, .uwParam = 0 }, // 3 Machine software interrupt handler
    { .pfnHook = NULL, .uwParam = 0 }, // 4 User timer interrupt handler
    { .pfnHook = NULL, .uwParam = 0 }, // 5 Supervisor timer interrupt handler
    { .pfnHook = NULL, .uwParam = 0 }, // 6  Reserved
    { .pfnHook = OsHwiDefaultHandler, .uwParam = 0 }, // 7 Machine timer interrupt handler
    { .pfnHook = NULL, .uwParam = 0 }, // 8  User external interrupt handler
    { .pfnHook = NULL, .uwParam = 0 }, // 9 Supervisor external interrupt handler
    { .pfnHook = NULL, .uwParam = 0 }, // 10 Reserved
    { .pfnHook = OsHwiDefaultHandler, .uwParam = 0 }, // 11 Machine external interrupt handler
    { .pfnHook = OsHwiDefaultHandler, .uwParam = 0 }, // 12 NMI handler
    { .pfnHook = NULL, .uwParam = 0 }, // 13 Reserved
    { .pfnHook = NULL, .uwParam = 0 }, // 14 Reserved
    { .pfnHook = NULL, .uwParam = 0 }, // 15 Reserved
    { .pfnHook = NULL, .uwParam = 0 }, // 16 Reserved
    { .pfnHook = NULL, .uwParam = 0 }, // 17 Reserved
    { .pfnHook = NULL, .uwParam = 0 }, // 18 Reserved
    { .pfnHook = NULL, .uwParam = 0 }, // 19 Reserved
    { .pfnHook = NULL, .uwParam = 0 }, // 20 Reserved
    { .pfnHook = NULL, .uwParam = 0 }, // 21 Reserved
    { .pfnHook = NULL, .uwParam = 0 }, // 22 Reserved
    { .pfnHook = NULL, .uwParam = 0 }, // 23 Reserved
    { .pfnHook = NULL, .uwParam = 0 }, // 24 Reserved
    { .pfnHook = NULL, .uwParam = 0 }, // 25 Reserved
};

LITE_OS_SEC_TEXT_INIT VOID OsHwiDefaultHandler(UINTPTR arg)
{
    (VOID)arg;
    PRINT_ERR("default handler\n");
    while (1) {
    }
}

LITE_OS_SEC_TEXT_INIT VOID OsHwiInit(VOID)
{
    UINT32 index;
    for (index = OS_RISCV_SYS_VECTOR_CNT; index < OS_HWI_MAX_NUM; index++) {
        g_hwiForm[index].pfnHook = OsHwiDefaultHandler;
        g_hwiForm[index].uwParam = 0;
    }
}

typedef VOID (*HwiProcFunc)(UINTPTR);
LITE_OS_SEC_TEXT_INIT VOID OsHwiInterruptDone(HWI_HANDLE_T hwiNum)
{
    g_intCount++;

    HWI_HANDLE_FORM_S *hwiForm = &g_hwiForm[hwiNum];
    HwiProcFunc func = (HwiProcFunc)(hwiForm->pfnHook);
    func(hwiForm->uwParam);

    ++g_hwiFormCnt[hwiNum];

    g_intCount--;
}

LITE_OS_SEC_TEXT UINT32 OsGetHwiFormCnt(HWI_HANDLE_T hwiNum)
{
    if (hwiNum < OS_HWI_MAX_NUM) {
        return g_hwiFormCnt[hwiNum];
    }

    return LOS_NOK;
}

LITE_OS_SEC_TEXT HWI_HANDLE_FORM_S *OsGetHwiForm(VOID)
{
    return g_hwiForm;
}

/*****************************************************************************
 Function    : LOS_HwiCreate
 Description : create hardware interrupt
 Input       : hwiNum     --- hwi num to create
               hwiPrio    --- priority of the hwi
               hwiMode    --- hwi interrupt mode
               hwiHandler --- hwi handler
               irqParam   --- param of the hwi handler
 Output      : None
 Return      : LOS_OK on success or error code on failure
 *****************************************************************************/
LITE_OS_SEC_TEXT UINT32 LOS_HwiCreate(HWI_HANDLE_T hwiNum,
                                      HWI_PRIOR_T hwiPrio,
                                      HWI_MODE_T hwiMode,
                                      HWI_PROC_FUNC hwiHandler,
                                      HWI_IRQ_PARAM_S irqParam)
{
    UINT32 intSave;

    if (hwiHandler == NULL) {
        return OS_ERRNO_HWI_PROC_FUNC_NULL;
    }
    if (hwiNum >= OS_HWI_MAX_NUM) {
        return OS_ERRNO_HWI_NUM_INVALID;
    }
    if (g_hwiForm[hwiNum].pfnHook == NULL) {
        return OS_ERRNO_HWI_NUM_INVALID;
    } else if (g_hwiForm[hwiNum].pfnHook != OsHwiDefaultHandler) {
        return OS_ERRNO_HWI_NUM_INVALID;
    }
    if ((hwiPrio < OS_HWI_PRIO_LOWEST) || (hwiPrio > OS_HWI_PRIO_HIGHEST)) {
        return OS_ERRNO_HWI_PRIO_INVALID;
    }

    intSave = LOS_IntLock();
    g_hwiForm[hwiNum].pfnHook = hwiHandler;
    g_hwiForm[hwiNum].uwParam = irqParam;

    if (hwiNum >= OS_RISCV_SYS_VECTOR_CNT) {
        OsSetLocalInterPri(hwiNum, hwiPrio);
    }

    LOS_IntRestore(intSave);

    return LOS_OK;
}

/*****************************************************************************
 Function    : LOS_HwiDelete
 Description : Delete hardware interrupt
 Input       : hwiNum   --- hwi num to delete
 Return      : LOS_OK on success or error code on failure
 *****************************************************************************/
LITE_OS_SEC_TEXT UINT32 LOS_HwiDelete(HWI_HANDLE_T hwiNum, HWI_IRQ_PARAM_S irqParam)
{
    UINT32 intSave;

    if (hwiNum >= OS_HWI_MAX_NUM) {
        return OS_ERRNO_HWI_NUM_INVALID;
    }

    intSave = LOS_IntLock();
    g_hwiForm[hwiNum].pfnHook = OsHwiDefaultHandler;
    g_hwiForm[hwiNum].uwParam = 0;
    LOS_IntRestore(intSave);
    return LOS_OK;
}

LITE_OS_SEC_TEXT VOID BackTraceSub(UINT32 fp)
{
    UINT32 backFp = fp;
    UINT32 tmpFp;
    UINT32 backRa;
    UINT32 count = 0;

    while (FP_CHECK(backFp)) {
        tmpFp = backFp;
        backRa = *((UINT32 *)(UINTPTR)(tmpFp - RA_OFFSET));
        backFp = *((UINT32 *)(UINTPTR)(tmpFp - FP_OFFSET));
        PRINTK("traceback %u -- ra = 0x%x    fp = 0x%x\n", count, backRa, backFp);

        count++;

        if ((count == OS_MAX_BACKTRACE) || (backFp == tmpFp) || \
            (!OsBackTraceRaCheck(backRa))) {
            break;
        }
    }
    PRINTK("*******backtrace end*******\n");
}

LITE_OS_SEC_TEXT VOID BackTrace(UINT32 fp)
{
    PRINTK("*******backtrace begin*******\n");

    BackTraceSub(fp);
}

LITE_OS_SEC_TEXT static VOID OsExcBackTrace(UINT32 fp, UINT32 ra)
{
    UINT32 backFp;
    if (FP_CHECK(fp)) {
        backFp = *((UINT32 *)(UINTPTR)(fp - RA_OFFSET));
        if ((backFp != ra) && FP_CHECK(backFp)) {
            fp = backFp;
        }
        BackTrace(fp);
    } else {
        PRINTK("fp error, back trace failed!\n");
    }
}

LITE_OS_SEC_TEXT VOID OsTaskBackTrace(UINT32 taskID)
{
    LosTaskCB *taskCB = NULL;

    if (taskID >= g_taskMaxNum) {
        PRINT_ERR("\r\nTask PID is invalid!\n");
        return;
    }

    taskCB = OS_TCB_FROM_TID(taskID);
    if ((taskCB->taskStatus & OS_TASK_STATUS_UNUSED) || (taskCB->taskEntry == NULL) ||
        (taskCB->taskName == NULL)) {
        PRINT_ERR("\r\nThe task is not created!\n");
        return;
    }

    if (taskCB->taskStatus & OS_TASK_STATUS_RUNNING) {
        OsBackTrace();
        return;
    }

    PRINTK("taskName = %s\n", taskCB->taskName);
    PRINTK("taskID   = 0x%x\n", taskCB->taskID);
    PRINTK("curr ra  = 0x%08x\n", ((TaskContext *)(taskCB->stackPointer))->ra);
    OsExcBackTrace(((TaskContext  *)(taskCB->stackPointer))->s0, ((TaskContext *)(taskCB->stackPointer))->ra);
}

LITE_OS_SEC_TEXT VOID OsBackTrace(VOID)
{
    UINT32 fp = GetFp();
    PRINTK("taskName = %s\n", g_losTask.runTask->taskName);
    PRINTK("taskID   = %u\n", g_losTask.runTask->taskID);
    PRINTK("curr fp  = 0x%08x \n", fp);
    BackTrace(fp);
}

LITE_OS_SEC_TEXT VOID LOS_Panic(const CHAR *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    PRINTK(fmt, ap);
    va_end(ap);
    OsDisableIRQ();
    while (1) {
    }
}

STATIC VOID OsDisplayTaskInfo(VOID)
{
    TSK_INFO_S taskInfo;
    UINT32 index;
    UINT32 ret;

    PRINTK("ID  Pri    Status     name \n\r");
    PRINTK("--  ---    ---------  ----\n\r");

    for (index = 0; index < LOSCFG_BASE_CORE_TSK_LIMIT; index++) {
        ret = LOS_TaskInfoGet(index, &taskInfo);
        if (ret != LOS_OK) {
            continue;
        }
        PRINTK("%d    %d     %s      %s \n\r",
               taskInfo.uwTaskID, taskInfo.usTaskPrio, OsConvertTskStatus(taskInfo.usTaskStatus), taskInfo.acName);
    }
    return;
}

LITE_OS_SEC_TEXT STATIC VOID OsExcInfoDisplayContext(const LosExcInfo *exc)
{
    const TaskContext *taskContext = &(exc->context->taskContext);

    PRINTK("mepc       = 0x%x\n", taskContext->mepc);
    PRINTK("mstatus    = 0x%x\n", taskContext->mstatus);
    PRINTK("mtval      = 0x%x\n", exc->context->mtval);
    PRINTK("mcause     = 0x%x\n", exc->context->mcause);
    PRINTK("ra         = 0x%x\n", taskContext->ra);
    PRINTK("sp         = 0x%x\n", taskContext->sp);
    PRINTK("gp         = 0x%x\n", exc->context->gp);
    PRINTK("tp         = 0x%x\n", taskContext->tp);
    PRINTK("t0         = 0x%x\n", taskContext->t0);
    PRINTK("t1         = 0x%x\n", taskContext->t1);
    PRINTK("t2         = 0x%x\n", taskContext->t2);
    PRINTK("s0         = 0x%x\n", taskContext->s0);
    PRINTK("s1         = 0x%x\n", taskContext->s1);
    PRINTK("a0         = 0x%x\n", taskContext->a0);
    PRINTK("a1         = 0x%x\n", taskContext->a1);
    PRINTK("a2         = 0x%x\n", taskContext->a2);
    PRINTK("a3         = 0x%x\n", taskContext->a3);
    PRINTK("a4         = 0x%x\n", taskContext->a4);
    PRINTK("a5         = 0x%x\n", taskContext->a5);
    PRINTK("a6         = 0x%x\n", taskContext->a6);
    PRINTK("a7         = 0x%x\n", taskContext->a7);
    PRINTK("s2         = 0x%x\n", taskContext->s2);
    PRINTK("s3         = 0x%x\n", taskContext->s3);
    PRINTK("s4         = 0x%x\n", taskContext->s4);
    PRINTK("s5         = 0x%x\n", taskContext->s5);
    PRINTK("s6         = 0x%x\n", taskContext->s6);
    PRINTK("s7         = 0x%x\n", taskContext->s7);
    PRINTK("s8         = 0x%x\n", taskContext->s8);
    PRINTK("s9         = 0x%x\n", taskContext->s9);
    PRINTK("s10        = 0x%x\n", taskContext->s10);
    PRINTK("s11        = 0x%x\n", taskContext->s11);
    PRINTK("t3         = 0x%x\n", taskContext->t3);
    PRINTK("t4         = 0x%x\n", taskContext->t4);
    PRINTK("t5         = 0x%x\n", taskContext->t5);
    PRINTK("t6         = 0x%x\n", taskContext->t6);

    OsExcBackTrace(taskContext->s0, taskContext->ra);
}

LITE_OS_SEC_TEXT VOID OsExcInfoDisplay(const LosExcContext *excBufAddr)
{
    g_excInfo.type = excBufAddr->mcause;
    g_excInfo.context = (LosExcContext *)excBufAddr;

    PRINTK("\r\nException Information     \n\r");

    if (excBufAddr->mcause < RISCV_EXC_TYPE_NUM) {
        PRINTK("Exc  type : Oops  - %s\n\r", g_excInformation[excBufAddr->mcause]);
    } else {
        PRINTK("Exc  type : Oops  - Invalid\n\r");
    }

    PRINTK("taskName = %s\n\r", g_losTask.runTask->taskName);
    PRINTK("taskID = %u\n\r", g_losTask.runTask->taskID);
    PRINTK("system mem addr:0x%x\n\r", (UINTPTR)OS_SYS_MEM_ADDR);
    OsExcInfoDisplayContext(&g_excInfo);
}

VOID OsExcEntry(const LosExcContext *excBufAddr)
{
    if (g_excInfo.nestCnt > 2) {
        PRINTK("hard faule!\n\r");
        goto SYSTEM_DEATH;
    }

    OsExcInfoDisplay(excBufAddr);

    PRINTK("----------------All Task infomation ------------\n\r");
    OsDisplayTaskInfo();

SYSTEM_DEATH:
    while (1) {
    }
}

VOID OsExcRegister(ExcInfoType type, EXC_INFO_SAVE_CALLBACK func, VOID *arg)
{
    ExcInfoArray *excInfo = NULL;
    if ((type >= OS_EXC_TYPE_MAX) || (func == NULL)) {
        PRINT_ERR("OsExcRegister ERROR!\n");
        return;
    }
    excInfo = &(g_excArray[type]);
    excInfo->uwType = type;
    excInfo->pFnExcInfoCb = func;
    excInfo->pArg = arg;
    excInfo->uwValid = TRUE;
}

/* stack protector */
UINT32 __stack_chk_guard = 0xd00a0dff;

VOID __stack_chk_fail(VOID)
{
    /* __builtin_return_address is a builtin function, building in gcc */
    LOS_Panic("stack-protector: Kernel stack is corrupted in: %p\n",
              __builtin_return_address(0));
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */
