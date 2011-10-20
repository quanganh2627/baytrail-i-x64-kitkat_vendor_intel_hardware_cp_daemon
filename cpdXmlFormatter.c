/*
 * hardware/Intel/cp_daemon/cpdXmlFormatter.c
 *
 * Creates and formats XML Files for +CPOS messages.
 * Source data is response structure in CPD_CONTEXT. Converts values into 3GPP space, where needed.
 * Sends responses to modem.
 *
 * Martin Junkar 09/18/2011
 *
 */
  
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#define LOG_TAG "CPDD_XF"
 
#include "cpd.h"
#include "cpdUtil.h"
#include "cpdInit.h"
#include "cpdXmlUtils.h"
#include "cpdModemReadWrite.h"
#include "cpdDebug.h"


int cpdSendCposResponse(pCPD_CONTEXT pCpd, char *pBuff);


/*
 * Create new, empty child for xml node.
 */
xmlNode *cpdXmlNodeAddChild(xmlNode *pNode,
                                 const char *pName,
                                 const char *pValue)
{
    xmlNode *pChild;

    pChild = xmlNewDocNode(pNode->doc, NULL, (xmlChar *)  pName, (xmlChar *)  pValue);
    xmlAddChild(pNode, pChild);

    return pChild;
}
    
     
/*
* Create new, child with Integer value for xml node.
*/
xmlNode *cpdXmlNodeAddChildInt(xmlNode *pNode, const char *pName,
                                     const int value)
{
    xmlNode *pChild;
    char buf[10];

    snprintf(buf, 9, "%d", value);

    pChild = xmlNewDocNode(pNode->doc, NULL, (xmlChar *)  pName, (xmlChar *)  buf);
    xmlAddChild(pNode, pChild);

    return pChild;
}
     
     
/*
*  Create new, child with Long value for xml node.
*/
xmlNode *cpdXmlNodeAddChildLong(xmlNode *pNode,
                                         const char *pName,
                                         const long value)
{
    xmlNode *pChild;
    char buf[10];

    sprintf(buf, "%ld", value);

    pChild = xmlNewDocNode(pNode->doc, NULL, (xmlChar *)  pName, (xmlChar *)  buf);
    xmlAddChild(pNode, pChild);

    return pChild;
}
     
     
/*
*
*/
xmlNode *cpdXmlNodeAddChildAttribute(xmlNode *pNode,
     const char *name,
     const char *content,
     const char *attr_name,
     const char *attr_content)
{
    xmlNode *pChild;

    pChild = xmlNewDocNode(pNode->doc, NULL, (xmlChar *)  name, (xmlChar *)  content);
    xmlAddChild(pNode, pChild);
    xmlNewProp(pChild, (xmlChar *)  attr_name, (xmlChar *)  attr_content);

    return pChild;
}
     
     
    
    
/*
* Format Loaction data into XML pDocument.
*/
int cpdXmlFormatLocation(xmlDoc *pDoc, pLOCATION pLoc)
{
    int result = CPD_NOK;
    xmlNode *pRoot, *pNode, *pNode1, *pNode2;
    unsigned int i;
    xmlNs ns;
    char *href = "http://www.w3.org/2001/XMLSchema-instance";
    char *prefix = "pos.xsd";
    xmlDtdPtr dtd = NULL;       /* DTD pointer */
    long ltemp;
	pCPD_CONTEXT pCpd;

	pCpd = cpdGetContext();
	
    ns.context = NULL;
    ns.href = (xmlChar *) href;
    ns.prefix = (xmlChar *) prefix;
    ns.type = XML_DTD_NODE;

    CPD_LOG(CPD_LOG_ID_TXT , "\n%u: %s()\n", getMsecTime(), __FUNCTION__);

    /* Create the pos pNode & add it to the pDocument */
    pRoot = xmlNewNode(NULL, (xmlChar *) "pos");
//    pRoot = xmlNewDocRawNode(NULL, "pos.xsd", "pos","content");
//    pRoot = xmlNewDocNodeEatName(NULL, &ns, (xmlChar *) "pos","content");
//    pRoot = xmlNewDocNode(NULL, &ns, "pos", NULL);
    // xsi:noNamespaceSchemaLocation="pos.xsd" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
    xmlDocSetRootElement(pDoc, pRoot);
    
//    dtd = xmlCreateIntSubset(pDoc, BAD_CAST "pos", NULL, BAD_CAST "tree2.dtd");
    
    /* Create the location pNode & add it to the pos pNode */
    pRoot = cpdXmlNodeAddChild(pRoot, "location", NULL);

    /* Creates the location_parameters pNode */
    pRoot = cpdXmlNodeAddChild(pRoot, "location_parameters", NULL);

    /* time */
    /* cpdXmlNodeAddChildInt(pRoot, "time", pLoc->time); FIXME : Not used by the IFX modem */

    /* direction */
    /* cpdXmlNodeAddChildInt(pRoot, "direction", pLoc->direction); FIXME : Not used by the IFX modem */

    /* shape_data */
    pRoot = cpdXmlNodeAddChild(pRoot, "shape_data",  NULL);
    CPD_LOG(CPD_LOG_ID_TXT, "\r\n %s(), shape type = %d", __FUNCTION__, pLoc->location_parameters.shape_type);

    switch (pLoc->location_parameters.shape_type) {
    case SHAPE_TYPE_POINT:
        /* ellipsoid_point */
        pNode = cpdXmlNodeAddChild(pRoot, "ellipsoid_point",  NULL);

        pNode1 = cpdXmlNodeAddChild(pNode,  "coordinate",  NULL);
        pNode2 = cpdXmlNodeAddChild(pNode1, "latitude", NULL);
        cpdXmlNodeAddChild(pNode2, "north", xmlBoolToString(pLoc->location_parameters.shape_data.point.coordinate.latitude.north));
        cpdXmlNodeAddChildLong(pNode2, "degrees", pLoc->location_parameters.shape_data.point.coordinate.latitude.degrees);
        cpdXmlNodeAddChildLong(pNode1, "longitude", pLoc->location_parameters.shape_data.point.coordinate.longitude);
        break;

    case SHAPE_TYPE_POINT_UNCERT_CIRCLE:
        /* ellipsoid_point_uncert_circle */
        pNode = cpdXmlNodeAddChild(pRoot, "ellipsoid_point_uncert_circle",  NULL);

        pNode1 = cpdXmlNodeAddChild(pNode,  "coordinate",  NULL);
        pNode2 = cpdXmlNodeAddChild(pNode1, "latitude", NULL);
        cpdXmlNodeAddChild(pNode2, "north", xmlBoolToString(pLoc->location_parameters.shape_data.point_uncert_circle.coordinate.latitude.north));
        cpdXmlNodeAddChildLong(pNode2, "degrees", pLoc->location_parameters.shape_data.point_uncert_circle.coordinate.latitude.degrees);
        cpdXmlNodeAddChildLong(pNode1, "longitude", pLoc->location_parameters.shape_data.point_uncert_circle.coordinate.longitude);

        cpdXmlNodeAddChildInt(pNode,  "uncert_circle", pLoc->location_parameters.shape_data.point_uncert_circle.uncert_circle);
        break;

    case SHAPE_TYPE_POINT_UNCERT_ELLIPSE:
        /* ellipsoid_point_uncert_ellipse */
        pNode = cpdXmlNodeAddChild(pRoot, "ellipsoid_point_uncert_ellipse",  NULL);

        pNode1 = cpdXmlNodeAddChild(pNode,  "coordinate",  NULL);
        pNode2 = cpdXmlNodeAddChild(pNode1, "latitude", NULL);
        cpdXmlNodeAddChild(pNode2, "north", xmlBoolToString(pLoc->location_parameters.shape_data.point_uncert_ellipse.coordinate.latitude.north));
        
        ltemp = (long) (93206.75556 * pLoc->location_parameters.shape_data.point_uncert_ellipse.coordinate.latitude.degrees);        
        ltemp = ltemp & 0xFFFFFF;
        cpdXmlNodeAddChildLong(pNode2, "degrees", ltemp);

        ltemp = (long) (46603.37778 * pLoc->location_parameters.shape_data.point_uncert_ellipse.coordinate.longitude);
        ltemp = ltemp & 0xFFFFFF;
        cpdXmlNodeAddChildLong(pNode1, "longitude", ltemp);

        pNode1 = cpdXmlNodeAddChild(pNode,  "uncert_ellipse", NULL);
        cpdXmlNodeAddChildInt(pNode1,  "uncert_semi_major", pLoc->location_parameters.shape_data.point_uncert_ellipse.uncert_semi_major);
        cpdXmlNodeAddChildInt(pNode1,  "uncert_semi_minor", pLoc->location_parameters.shape_data.point_uncert_ellipse.uncert_semi_minor);
        cpdXmlNodeAddChildInt(pNode1,  "orient_major", pLoc->location_parameters.shape_data.point_uncert_ellipse.orient_major);
        cpdXmlNodeAddChildInt(pNode1,  "confidence", pLoc->location_parameters.shape_data.point_uncert_ellipse.confidence);
        break;

    case SHAPE_TYPE_POLYGON:
        /* polygon */
        pNode = cpdXmlNodeAddChild(pRoot, "polygon",  NULL);

        for (i=0; i<pLoc->location_parameters.shape_data.polygon.nItems; i++) {
            pNode1 = cpdXmlNodeAddChild(pNode,  "coordinate",  NULL);
            pNode2 = cpdXmlNodeAddChild(pNode1, "latitude", NULL);
            cpdXmlNodeAddChild(pNode2, "north", xmlBoolToString(pLoc->location_parameters.shape_data.polygon.pCoordinates[i].latitude.north));
            cpdXmlNodeAddChildLong(pNode2, "degrees", pLoc->location_parameters.shape_data.polygon.pCoordinates[i].latitude.degrees);
            cpdXmlNodeAddChildLong(pNode1, "longitude", pLoc->location_parameters.shape_data.polygon.pCoordinates[i].longitude);
        }
        break;

    case SHAPE_TYPE_POINT_ALT:
        /* ellipsoid_point_alt */
        pNode = cpdXmlNodeAddChild(pRoot, "ellipsoid_point_alt",  NULL);

        pNode1 = cpdXmlNodeAddChild(pNode,  "coordinate",  NULL);
        pNode2 = cpdXmlNodeAddChild(pNode1, "latitude", NULL);
        cpdXmlNodeAddChild(pNode2, "north", xmlBoolToString(pLoc->location_parameters.shape_data.point_alt.coordinate.latitude.north));
        cpdXmlNodeAddChildLong(pNode2, "degrees", pLoc->location_parameters.shape_data.point_alt.coordinate.latitude.degrees);
        cpdXmlNodeAddChildLong(pNode1, "longitude", pLoc->location_parameters.shape_data.point_alt.coordinate.longitude);

        pNode1 = cpdXmlNodeAddChild(pNode,  "altitude", NULL);
        cpdXmlNodeAddChildInt(pNode1, "height_above_surface", pLoc->location_parameters.shape_data.point_alt.altitude.height_above_surface);
        cpdXmlNodeAddChildInt(pNode1, "height", pLoc->location_parameters.shape_data.point_alt.altitude.height);
        break;

    case SHAPE_TYPE_POINT_ALT_UNCERT_ELLIPSE:
        /* ellipsoid_point_alt_uncertellipse */
        pNode = cpdXmlNodeAddChild(pRoot, "ellipsoid_point_alt_uncertellipse",  NULL);

        pNode1 = cpdXmlNodeAddChild(pNode,  "coordinate",  NULL);
        pNode2 = cpdXmlNodeAddChild(pNode1, "latitude", NULL);
        cpdXmlNodeAddChild(pNode2, "north", xmlBoolToString(pLoc->location_parameters.shape_data.point_alt_uncertellipse.coordinate.latitude.north));
        ltemp = (long) (93206.75556 * pLoc->location_parameters.shape_data.point_uncert_ellipse.coordinate.latitude.degrees);        
        ltemp = ltemp & 0xFFFFFF;
        CPD_LOG(CPD_LOG_ID_TXT, "\r\n Lat=%f = %d", pLoc->location_parameters.shape_data.point_uncert_ellipse.coordinate.latitude.degrees, ltemp);
        cpdXmlNodeAddChildLong(pNode2, "degrees", ltemp);

//        ltemp = (long) (46603.37778 * pLoc->location_parameters.shape_data.point_uncert_ellipse.coordinate.longitude);
//        ltemp = ltemp & 0xFFFFFF;
//		if (pCpd->request.posMeas.flag == POS_MEAS_RRC) {
			ltemp = (long) (46603.0 * pLoc->location_parameters.shape_data.point_uncert_ellipse.coordinate.longitude);
//    	}
        CPD_LOG(CPD_LOG_ID_TXT, "\r\n Lon=%f = %d, flag=%d", pLoc->location_parameters.shape_data.point_uncert_ellipse.coordinate.longitude, ltemp, 
			pCpd->request.posMeas.flag);
        LOGD("Lon=%f = %d, flag=%d", pLoc->location_parameters.shape_data.point_uncert_ellipse.coordinate.longitude, ltemp, 
			pCpd->request.posMeas.flag);
        cpdXmlNodeAddChildLong(pNode1, "longitude", ltemp);

        pNode1 = cpdXmlNodeAddChild(pNode,  "altitude", NULL);
        cpdXmlNodeAddChildInt(pNode1, "height_above_surface", pLoc->location_parameters.shape_data.point_alt_uncertellipse.altitude.height_above_surface);
        cpdXmlNodeAddChildInt(pNode1, "height", pLoc->location_parameters.shape_data.point_alt_uncertellipse.altitude.height);

        cpdXmlNodeAddChildInt(pNode,  "uncert_semi_major", pLoc->location_parameters.shape_data.point_alt_uncertellipse.uncert_semi_major);
        cpdXmlNodeAddChildInt(pNode,  "uncert_semi_minor", pLoc->location_parameters.shape_data.point_alt_uncertellipse.uncert_semi_minor);
        cpdXmlNodeAddChildInt(pNode,  "orient_major", pLoc->location_parameters.shape_data.point_alt_uncertellipse.orient_major);
        cpdXmlNodeAddChildInt(pNode,  "confidence", pLoc->location_parameters.shape_data.point_alt_uncertellipse.confidence);
        cpdXmlNodeAddChildInt(pNode,  "uncert_alt", pLoc->location_parameters.shape_data.point_alt_uncertellipse.uncert_alt);
        break;

    case SHAPE_TYPE_ARC:
        /* ellips_arc */
        pNode = cpdXmlNodeAddChild(pRoot, "ellips_arc",  NULL);

        pNode1 = cpdXmlNodeAddChild(pNode,  "coordinate",  NULL);
        pNode2 = cpdXmlNodeAddChild(pNode1, "latitude", NULL);
        cpdXmlNodeAddChild(pNode2, "north", xmlBoolToString(pLoc->location_parameters.shape_data.ellips_arc.coordinate.latitude.north));
        cpdXmlNodeAddChildLong(pNode2, "degrees", pLoc->location_parameters.shape_data.ellips_arc.coordinate.latitude.degrees);
        cpdXmlNodeAddChildLong(pNode1, "longitude", pLoc->location_parameters.shape_data.ellips_arc.coordinate.longitude);

        cpdXmlNodeAddChildInt(pNode,  "inner_rad",  pLoc->location_parameters.shape_data.ellips_arc.inner_rad);
        cpdXmlNodeAddChildInt(pNode,  "uncert_rad",  pLoc->location_parameters.shape_data.ellips_arc.uncert_rad);
        cpdXmlNodeAddChildInt(pNode,  "offset_angle",  pLoc->location_parameters.shape_data.ellips_arc.offset_angle);
        cpdXmlNodeAddChildInt(pNode,  "included_angle",  pLoc->location_parameters.shape_data.ellips_arc.included_angle);
        cpdXmlNodeAddChildInt(pNode,  "confidence",  pLoc->location_parameters.shape_data.ellips_arc.confidence);
        break;
    case SHAPE_TYPE_NONE:
        break;
    }

    result = CPD_OK;
    return result;
}



