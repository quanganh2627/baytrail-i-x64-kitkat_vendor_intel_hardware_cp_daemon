/*
 *  hardware/Intel/cp_daemon/cpdUtil.c
 *
 * General utilities for CPDD.
 *
 * Martin Junkar 09/18/2011
 *
 *
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <utime.h>
#include <sys/time.h>

static struct timeval start_time;

void initTime(void) 
{
	gettimeofday(&start_time, NULL);
}

unsigned int get_msec_time_now(void) 
{
	struct timeval t;
	unsigned int tMsec;
	gettimeofday(&t, NULL);
	tMsec = (t.tv_sec) * 1000 + (t.tv_usec)/1000;
	return tMsec;
}

unsigned int getMsecTime(void) 
{
	struct timeval t;
	unsigned int tMsec;
	gettimeofday(&t, NULL);
	tMsec = (t.tv_sec - start_time.tv_sec) * 1000 + (t.tv_usec - start_time.tv_usec)/1000;
	return tMsec;
}

unsigned int getMsecDt(unsigned int t)
{
	unsigned int tNow = getMsecTime();
	if (t > tNow){
		return t - tNow;
	}
	return tNow - t;
}

int cpdMoveBufferLeft(char *pB, int *pIndex, int left)
{
	if (left < 0) {
		return *pIndex;
	}
	
	if (left > *pIndex) {
		left = *pIndex;
	}
	memmove(pB, (pB + left), *pIndex);
	*pIndex = *pIndex - left;

	return *pIndex;
}

/*
 * used in debug mode, reading console input
 */
int readUserChoice(void)
{
	int choice;
	char response[10];

	do {
		fgets(response, 9, stdin);
	}while ((sscanf(response, "%d", &choice) != 1) );
	return choice;
}

/*
 * find a string of bytes in a buffer of sub-bffer.
 * similar to strstr() but it handles special cases used in CPD.
 */
int cpdFindString(char *pB, int len, char *findMe, int lenStr)
{
	int result = -1;
	char *pS = NULL;
    char *pC = NULL;
    char *pN = NULL;
    int i = 0;

	if (len <= 0) {
		return result;
	}
	if (pB == NULL) {
		return result;
	}
    pN = pB;
    while ((result < 0) && (i < len)) {
        while (i < len) {
            if (pN[i] == findMe[0]) {
                pC = &(pN[i]);
                break;
            }
            i++;
        }
            
        if (pC != NULL) {
        	pS = strstr(pC, findMe);
        	if (pS != NULL){
        		result = (int) (pS - pB);
        		if (result > len) {
        			result = -1;
        		}
                else {
                    break;
                }
        	}
        }
        else {
            break;
        }
        i++;
    }
	return result;
}

/*
 * Create & start a thread, initialize variables and wait fdor thread to confirm it is running.
 */
int cpdCreateThread()
{
	int result = 0;
/*	result = pthread_create(&(pCpd->modemInfo.modemReadThread), NULL, cpdModemReadThreadLoop, (void *) pCpd);
	usleep(1000);
	if ((result == 0) && (pCpd->modemInfo.modemReadThreadState == THREAD_STATE_RUNNING)) {
		result = 1; 
	}
*/	
	return result;
}



