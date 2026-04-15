/******************************************************************************
* #Author       : Zafer Satilmis
* #Revision     : 1.0
* #Date         : AMar 04, 2026 - 1:43:59 PM
* #File Name    : AppInternalMsgFrame.c
*******************************************************************************/
/******************************************************************************
*
*
*******************************************************************************/
#define SHOW_PAGE_DBG_MSG  (DISABLE)
/********************************* INCLUDES ***********************************/
#include "AppNetworkService.h"
#include "../../../../Customers/NetworkService_Config.h"

#include "AppGlobalVariables.h"
#include "AppTaskManager.h"
#include "AppLogRecorder.h"


/****************************** MACRO DEFINITIONS *****************************/

/******************************* TYPE DEFINITIONS *****************************/

/********************************** VARIABLES *********************************/


/***************************** STATIC FUNCTIONS  ******************************/

/***************************** PUBLIC FUNCTIONS  ******************************/
RETURN_STATUS appNetworkServiceStart(void)
{
    RETURN_STATUS retVal = FAILURE;

    if (SUCCESS == networkServiceInitTCPIPStack())
    {
        zosDelayTask(1000); //wait for the stack to be ready before starting interfaces
        retVal = networkServiceStartInterfaces();
        if (SUCCESS == retVal)
        {
            //someone may need to know network service is ready, so network ready flag is set here
//            zosEventGroupSet(gp_systemSetupEventGrp, NETWORK_SERVICE_READY_FLAG);
            DEBUG_INFO("->[I] Network Service started");
        }
    }

    return retVal;
}

/******************************** End Of File *********************************/