/*
 * Send CPOS response to modem.
 * Creates AT+CPOS <XML> reponse(s) from XML data.
 * If needed, split large XML into pieces (handle mux/modem limit on packet size.
 */
int cpdSendCposResponse(pCPD_CONTEXT pCpd, char *pBuff)
{
	int result = CPD_NOK;
    int rr = 0;
    int len = 0;
	char pTxBuffer[MODEM_MUX_AT_CMD_MAX_LENGTH +1];

    CPD_LOG(CPD_LOG_ID_TXT , "\n  %u: %s()\n", getMsecTime(), __FUNCTION__);
    sprintf(pTxBuffer, "%s%s%s%c", AT_CMD_CRLF, AT_CMD_AT, AT_CMD_CPOS, AT_CMD_CR_CHR);
//    sprintf(pTxBuffer, "%s%s%s: ", AT_CMD_CRLF, AT_CMD_AT, AT_CMD_CPOS);
    len = len + strlen(pTxBuffer);
	pCpd->modemInfo.waitForThisResponse = AT_RESPONSE_CRLF;
    rr = cpdModemSendCommand(pCpd, pTxBuffer, strlen(pTxBuffer),1000UL);
	pCpd->modemInfo.waitForThisResponse = AT_RESPONSE_NONE;
    len = len + strlen(pBuff);
    rr = rr + cpdModemSendCommand(pCpd, pBuff, strlen(pBuff),0);
    sprintf(pTxBuffer, "%c%s", AT_CMD_CTRL_Z_CHR, AT_CMD_CRLF);
    len = len + strlen(pTxBuffer);
	pCpd->modemInfo.haveResponse = 0;
	pCpd->modemInfo.responseValue = CPD_ERROR;
    rr = rr + cpdModemSendCommand(pCpd, pTxBuffer, strlen(pTxBuffer),1000UL); /* TODO: determine proper timeout */
    CPD_LOG(CPD_LOG_ID_TXT, "\n  %u: %s(), rr=%d, len=%d", getMsecTime(), __FUNCTION__, rr, len);
    if (rr == len) {
        result = CPD_OK;
    }
	return result;
}


 


