/*
 * Copyright (c) 2025-2025 Huawei Technologies Co., Ltd. All rights reserved.
 *
 * UniProton is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * Create: 2025-08-04
 * Description: Task periodic execution implementation
 */
#include "prt_signal.h"
#include "prt_timer.h"
#include "prt_swtmr_external.h"
#include "signal.h"
#include "prt_task_external.h"

void OsDelieverSigPeriod(TimerHandle tmrHandle, U32 arg1, U32 arg2, U32 arg3, U32 arg4)
{
    (void)tmrHandle;
    (void)arg2;
    (void)arg3;
    (void)arg4;
    signalInfo info = {0};
    info.si_signo = SIGALRM;
    info.si_code = SI_USER;
    uintptr_t intSave = OsIntLock();
    PRT_SignalDeliver(arg1, &info);
    OsIntRestore(intSave);
}

int OsPeriodTimerCreate(TskHandle taskPid, timer_t *timerId)
{
    int ret;
    TimerHandle swtmrId;
    struct TimerCreatePara timerPara = {0};
    if (!timerId) {
        return -EINVAL;
    }

    timerPara.type = OS_TIMER_SOFTWARE;
    timerPara.mode = OS_TIMER_LOOP;
    timerPara.interval = 1;
    timerPara.timerGroupId = 0;
    timerPara.callBackFunc = OsDelieverSigPeriod;
    timerPara.arg1 = taskPid;
    ret = PRT_TimerCreate(&timerPara, &swtmrId);
    if (ret != OS_OK) {
        return -EINVAL;
    }

    *timerId = (timer_t)swtmrId;
    return OS_OK;
}

int OsPeriodTimerSetTime(timer_t timerId, U32 idate, U32 period)
{
    int ret;
    struct TagSwTmrCtrl *swtmr = NULL;
    ret = PRT_TimerStop(0, (TimerHandle)timerId);
    if (ret != OS_OK && ret != OS_ERRNO_SWTMR_UNSTART) {
        return -EINVAL;
    }

    if (idate == 0) {
        return OS_OK;
    }

    U32 now = (U32)PRT_TickGetCount();
    uintptr_t intSave = OsIntLock();
    swtmr = g_swtmrCbArray + OS_SWTMR_ID_2_INDEX((TimerHandle)timerId);
    swtmr->mode = (period ? OS_TIMER_LOOP : OS_TIMER_ONCE);
    swtmr->interval = (period ? period : idate - now);
    swtmr->idxRollNum = idate - now;
    swtmr->overrun = 0;
    OsIntRestore(intSave);

    ret = PRT_TimerStart(0, (TimerHandle)timerId);
    if (ret != OS_OK) {
        return -EINVAL;
    }

    return OS_OK;
}

int PRT_TaskSetPeriodic(TskHandle taskPid, U32 idate, U32 period)
{
    int ret;
    U32 now;
    struct TagTskCb *tskCb = NULL;
    if (!taskPid) {
        tskCb = RUNNING_TASK;
    } else {
        tskCb = GET_TCB_HANDLE(taskPid);
    }

    uintptr_t intSave = OsIntLock();
    if (!tskCb->itimer && !(tskCb->taskStatus & OS_TSK_SET_PERIODIC)) {
        ret = OsPeriodTimerCreate(taskPid, &tskCb->itimer);
        if (ret != OS_OK) {
            OsIntRestore(intSave);
            return ret;
        }
    
        tskCb->taskStatus |= OS_TSK_SET_PERIODIC;
    }
    OsIntRestore(intSave);

    now = (U32)PRT_TickGetCount();
    if (idate == TM_NOW) {
        idate = now + period;
    } else if (idate < now) {
        (void)PRT_TimerDelete(0, (TimerHandle)tskCb->itimer);
        return -ETIMEDOUT;
    }

    ret = OsPeriodTimerSetTime(tskCb->itimer, idate, period);
    if (ret != OS_OK) {
        (void)PRT_TimerDelete(0, (TimerHandle)tskCb->itimer);
    }

    return ret;
}

int PRT_TaskWaitPeriod(U32 *overruns)
{
    int ret;
    uintptr_t intSave;
    signalInfo prtInfo = {0};
    signalSet prtSet = sigMask(SIGALRM);
    struct TagTskCb *tskCb = RUNNING_TASK;
    struct TagSwTmrCtrl *swtmr = NULL;
    if (!(tskCb->taskStatus & OS_TSK_SET_PERIODIC)) {
        return -EWOULDBLOCK;
    }

    ret = PRT_SignalWait(&prtSet, &prtInfo, OS_SIGNAL_WAIT_FOREVER);
    if (ret != OS_OK) {
        return -EINTR;
    }

    if (overruns) {
        swtmr = g_swtmrCbArray + OS_SWTMR_ID_2_INDEX((TimerHandle)tskCb->itimer);
        intSave = OsIntLock();
        *overruns = (U32)swtmr->overrun;
        swtmr->overrun = 0;
        OsIntRestore(intSave);
        if (*overruns) {
            return -ETIMEDOUT;
        }
    }

    return OS_OK;
}
