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
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <poll.h>
#define LOG_TAG "CPDD_SM"

#include "cpd.h"
#include "cpdInit.h"
#include "cpdUtil.h"
#include "cpdModemReadWrite.h"
#include "cpdDebug.h"

/* this is from kernel-mode PM driver */
#define OS_STATE_ON 	0
#define OS_PM_CURRENT_STATE_NAME "/sys/power/current_state"

#define PM_STATE_BUFFER_SIZE	64


static void cpdSystemMonitorThreadSignalHandler(int );
static int cpdInitSystemPowerState(pCPD_CONTEXT pCpd);
static int cpdGetSystemPowerState(pCPD_CONTEXT);
static void cpdCloseSystemPowerState(pCPD_CONTEXT );
static int cpdReadSystemPowerState(pCPD_CONTEXT );


static int cpdReadSystemPowerState(pCPD_CONTEXT pCpd)
{
	int state = CPD_ERROR;
	int nRd;
	char pmStateBuffer[PM_STATE_BUFFER_SIZE];
	CPD_LOG(CPD_LOG_ID_TXT, "\n%u:%s()\n", getMsecTime(), __FUNCTION__);
	LOGD("%u:%s()", getMsecTime(), __FUNCTION__);
	if (pCpd->systemMonitor.pmfd < 0) {
		LOGE("Can't read %s (fd=%d), power management not enabled!", OS_PM_CURRENT_STATE_NAME, pCpd->systemMonitor.pmfd);
		return state;
	}
	lseek(pCpd->systemMonitor.pmfd, 0, SEEK_SET);
	nRd = read(pCpd->systemMonitor.pmfd, &pmStateBuffer, sizeof(pmStateBuffer)-1);
	if (nRd <= 0) {
		LOGE("%u: Unexpected PM value, nRd=%d", getMsecTime(),nRd);
		cpdCloseSystemPowerState(pCpd);
		return state;
	}
	pmStateBuffer[nRd] = 0;
	CPD_LOG(CPD_LOG_ID_TXT, "\n%u:PM(%d)=%s\n", getMsecTime(), nRd, pmStateBuffer);
	LOGD("%u:PM(%d)=%s", getMsecTime(), nRd, pmStateBuffer);
	state = atoi(pmStateBuffer);
	CPD_LOG(CPD_LOG_ID_TXT, "\n%u:%s()=%d\n", getMsecTime(), __FUNCTION__, state);
	LOGD("%u:%s()=%d", getMsecTime(), __FUNCTION__, state);
	return state;
}

/*
 * Initialize PM monitoring.
 * return curent System power mode: CPD_OK = Active, CPD_NOK = Inactive
 */
static int cpdInitSystemPowerState(pCPD_CONTEXT pCpd)
{
	int result = CPD_ERROR;
	int state;

	CPD_LOG(CPD_LOG_ID_TXT, "\n%u:%s()\n", getMsecTime(), __FUNCTION__);
	LOGD("%u:%s()", getMsecTime(), __FUNCTION__);
	if (pCpd->systemMonitor.pmfd < 0) {
		pCpd->systemMonitor.pmfd = open(OS_PM_CURRENT_STATE_NAME, O_RDONLY);
	}
	state = cpdReadSystemPowerState(pCpd);
	if (state == OS_STATE_ON){
		result = CPD_OK;
	}
	else {
		result = CPD_NOK;
	}
	CPD_LOG(CPD_LOG_ID_TXT, "\n%u:%s()=%d\n", getMsecTime(), __FUNCTION__, result);
	LOGD("%u:%s()=%d", getMsecTime(), __FUNCTION__, result);
	return result;
}	

/*
 * Monitor system power state.
 * Exit woth CPD_OK, if systemm is in active state.
 * Do not return if system is in inactive state.
 * Exit with an error if there is error in PM state.
 */
