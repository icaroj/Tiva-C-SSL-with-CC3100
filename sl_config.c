/*
 * Sample application to switch to STA mode, connect and ping AP
 *
 * Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/
 *
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*
 * Application Name     -   Getting started with Wi-Fi STATION mode
 * Application Overview -   This is a sample application demonstrating how to
 *                          start CC3100 in WLAN-Station mode and connect to a
 *                          Wi-Fi access-point. The application connects to an
 *                          access-point and ping's the gateway. It also checks
 *                          for an internet connectivity by pinging "www.ti.com"
 * Application Details  -   http://processors.wiki.ti.com/index.php/CC31xx_Getting_Started_with_WLAN_Station
 *                          doc\examples\getting_started_with_wlan_station.pdf
 */

#include "simplelink.h"
#include "sl_common.h"
#include "sl_config.h"

/*
 * ASYNCHRONOUS EVENT HANDLERS -- Start
 */

/*!
    \brief This function handles WLAN events

    \param[in]      pWlanEvent is the event passed to the handler

    \return         None

    \note

    \warning
 */
void SimpleLinkWlanEventHandler(SlWlanEvent_t *pWlanEvent)
{
        if(pWlanEvent == NULL)
        {
                CLI_Write((_u8 *)" [WLAN EVENT] NULL Pointer Error \n\r");
                return;
        }

        switch(pWlanEvent->Event)
        {
        case SL_WLAN_CONNECT_EVENT:
        {
                SET_STATUS_BIT(g_Status, STATUS_BIT_CONNECTION);

                /*
                 * Information about the connected AP (like name, MAC etc) will be
                 * available in 'slWlanConnectAsyncResponse_t' - Applications
                 * can use it if required
                 *
                 * slWlanConnectAsyncResponse_t *pEventData = NULL;
                 * pEventData = &pWlanEvent->EventData.STAandP2PModeWlanConnected;
                 *
                 */
        }
        break;

        case SL_WLAN_DISCONNECT_EVENT:
        {
                slWlanConnectAsyncResponse_t*  pEventData = NULL;

                CLR_STATUS_BIT(g_Status, STATUS_BIT_CONNECTION);
                CLR_STATUS_BIT(g_Status, STATUS_BIT_IP_ACQUIRED);

                pEventData = &pWlanEvent->EventData.STAandP2PModeDisconnected;

                /* If the user has initiated 'Disconnect' request, 'reason_code' is SL_USER_INITIATED_DISCONNECTION */
                if(SL_WLAN_DISCONNECT_USER_INITIATED_DISCONNECTION == pEventData->reason_code)
                {
                        CLI_Write((_u8 *)" Device disconnected from the AP on application's request \n\r");
                }
                else
                {
                        CLI_Write((_u8 *)" Device disconnected from the AP on an ERROR..!! \n\r");
                }
        }
        break;

        default:
        {
                CLI_Write((_u8 *)" [WLAN EVENT] Unexpected event \n\r");
        }
        break;
        }
}

/*!
    \brief This function handles events for IP address acquisition via DHCP
           indication

    \param[in]      pNetAppEvent is the event passed to the handler

    \return         None

    \note

    \warning
 */
void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *pNetAppEvent)
{
        if(pNetAppEvent == NULL)
        {
                CLI_Write((_u8 *)" [NETAPP EVENT] NULL Pointer Error \n\r");
                return;
        }

        switch(pNetAppEvent->Event)
        {
        case SL_NETAPP_IPV4_IPACQUIRED_EVENT:
        {
                SlIpV4AcquiredAsync_t *pEventData = NULL;

                SET_STATUS_BIT(g_Status, STATUS_BIT_IP_ACQUIRED);

                pEventData = &pNetAppEvent->EventData.ipAcquiredV4;
                g_GatewayIP = pEventData->gateway;
        }
        break;

        default:
        {
                CLI_Write((_u8 *)" [NETAPP EVENT] Unexpected event \n\r");
        }
        break;
        }
}

/*!
    \brief This function handles callback for the HTTP server events

    \param[in]      pHttpEvent - Contains the relevant event information
    \param[in]      pHttpResponse - Should be filled by the user with the
                    relevant response information

    \return         None

    \note

    \warning
 */
void SimpleLinkHttpServerCallback(SlHttpServerEvent_t *pHttpEvent,
                                  SlHttpServerResponse_t *pHttpResponse)
{
        /*
         * This application doesn't work with HTTP server - Hence these
         * events are not handled here
         */
        CLI_Write((_u8 *)" [HTTP EVENT] Unexpected event \n\r");
}


/*!
    \brief This function handles ping report events

    \param[in]      pPingReport holds the ping report statistics

    \return         None

    \note

    \warning
 */
