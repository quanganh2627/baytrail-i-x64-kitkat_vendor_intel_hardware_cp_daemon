/*
 * hardware/Intel/cp_daemon/cpdSystemMonitor.c
 *
 * System Monitor for CPDD.
 * SystemMonitor runs as a thread. It monotors connection with modem and socket connections.
 * In the case of problem(s) it wil restart connections, reinitialize everything, so that other parts of CPDD can proced with work.
 *
 * Martin Junkar 09/18/2011
 *
 */
 
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#define LOG_TAG "CPDD_SM"

#include "cpd.h"
#include "cpdInit.h"
#include "cpdUtil.h"
#include "cpdModemReadWrite.h"
#include "cpdDebug.h"


static void cpdSystemMonitorThreadSignalHandler(int );

static void cpdSystemMonitorThreadSignalHandler(int sig)
{
	CPD_LOG(CPD_LOG_ID_TXT, "Signal(%s)=%d\n", __FUNCTION__, sig);
	LOGW("Signal(%s())=%d", __FUNCTION__, sig);
	switch (sig) {
	case SIGHUP:
	case SIGTERM:
	case SIGQUIT:
	case SIGUSR1:
		pthread_exit(0);
	}
}

static void cpdSystemMonitorModem(pCPD_CONTEXT pCpd)
{
    int result;
    if (pCpd == NULL) {
        return;
    }
    if (pCpd->modemInfo.keepOpenCtrl.keepOpen == 0) {
        return;
    }
    if (pCpd->modemInfo.modemReadThreadState != THREAD_STATE_RUNNING) {
        if (getMsecDt(pCpd->modemInfo.keepOpenCtrl.lastOpenAt) >= pCpd->modemInfo.keepOpenCtrl.keepOpenRetryInterval) {
            result = cpdModemOpen(pCpd);
        }
    }
    return;
}

static void cpdSystemMonitorRegisterForCP(pCPD_CONTEXT pCpd)
{
    int result;
	int needRegistering = CPD_OK;
	int doNotRegisterNow = CPD_NOK;
    if (pCpd == NULL) {
        return;
    }
    if (pCpd->modemInfo.keepOpenCtrl.keepOpen == 0) {
        return;
    }

	if ((pCpd->modemInfo.registeredForCPOSR == 0) ||
		 (pCpd->modemInfo.registeredForCPOSRat == 0)) {
		needRegistering = CPD_OK;
	}
	if (getMsecDt(pCpd->modemInfo.receivedCPOSRat) < CPD_MODEM_MONITOR_CPOSR_EVENT) {
		doNotRegisterNow = CPD_OK;
	}
	if (getMsecDt(pCpd->modemInfo.processingCPOSRat) < CPD_MODEM_MONITOR_CPOSR_EVENT) {
		doNotRegisterNow = CPD_OK;
	}
    if (pCpd->modemInfo.modemReadThreadState == THREAD_STATE_RUNNING) {
        if ((getMsecDt(pCpd->modemInfo.lastDataReceived) > CPD_MODEM_MONITOR_RX_INTERVAL) &&
            (getMsecDt(pCpd->modemInfo.lastDataSent) > CPD_MODEM_MONITOR_TX_INTERVAL)) {
			if ((needRegistering == CPD_OK) && (doNotRegisterNow == CPD_NOK)) {
            	cpdModemInitForCP(pCpd);
			}
			else {
				CPD_LOG(CPD_LOG_ID_TXT, "\n %u, Skip CPOSR registration, %d, %d", getMsecTime(), needRegistering, doNotRegisterNow);
			}
        }
    }
}    

static void cpdSystemMonitorGpsSocket(pCPD_CONTEXT pCpd)
{
    int result;
    if (pCpd == NULL) {
        return;
    }
    
    if (pCpd->scIndexToGps >= 0) {
        if ((pCpd->scGps.clients[pCpd->scIndexToGps].state == SOCKET_STATE_OFF) || 
            (pCpd->scGps.clients[pCpd->scIndexToGps].state == SOCKET_STATE_TERMINATED)) {
            pCpd->scGps.clients[pCpd->scIndexToGps].state = SOCKET_STATE_OFF;
            pCpd->scIndexToGps = CPD_ERROR;
        }
    }
    if (pCpd->scIndexToGps == CPD_ERROR) {
        if (getMsecDt(pCpd->scGpsKeepOpenCtrl.lastOpenAt) >= pCpd->scGpsKeepOpenCtrl.keepOpenRetryInterval) {
            pCpd->scIndexToGps = cpdSocketClientOpen(&(pCpd->scGps), SOCKET_HOST_GPS, SOCKET_PORT_GPS);
            pCpd->scGpsKeepOpenCtrl.lastOpenAt = getMsecTime();
            pCpd->scGpsKeepOpenCtrl.keepOpenRetryCount++;
        }
    }
    else {
        if (pCpd->scGps.clients[pCpd->scIndexToGps].state != SOCKET_STATE_RUNNING) {
            cpdSocketClose(&(pCpd->scGps.clients[pCpd->scIndexToGps]));
        }
    }
}