static int cpdGetSystemPowerState(pCPD_CONTEXT pCpd)
{
	int result = CPD_ERROR;
	int ret, state;
	struct pollfd fds;

	CPD_LOG(CPD_LOG_ID_TXT, "\n%u:%s()\n", getMsecTime(), __FUNCTION__);
	LOGD("%u:%s()", getMsecTime(), __FUNCTION__);
	if (pCpd->systemMonitor.pmfd < 0)
	{
		LOGE("Power management is not enabled/supported.");
		CPD_LOG(CPD_LOG_ID_TXT, "Power management is not enabled/supported.");
		return result;
	}
	fds.fd = pCpd->systemMonitor.pmfd;
	fds.events = POLLERR | POLLPRI;
	state = cpdReadSystemPowerState(pCpd);
	if (state == OS_STATE_ON){
		result = CPD_OK;
	}
	if (pCpd->systemMonitor.processingRequest == CPD_OK) {
		result = pCpd->systemMonitor.processingRequest;
	}
	while (result != CPD_OK) {
		ret = poll(&fds, 1, -1);
		if (ret > 0) {
			if (fds.revents & (POLLERR | POLLPRI)) {
				state = cpdReadSystemPowerState(pCpd);
				if (state == OS_STATE_ON){
					result = CPD_OK;
					/* System is alive, exit, handle events.. */
				}
				else if (state < 0) {
					result = CPD_ERROR;
					/* PM error, exit! */
					break;
				}
				else {
					result = CPD_NOK;
					/* sysytem is in inactive state, wait here */
				}
			}
		} 
		else {
			result = CPD_ERROR;
			CPD_LOG(CPD_LOG_ID_TXT, "\n%u:%s(), poll resul=%d\n", getMsecTime(), __FUNCTION__, ret);
			LOGE("%u:%s(), poll result=%d",getMsecTime(), __FUNCTION__, ret);
			break;
		}
	}
	CPD_LOG(CPD_LOG_ID_TXT, "\n%u:%s()=%d\n", getMsecTime(), __FUNCTION__, result);
	LOGD("%u:%s()=%d",getMsecTime(), __FUNCTION__, result);
	return result;
}

/*
 * Close and disable Power management.
 */
static void cpdCloseSystemPowerState(pCPD_CONTEXT pCpd)
{
	CPD_LOG(CPD_LOG_ID_TXT, "\n%u:%s()\n", getMsecTime(), __FUNCTION__);
	LOGD("%u:%s()", getMsecTime(), __FUNCTION__);
	if (pCpd->systemMonitor.pmfd >= 0)
	{
		close(pCpd->systemMonitor.pmfd);
		pCpd->systemMonitor.pmfd = CPD_ERROR;
	}
}



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
    if ((pCpd->modemInfo.modemReadThreadState != THREAD_STATE_RUNNING) ||
		(pCpd->modemInfo.modemFd < 0)) {
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
		if (cpdGetSystemPowerState(pCpd) == CPD_OK) {
		    usleep((pCpd->systemMonitor.loopInterval / 4) * 1000UL);
		}
		else {
			/* try to re-initialize PM, use longer delay than in active mode */
		    usleep((pCpd->systemMonitor.loopInterval) * 1000UL);
			cpdInitSystemPowerState(pCpd);
		}
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
	
	pCpd->systemMonitor.pmfd = -1;
	cpdInitSystemPowerState(pCpd);
	
    if ((pCpd->systemMonitor.monitorThreadState == THREAD_STATE_OFF) ||
        (pCpd->systemMonitor.monitorThreadState == THREAD_STATE_TERMINATED)) {
        pCpd->systemMonitor.monitorThreadState = THREAD_STATE_STARTING; 
        result = pthread_create(&(pCpd->systemMonitor.monitorThread), NULL, cpdSystemMonitorThread, (void *) pCpd);
        sleep(1UL);
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
	cpdCloseSystemPowerState(pCpd);
	usleep(1000);
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