static void SimpleLinkPingReport(SlPingReport_t *pPingReport)
{
        SET_STATUS_BIT(g_Status, STATUS_BIT_PING_DONE);

        if(pPingReport == NULL)
        {
                CLI_Write((_u8 *)" [PING REPORT] NULL Pointer Error\r\n");
                return;
        }

        g_PingPacketsRecv = pPingReport->PacketsReceived;
}

/*!
    \brief This function handles general error events indication

    \param[in]      pDevEvent is the event passed to the handler

    \return         None
 */
void SimpleLinkGeneralEventHandler(SlDeviceEvent_t *pDevEvent)
{
        /*
         * Most of the general errors are not FATAL are are to be handled
         * appropriately by the application
         */
        CLI_Write((_u8 *)" [GENERAL EVENT] \n\r");
}

/*!
    \brief This function handles socket events indication

    \param[in]      pSock is the event passed to the handler

    \return         None
 */
void SimpleLinkSockEventHandler(SlSockEvent_t *pSock)
{
        /*
         * This application doesn't work w/ socket - Hence not handling these events
         */
        CLI_Write((_u8 *)" [SOCK EVENT] Unexpected event \n\r");
}
/*
 * ASYNCHRONOUS EVENT HANDLERS -- End
 */

/*!
    \brief This function configure the SimpleLink device in its default state. It:
           - Sets the mode to STATION
           - Configures connection policy to Auto and AutoSmartConfig
           - Deletes all the stored profiles
           - Enables DHCP
           - Disables Scan policy
           - Sets Tx power to maximum
           - Sets power policy to normal
           - Unregisters mDNS services
           - Remove all filters

    \param[in]      none

    \return         On success, zero is returned. On error, negative is returned
 */
_i32 configureSimpleLinkToDefaultState(){
        SlVersionFull ver = {0};
        _WlanRxFilterOperationCommandBuff_t RxFilterIdMask = {0};

        _u8 val = 1;
        _u8 configOpt = 0;
        _u8 configLen = 0;
        _u8 power = 0;

        _i32 retVal = -1;
        _i32 mode = -1;

        mode = sl_Start(0, 0, 0);
        ASSERT_ON_ERROR(mode);

        /* If the device is not in station-mode, try configuring it in station-mode */
        if (ROLE_STA != mode)
        {
                if (ROLE_AP == mode)
                {
                        /* If the device is in AP mode, we need to wait for this event before doing anything */
                        while(!IS_IP_ACQUIRED(g_Status)) { _SlNonOsMainLoopTask(); }
                }

                /* Switch to STA role and restart */
                retVal = sl_WlanSetMode(ROLE_STA);
                ASSERT_ON_ERROR(retVal);

                retVal = sl_Stop(SL_STOP_TIMEOUT);
                ASSERT_ON_ERROR(retVal);

                retVal = sl_Start(0, 0, 0);
                ASSERT_ON_ERROR(retVal);

                /* Check if the device is in station again */
                if (ROLE_STA != retVal)
                {
                        /* We don't want to proceed if the device is not coming up in station-mode */
                        ASSERT_ON_ERROR(DEVICE_NOT_IN_STATION_MODE);
                }
        }

        /* Get the device's version-information */
        configOpt = SL_DEVICE_GENERAL_VERSION;
        configLen = sizeof(ver);
        retVal = sl_DevGet(SL_DEVICE_GENERAL_CONFIGURATION, &configOpt, &configLen, (_u8 *)(&ver));
        ASSERT_ON_ERROR(retVal);

        /* Set connection policy to Auto + SmartConfig (Device's default connection policy) */
        retVal = sl_WlanPolicySet(SL_POLICY_CONNECTION, SL_CONNECTION_POLICY(1, 0, 0, 0, 1), NULL, 0);
        ASSERT_ON_ERROR(retVal);

        /* Remove all profiles */
        retVal = sl_WlanProfileDel(0xFF);
        ASSERT_ON_ERROR(retVal);

        /*
         * Device in station-mode. Disconnect previous connection if any
         * The function returns 0 if 'Disconnected done', negative number if already disconnected
         * Wait for 'disconnection' event if 0 is returned, Ignore other return-codes
         */
        retVal = sl_WlanDisconnect();
        if(0 == retVal)
        {
                /* Wait */
                while(IS_CONNECTED(g_Status)) { _SlNonOsMainLoopTask(); }
        }

        /* Enable DHCP client*/
        retVal = sl_NetCfgSet(SL_IPV4_STA_P2P_CL_DHCP_ENABLE,1,1,&val);
        ASSERT_ON_ERROR(retVal);

        /* Disable scan */
        configOpt = SL_SCAN_POLICY(0);
        retVal = sl_WlanPolicySet(SL_POLICY_SCAN, configOpt, NULL, 0);
        ASSERT_ON_ERROR(retVal);

        /* Set Tx power level for station mode
           Number between 0-15, as dB offset from max power - 0 will set maximum power */
        power = 0;
        retVal = sl_WlanSet(SL_WLAN_CFG_GENERAL_PARAM_ID, WLAN_GENERAL_PARAM_OPT_STA_TX_POWER, 1, (_u8 *)&power);
        ASSERT_ON_ERROR(retVal);

        /* Set PM policy to normal */
        retVal = sl_WlanPolicySet(SL_POLICY_PM, SL_NORMAL_POLICY, NULL, 0);
        ASSERT_ON_ERROR(retVal);

        /* Unregister mDNS services */
        retVal = sl_NetAppMDNSUnRegisterService(0, 0);
        ASSERT_ON_ERROR(retVal);

        /* Remove  all 64 filters (8*8) */
        pal_Memset(RxFilterIdMask.FilterIdMask, 0xFF, 8);
        retVal = sl_WlanRxFilterSet(SL_REMOVE_RX_FILTER, (_u8 *)&RxFilterIdMask,
                                    sizeof(_WlanRxFilterOperationCommandBuff_t));
        ASSERT_ON_ERROR(retVal);

        retVal = sl_Stop(SL_STOP_TIMEOUT);
        ASSERT_ON_ERROR(retVal);

        retVal = initializeAppVariables();
        ASSERT_ON_ERROR(retVal);

        return retVal; /* Success */
}