void *cpdSystemMonitorThread( void *pArg)
{
    int result = CPD_ERROR;
    pCPD_CONTEXT pCpd;
	struct sigaction sigact;
    
    if (pArg == NULL) {
        return NULL;
    }
    
    CPD_LOG(CPD_LOG_ID_TXT, "\n%u: %s()\n", getMsecTime(), __FUNCTION__);
    LOGD("%u: %s()", getMsecTime(), __FUNCTION__);
	/* Register the SIGUSR1 signal handler */
	memset(&sigact, 0, sizeof(sigact));
	sigact.sa_handler = cpdSystemMonitorThreadSignalHandler;
	sigaction(SIGUSR1, &sigact, NULL);

    pCpd = (pCPD_CONTEXT) pArg;
    pCpd->systemMonitor.lastCheck = 0;
    if (pCpd->systemMonitor.loopInterval < 100) {
        pCpd->systemMonitor.loopInterval = 100;
    }
    pCpd->systemMonitor.monitorThreadState = THREAD_STATE_RUNNING;
    CPD_LOG(CPD_LOG_ID_TXT , "\n  %u: last:%u, loopInterval:%u\n", getMsecTime(),pCpd->systemMonitor.lastCheck, pCpd->systemMonitor.loopInterval);
    LOGD("%u:%s(), last:%u, loopInterval:%u", getMsecTime(), __FUNCTION__, pCpd->systemMonitor.lastCheck, pCpd->systemMonitor.loopInterval);
    while (pCpd->systemMonitor.monitorThreadState == THREAD_STATE_RUNNING) {
        if (getMsecDt(pCpd->systemMonitor.lastCheck) >= pCpd->systemMonitor.loopInterval) {
            cpdSystemMonitorModem(pCpd);
            cpdSystemMonitorGpsSocket(pCpd);
            cpdSystemMonitorRegisterForCP(pCpd);
			pCpd->systemMonitor.lastCheck = getMsecTime();
        }
        usleep((pCpd->systemMonitor.loopInterval / 4) * 1000UL);
    }
    pCpd->systemMonitor.monitorThreadState = THREAD_STATE_TERMINATED;
    CPD_LOG(CPD_LOG_ID_TXT, "\n %u: EXIT %s()", getMsecTime(), __FUNCTION__);
    LOGD("%u: EXIT %s()", getMsecTime(), __FUNCTION__);
    return NULL;
}

int cpdSystemMonitorStart( void )
{
    int result = CPD_ERROR;
    pCPD_CONTEXT pCpd;
    pCpd = cpdGetContext();
    CPD_LOG(CPD_LOG_ID_TXT, "\n%u: %s()\n", getMsecTime(), __FUNCTION__);
    LOGD("%u: %s()\n", getMsecTime(), __FUNCTION__);
    if ((pCpd->systemMonitor.monitorThreadState == THREAD_STATE_OFF) ||
        (pCpd->systemMonitor.monitorThreadState == THREAD_STATE_TERMINATED)) {
        pCpd->systemMonitor.monitorThreadState = THREAD_STATE_STARTING; 
        result = pthread_create(&(pCpd->systemMonitor.monitorThread), NULL, cpdSystemMonitorThread, (void *) pCpd);
        usleep(1000);
        if ((result == 0) && (pCpd->modemInfo.modemReadThreadState == RUN_ACTIVE)) {
            result = CPD_OK;
        }
    }
    CPD_LOG(CPD_LOG_ID_TXT, "\n %u: %s()=%d\n", getMsecTime(), __FUNCTION__, result);
    LOGD("%u: %s()=%d", getMsecTime(), __FUNCTION__, result);
    return result;
}

int cpdSystemMonitorStop(pCPD_CONTEXT pCpd)
{
    int count = 1000;
    CPD_LOG(CPD_LOG_ID_TXT, "\n%u: %s()\n", getMsecTime(), __FUNCTION__);
    LOGD("%u: %s()", getMsecTime(), __FUNCTION__);
    if (pCpd == NULL) {
        return CPD_ERROR;
    }
    pCpd->systemMonitor.monitorThreadState = THREAD_STATE_TERMINATE;
    usleep(10000);
    if (pCpd->systemMonitor.monitorThreadState != THREAD_STATE_TERMINATED) {
		pthread_kill(pCpd->systemMonitor.monitorThread, SIGUSR1);
        usleep(100);
        pCpd->modemInfo.modemReadThreadState = THREAD_STATE_TERMINATED;
    }
    CPD_LOG(CPD_LOG_ID_TXT, "\n%u: EXIT %s()", getMsecTime(), __FUNCTION__);
    LOGD("%u: EXIT %s()", getMsecTime(), __FUNCTION__);
    return CPD_OK;
}