int cpdSendCpPositionResponseToModem(pCPD_CONTEXT pCpd)
{
    int result = 0;
    xmlDoc *pDoc;
    xmlBuffer *pXmlBuffer;
    xmlOutputBuffer *pOutBuffer;
    pLOCATION pLocation;

    pLocation = &(pCpd->response.location);
    CPD_LOG(CPD_LOG_ID_TXT , "\n  %u: %s()\n", getMsecTime(), __FUNCTION__);
    LOGD("%u: %s()\n", getMsecTime(), __FUNCTION__);
	CPD_LOG(CPD_LOG_ID_TXT , "\n Location : %f, %f, %d",
		pCpd->response.location.location_parameters.shape_data.point_alt_uncertellipse.coordinate.longitude,
		pCpd->response.location.location_parameters.shape_data.point_alt_uncertellipse.coordinate.latitude.degrees,
		pCpd->response.location.location_parameters.shape_data.point_alt_uncertellipse.altitude.height
		);
	LOGD("Rx Location : %f, %f, %d",
		pCpd->response.location.location_parameters.shape_data.point_alt_uncertellipse.coordinate.longitude,
		pCpd->response.location.location_parameters.shape_data.point_alt_uncertellipse.coordinate.latitude.degrees,
		pCpd->response.location.location_parameters.shape_data.point_alt_uncertellipse.altitude.height
		);
	CPD_LOG(CPD_LOG_ID_TXT | CPD_LOG_ID_CONSOLE, "\n TTFF : %u, %u, %u, %u, %u",
		(pCpd->response.dbgStats.posReceivedFromGps1 - pCpd->response.dbgStats.posRequestedFromGps),
		pCpd->response.dbgStats.posRequestedByNetwork,
		pCpd->response.dbgStats.posRequestedFromGps,
		pCpd->response.dbgStats.posReceivedFromGps1,
		pCpd->response.dbgStats.posReceivedFromGps
		);
	LOGD("TTFF : %u, %u, %u, %u, %u",
		(pCpd->response.dbgStats.posReceivedFromGps1 - pCpd->response.dbgStats.posRequestedFromGps),
		pCpd->response.dbgStats.posRequestedByNetwork,
		pCpd->response.dbgStats.posRequestedFromGps,
		pCpd->response.dbgStats.posReceivedFromGps1,
		pCpd->response.dbgStats.posReceivedFromGps
		);
	
    pDoc = xmlNewDoc((xmlChar *) "1.0");
    pXmlBuffer = xmlBufferCreate();
    pOutBuffer = xmlOutputBufferCreateBuffer(pXmlBuffer, NULL);

    /* Create the XML document from data */
    result = cpdXmlFormatLocation(pDoc, pLocation);
    if (result == CPD_OK) {
        /* Convert  XML Document into pXmlBuffer */
        xmlSaveFormatFileTo(pOutBuffer, pDoc, "utf-8", 1);
        CPD_LOG(CPD_LOG_ID_TXT, "\r\npOutBuffer[");
        CPD_LOG_DATA(CPD_LOG_ID_TXT | CPD_LOG_ID_XML_TX, (char *)pXmlBuffer->content ,strlen((char *)pXmlBuffer->content));
        CPD_LOG(CPD_LOG_ID_XML_TX, "\r\n");
        CPD_LOG(CPD_LOG_ID_TXT, "]\r\n");

        /* Send response to the modem */
		CPD_LOG(CPD_LOG_ID_TXT , "\n  %u: %u, dT=%u, alreadySent=%d\n", getMsecTime(), 
			pCpd->modemInfo.sendingCPOSat, getMsecDt(pCpd->modemInfo.sendingCPOSat),
			pCpd->modemInfo.sentCPOSok);
		if (pCpd->modemInfo.sentCPOSok != CPD_OK) {
			CPD_LOG(CPD_LOG_ID_TXT , "\n  %u: %u!= 0\n", getMsecTime(), pCpd->modemInfo.sendingCPOSat);
			if (getMsecDt(pCpd->modemInfo.sendingCPOSat) >= 1000UL) {
				pCpd->modemInfo.sendingCPOSat = getMsecTime();
				pCpd->modemInfo.haveResponse = 0;
				pCpd->modemInfo.responseValue = CPD_ERROR;
        		result = cpdSendCposResponse(pCpd, (char *)pXmlBuffer->content);
				if (result == CPD_OK) {
					if ((pCpd->modemInfo.haveResponse != 0) &&
						(pCpd->modemInfo.responseValue == AT_RESPONSE_OK)) {
						pCpd->modemInfo.sentCPOSok = CPD_OK;
						pCpd->systemMonitor.processingRequest = CPD_NOK; 
					}
					else {
						result = CPD_NOK;
					}
				}
				CPD_LOG(CPD_LOG_ID_TXT , "\n %u: cpdSendCposResponse() = %d, Modem: %d, %d, CPOSsent=%d", 
					getMsecTime(), result, pCpd->modemInfo.haveResponse, pCpd->modemInfo.responseValue, pCpd->modemInfo.sentCPOSok); 
			}
			else {
				CPD_LOG(CPD_LOG_ID_TXT , "\n  %u:Not sending response to modem, dT=%u\n", 
					getMsecTime(), getMsecDt(pCpd->modemInfo.sendingCPOSat));
			}
		}
    }

    xmlBufferFree(pXmlBuffer);

    /* Release allocated resoucrs */
    xmlFreeDoc(pDoc);
    //xmlOutputBufferClose(pOutBuffer);
    CPD_LOG(CPD_LOG_ID_TXT, "\r\n %u: END %s() = %d", getMsecTime(), __FUNCTION__, result);
    LOGD("%u: END %s() = %d", getMsecTime(), __FUNCTION__, result);
    return result;
}