/*!
    \brief Connecting to a WLAN Access point

    This function connects to the required AP (SSID_NAME).
    The function will return once we are connected and have acquired IP address

    \param[in]  None

    \return     0 on success, negative error-code on error

    \note

    \warning    If the WLAN connection fails or we don't acquire an IP address,
                We will be stuck in this function forever.
 */
_i32 establishConnectionWithAP(){
        SlSecParams_t secParams = {0};
        _i32 retVal = 0;

        secParams.Key = (_i8 *)PASSKEY;
        secParams.KeyLen = pal_Strlen(PASSKEY);
        secParams.Type = SEC_TYPE;

        retVal = sl_WlanConnect((_i8 *)SSID_NAME, pal_Strlen(SSID_NAME), 0, &secParams, 0);
        ASSERT_ON_ERROR(retVal);

        /* Wait */
        while((!IS_CONNECTED(g_Status)) || (!IS_IP_ACQUIRED(g_Status))) { _SlNonOsMainLoopTask(); }

        return SUCCESS;
}

/*!
    \brief This function checks the LAN connection by pinging the AP's gateway

    \param[in]  None

    \return     0 on success, negative error-code on error
 */
_i32 checkLanConnection(){
        SlPingStartCommand_t pingParams = {0};
        SlPingReport_t pingReport = {0};

        _i32 retVal = -1;

        CLR_STATUS_BIT(g_Status, STATUS_BIT_PING_DONE);
        g_PingPacketsRecv = 0;

        /* Set the ping parameters */
        pingParams.PingIntervalTime = PING_INTERVAL;
        pingParams.PingSize = PING_PKT_SIZE;
        pingParams.PingRequestTimeout = PING_TIMEOUT;
        pingParams.TotalNumberOfAttempts = NO_OF_ATTEMPTS;
        pingParams.Flags = 0;
        pingParams.Ip = g_GatewayIP;

        /* Check for LAN connection */
        retVal = sl_NetAppPingStart( (SlPingStartCommand_t*)&pingParams, SL_AF_INET,
                                     (SlPingReport_t*)&pingReport, SimpleLinkPingReport);
        ASSERT_ON_ERROR(retVal);

        /* Wait */
        while(!IS_PING_DONE(g_Status)) { _SlNonOsMainLoopTask(); }

        if(0 == g_PingPacketsRecv)
        {
                /* Problem with LAN connection */
                ASSERT_ON_ERROR(LAN_CONNECTION_FAILED);
        }

        /* LAN connection is successful */
        return SUCCESS;
}

/*!
    \brief This updates the date and time of CC3100

    \param[in]      None

    \return         0 for success, negative otherwise

    \note

    \warning
 */
_i32 SetTime()
{
        _i32 retVal = -1;
        SlDateTime_t dateTime= {0};

        dateTime.sl_tm_day = DATE;
        dateTime.sl_tm_mon = MONTH;
        dateTime.sl_tm_year = YEAR;
        dateTime.sl_tm_hour = HOUR;
        dateTime.sl_tm_min = MINUTE;
        dateTime.sl_tm_sec = SECOND;

        retVal = sl_DevSet(SL_DEVICE_GENERAL_CONFIGURATION,SL_DEVICE_GENERAL_CONFIGURATION_DATE_TIME,
                           sizeof(SlDateTime_t),(_u8 *)(&dateTime));
        ASSERT_ON_ERROR(retVal);

        return SUCCESS;
}

/*!
    \brief This function initializes the application variables

    \param[in]  None

    \return     0 on success, negative error-code on error
 */
_i32 initializeAppVariables(){
        g_Status = 0;
        g_PingPacketsRecv = 0;
        g_GatewayIP = 0;

        return SUCCESS;
}
