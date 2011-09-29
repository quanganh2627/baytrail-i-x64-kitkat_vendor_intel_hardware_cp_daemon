/*
 * hardware/Intel/cp_daemon/cpdStart.c
 *
 * Startup manager for CPDD.
 * Opens gsmtty port to modem, initializes 2 SocketServers, one is Client-type for communication with GPS,
 * the other is Server-type for diagnostics and modem pass-through channel.
 * At the end configures and starts System Monitor thread.
 * In the case when CPDD is termnated, Stop() function closes connections.
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
#include <arpa/inet.h>
#include <termios.h>
#include <sys/poll.h>
#define LOG_TAG "CPDD_ST"

#include "cpd.h"
#include "cpdInit.h"
#include "cpdModemReadWrite.h"
#include "cpdSocketServer.h"
#include "cpdGpsComm.h"
#include "cpdXmlFormatter.h"
#include "cpdSystemMonitor.h"
#include "cpdUtil.h"
#include "cpdDebug.h"

/* debug code for MUX bug: */
#define xSTARTUP_DELAY	(30000UL)

int cpdStart(pCPD_CONTEXT pCpd) 
{
    int result = CPD_OK;
    int r;

    CPD_LOG(CPD_LOG_ID_TXT, "\n%u: %s()", getMsecTime(), __FUNCTION__);
    LOGD("%u: %s()", getMsecTime(), __FUNCTION__);
    pCpd->pfCposrMessageHandlerInCpd =  (int (*)(void * )) &cpdFormatAndSendMsgToGps;
    pCpd->pfMessageHandlerInCpd = (int (*)(void * )) &cpdSendCpPositionResponseToModem;
    
    pCpd->pfMessageHandlerInGps = NULL;
	/* this is for debug testing while MUX is broken*/
#ifdef STARTUP_DELAY   
	/* Debug mode: delay opening gsmtty7 */
	CPD_LOG(CPD_LOG_ID_TXT, "\r\n%u: %s()->Delayed call to cpdModemOpen()!!!", getMsecTime(), __FUNCTION__); 
	pCpd->modemInfo.keepOpenCtrl.lastOpenAt = getMsecTime() + (STARTUP_DELAY / 2) ;
	pCpd->modemInfo.keepOpenCtrl.keepOpenRetryInterval = STARTUP_DELAY;
	pCpd->modemInfo.lastDataReceived = pCpd->modemInfo.keepOpenCtrl.lastOpenAt;
	pCpd->modemInfo.lastDataSent = pCpd->modemInfo.keepOpenCtrl.lastOpenAt;
	pCpd->modemInfo.keepOpenCtrl.keepOpen = CPD_OK;
#else
    r = cpdModemOpen(pCpd);
    pCpd->modemInfo.keepOpenCtrl.keepOpen = CPD_OK;
    pCpd->modemInfo.keepOpenCtrl.keepOpenRetryInterval = 3000;
#endif	
    

    pCpd->scGps.initialized = CPD_NOK;
    pCpd->scGps.maxConnections = 1;
    pCpd->scGps.portNo = 0;
    pCpd->scGps.pfReadCallback = &cpdGpsCommMsgReader;
    pCpd->scGps.type = SOCKET_SERVER_TYPE_CLIENT;

    pCpd->scGpsKeepOpenCtrl.keepOpen = CPD_OK;
    pCpd->scGpsKeepOpenCtrl.keepOpenRetryInterval = 3000;
    
    r = cpdSocketServerInit(&(pCpd->scGps));
    if (r == CPD_OK) {
        pCpd->scIndexToGps = cpdSocketClientOpen(&(pCpd->scGps), SOCKET_HOST_GPS, SOCKET_PORT_GPS);
        pCpd->scGpsKeepOpenCtrl.lastOpenAt = getMsecTime();
        pCpd->scGpsKeepOpenCtrl.keepOpenRetryCount++;
        if (pCpd->scIndexToGps == CPD_ERROR) {
            CPD_LOG( CPD_LOG_ID_TXT, "\r\n !!!Socket to GPS Open failed %s:%d,  index=%d\n", SOCKET_HOST_GPS, SOCKET_PORT_GPS, pCpd->scIndexToGps); 
            LOGE("!!!Socket to GPS Open failed %s:%d = %d\n", SOCKET_HOST_GPS, SOCKET_PORT_GPS, pCpd->scIndexToGps); 
        }
        else {
            CPD_LOG(CPD_LOG_ID_TXT, "\r\n Socket to GPS established %s:%d, index=%d\n", SOCKET_HOST_GPS, SOCKET_PORT_GPS, pCpd->scIndexToGps); 
            LOGD("Socket to GPS established %s:%d = %d\n", SOCKET_HOST_GPS, SOCKET_PORT_GPS, pCpd->scIndexToGps); 
        }
    }

    pCpd->ssModemComm.initialized = CPD_NOK;
    pCpd->ssModemComm.maxConnections = SOCKET_SERVER_MODEM_COMM_MAX_CONNECTIONS;
    pCpd->ssModemComm.portNo = SOCKET_PORT_MODEM_COMM;
    pCpd->ssModemComm.pfReadCallback = (int (*)(void *, char *, int, int)) &cpdModemSocketWriteToAllExcpet;
    r = cpdSocketServerInit(&(pCpd->ssModemComm));
    if (r != CPD_OK){
        CPD_LOG(CPD_LOG_ID_TXT, "\r\n MODEM_COMM Socket Server init() failed = %d\n", r);
        LOGE("MODEM_COMM Socket Server init() failed = %d\n", r);
    }
    else {
        r = cpdSocketServerOpen(&(pCpd->ssModemComm));
        if (r != CPD_OK){
            CPD_LOG( CPD_LOG_ID_TXT, "\r\n MODEM_COMM Socket Server open() failed =  %d\n", r);
            LOGE("MODEM_COMM Socket Server open() failed =  %d\n", r);
        }
		else {
	        CPD_LOG(CPD_LOG_ID_TXT, "\r\n MODEM_COMM Socket Server is ready on port %d\n", pCpd->ssModemComm.portNo);
	        LOGD("MODEM_COMM Socket Server is ready on port %d\n", pCpd->ssModemComm.portNo);
		}
    }
	pCpd->systemMonitor.loopInterval = CPD_SYSTEMMONITOR_INTERVAL;
     /* for now always return OK, even if init of comm resources fails, sysyem monitor thread will restart them later.. */
	CPD_LOG(CPD_LOG_ID_TXT , "\n  %u: %s()=%d\n", getMsecTime(), __FUNCTION__, result);
	LOGD("%u: %s()=%d\n", getMsecTime(), __FUNCTION__, result);
    return result;
}


 int cpdStop(pCPD_CONTEXT pCpd) 
{
    int result = CPD_OK;
    CPD_LOG(CPD_LOG_ID_TXT , "\n%u: %s()\n", getMsecTime(), __FUNCTION__);
    LOGD("%u: %s()\n", getMsecTime(), __FUNCTION__);
    /* the end, stop monitoring service before closing connections */
    cpdSystemMonitorStop(pCpd);
    
    result = cpdModemClose(pCpd);

    result = cpdSocketServerClose(&(pCpd->scGps));
    usleep(1000);
    
    result= cpdSocketServerClose(&(pCpd->ssModemComm));
    usleep(1000);
    
	cpdDeInit();
    CPD_LOG(CPD_LOG_ID_TXT , "\n  %u: %s()=%d\n", getMsecTime(), __FUNCTION__, result);
    LOGD("%u: %s()=%d\n", getMsecTime(), __FUNCTION__, result);
    return result;
}    

