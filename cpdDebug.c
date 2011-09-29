/*
 *  hardware/Intel/cp_daemon/cpdDebug.c
 *
 * Proprietary (extra) debug & logging for CP DAEMON.
 * This is NOT production code!For testing and development only.
 * In production code this code won't be built and included in the binary.
 *
 * Creates extra log files in CPD_LOG_DIR     "/data/logs/gps"; where it stores data and executioon traces.
 *
 * Martin Junkar 09/18/2011
 *
 *
 */
 
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <utils/Log.h>

#ifndef LOG_TAG
#define LOG_TAG "CPDD"
#endif

#include "cpdUtil.h"
#include "cpdDebug.h"

#ifdef CLEAN_TRACE
pthread_mutex_t debugLock;
#endif

#define CPD_LOG_DIR     "/data/logs/gps"

FILE *pLog = NULL;
FILE *pModemRxLog = NULL;
FILE *pModemTxLog = NULL;
FILE *pModemRxTxLog = NULL;
FILE *pXmlRxLog = NULL;
FILE *pXmlTxLog = NULL;
int cpdDebugLogIndex = 0;
int cpdDebugLogTime = 0;
char cpdDebugFileName[256];


void cpdDebugInit(char *pPrefix)
{
    int result;
    char fileName[256];
	
    result = mkdir(CPD_LOG_DIR, 0777);
    fileName[0] = 0;
    
    pthread_mutex_init(&debugLock, NULL);
    
    if (cpdDebugLogIndex == 0) {
        cpdDebugLogTime = get_msec_time_now();
    }
    if (pPrefix != NULL) {
        snprintf(fileName, 250, "%s/log_%s_%09u_%d", CPD_LOG_DIR, pPrefix, cpdDebugLogTime, cpdDebugLogIndex);
    }
    else {
        snprintf(fileName, 250, "%s/log_%09u_%d", CPD_LOG_DIR, cpdDebugLogTime, cpdDebugLogIndex);
    }
    strcpy(cpdDebugFileName, fileName);
    
    sprintf(fileName, "%s.txt", cpdDebugFileName);
    pLog = fopen(fileName, "w");

	if (strcmp(pPrefix, "GPS") != 0) {
	    sprintf(fileName, "%s_modem_rx.txt", cpdDebugFileName);
	    pModemRxLog = fopen(fileName, "w");
	    sprintf(fileName, "%s_modem_tx.txt", cpdDebugFileName);
	    pModemTxLog = fopen(fileName, "w");
	    sprintf(fileName, "%s_modem_rxtx.txt", cpdDebugFileName);
	    pModemRxTxLog = fopen(fileName, "w");
	    sprintf(fileName, "%s_xml_rx.txt", cpdDebugFileName);
	    pXmlRxLog = fopen(fileName, "w");
	    sprintf(fileName, "%s_xml_tx.txt", cpdDebugFileName);
	    pXmlTxLog = fopen(fileName, "w");
	}
    cpdDebugLogIndex++;
}


FILE *cpdGetLogFp(int logID)
{
    FILE *fp = NULL;
    switch (logID) {
        case CPD_LOG_ID_TXT:
            fp = pLog;
            break;
        case CPD_LOG_ID_MODEM_RX:
            fp = pModemRxLog;
            break;
        case CPD_LOG_ID_MODEM_TX:
            fp = pModemTxLog;
            break;
        case CPD_LOG_ID_MODEM_RXTX:
            fp = pModemRxTxLog;
            break;
        case CPD_LOG_ID_XML_RX:
            fp = pXmlRxLog;
            break;
    }
    return fp;
}

void cpdDebugLog(int logID, const char *pFormat, ...)
{
    FILE *fp = cpdGetLogFp(logID);
    int result;
    int console = 0;
    int i;
    unsigned int mask;
    va_list args;
#ifdef CPD_DEBUG_ADD_TIMESTAMP
	char timeStampStr[64];
	getTimeString(timeStampStr, 60);
#endif

    if ((logID == 0) || ((logID & CPD_LOG_ID_CONSOLE) != 0)) {
        console = 1;
    }
    va_start(args, pFormat); //Requires the last fixed parameter (to get the address)
    
    i = 1;
    while (logID != 0) {
        mask = (1 << i);
        fp = cpdGetLogFp((logID & mask));
        if (fp != NULL) {
#ifdef CPD_DEBUG_ADD_TIMESTAMP
			fwrite(timeStampStr, 1, strlen(timeStampStr), fp);
#endif
            result = vfprintf(fp, pFormat, args);
            if (result < 0) {
                cpdDebugClose();
                cpdDebugInit(NULL);
            }
            else {
                fflush(fp);
            }
        }
        logID = logID & (~mask);
        i++;
    }
    if (console) {
        vprintf(pFormat, args);
    }
    va_end(args);
}


void cpdDebugLogData(int logID, const char *pB, int len)
{
    int result;
    FILE *fp;
    int i = 1;
    unsigned int mask;
    int console = 0;
    
    if ((logID == 0) || ((logID & CPD_LOG_ID_CONSOLE) != 0)) {
        console = 1;
    }
    while (logID != 0) {
        mask = (1 << i);
        fp = cpdGetLogFp((logID & mask));
        if (fp != NULL) {
            result = fwrite(pB, 1, len, fp);
            if (result < 0) {
                cpdDebugClose();
                cpdDebugInit(NULL);
            }
            else {
                fflush(fp);
            }
                
        }
        logID = logID & (~mask);
        i++;
    }
    if (console) {
        printf("\r\nLogData(%d)=[%s]", len, pB);
    }
}
   
void cpdDebugClose( void )
{
    if (pLog != NULL) {
        fclose(pLog);
    }
    if (pModemRxLog != NULL) {
        fclose(pModemRxLog);
    }
    if (pModemTxLog != NULL) {
        fclose(pModemTxLog);
    }
    if (pModemRxTxLog != NULL) {
        fclose(pModemRxTxLog);
    }
    if (pXmlRxLog != NULL) {
        fclose(pXmlRxLog);
    }
    if (pXmlTxLog != NULL) {
        fclose(pXmlTxLog);
    }
//    pthread_mutex_destroy(&debugLock);
}


 
