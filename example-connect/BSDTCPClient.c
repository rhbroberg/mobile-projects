/*
  This example code is in public domain.

  This example code is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

/* 
This example creates a TCP Client and sends an http request to a server and prints out the first 20 bytes of the data that is received from server.

It opens the bearer by vm_bearer_open() and after the bearer is opened creates a sub thread by vm_thread_create(). In the sub thread, it establishes a connection to CONNECT_ADDRESS. After the connection is established it sends the string by send() and receives the response by recv().

You can change the connect address by modify MACRO CONNECT_ADDRESS, change the APN information according to your SIM card.
*/

#include <string.h>
#include "vmtype.h" 
#include "vmboard.h"
#include "vmsystem.h"
#include "vmlog.h" 
#include "vmcmd.h" 
#include "vmdcl.h"
#include "vmdcl_gpio.h"
#include "vmthread.h"
#include "ResID.h"
#include "BSDTCPClient.h"
#include "vmsock.h"
#include "vmbearer.h"
#include "vmtimer.h"
#include "vmgsm_gprs.h"

//#define DIST_APN
#ifdef DIST_APN
#define APN "cmwap"
#define PROXY_IP    "10.0.0.172"
#else
#define APN "T-MOBILE"
#define PROXY_IP	"92.242.140.21"

#endif

#define CONNECT_ADDRESS "103.235.46.39"
#define CONNECT_PORT 80
#define USING_PROXY VM_TRUE
//#define USING_PROXY VM_FALSE
#define PROXY_PORT  80
#define REQUEST "GET http://www.baidu.com/ HTTP/1.1\r\nHOST:www.baidu.com\r\n\r\n"
#define START_TIMER 60000
#define MAX_BUF_LEN 512

static VM_BEARER_HANDLE g_bearer_hdl;
static VM_THREAD_HANDLE g_thread_handle;
static VMINT g_soc_client;

static VMINT32 soc_sub_thread(VM_THREAD_HANDLE thread_handle, void* user_data)
{
    SOCKADDR_IN addr_in = {0};
    char buf[MAX_BUF_LEN] = {0};
    int len = 0;
    int ret;
    
    vm_log_debug("sending request");

    g_soc_client = socket(PF_INET, SOCK_STREAM, 0);
    addr_in.sin_family = PF_INET;
    addr_in.sin_addr.S_un.s_addr = inet_addr(CONNECT_ADDRESS);
    addr_in.sin_port = htons(CONNECT_PORT);
    
    ret = connect(g_soc_client, (SOCKADDR*)&addr_in, sizeof(SOCKADDR));
    strcpy(buf, REQUEST);
    ret = send(g_soc_client, buf, strlen(REQUEST), 0);
    vm_log_debug("sent; waiting for receive");

    ret = recv(g_soc_client, buf, MAX_BUF_LEN, 0);
    if(0 == ret)
    {
        vm_log_debug("Received FIN from server");
    }
    else
    {
        vm_log_debug("Received %d bytes data", ret);
    }
    buf[20] = 0;
    vm_log_debug("First 20 bytes of the data:%s", buf);
    closesocket(g_soc_client);
    return 0;
}

static void bearer_callback(VM_BEARER_HANDLE handle, VM_BEARER_STATE event, VMUINT data_account_id, void *user_data)
{
    vm_log_debug("in bearer callback");
    VMCHAR msg[1024];
    sprintf(msg, "parms are %d and %d", VM_BEARER_WOULDBLOCK, g_bearer_hdl);
    // this would get the bearer info, like ip address
    //VM_E_SOC_SUCCESS :               Get IP address successfully, result is filled.
    //ret = vm_bearer_get_data_account_id(apn, &dtacct_id);

    vm_log_debug((const char *)msg);

	if (VM_BEARER_WOULDBLOCK == g_bearer_hdl)
    {
        g_bearer_hdl = handle;
    }
    if (handle == g_bearer_hdl)
    {
        switch (event)
        {
            case VM_BEARER_DEACTIVATED:
                vm_log_debug("deactivated");
                break;
            case VM_BEARER_ACTIVATING:
                vm_log_debug("activating");
                break;
            case VM_BEARER_ACTIVATED:
                vm_log_debug("starting thread");
                g_thread_handle = vm_thread_create(soc_sub_thread, NULL, 0);
                break;
            case VM_BEARER_DEACTIVATING:
                vm_log_debug("deactivating");
                break;
            default:
                vm_log_debug("default");
            	break;
        }
    }
}

void set_custom_apn(void)
{
    VMINT ret;
    vm_gsm_gprs_apn_info_t apn_info;

    memset(&apn_info, 0, sizeof(apn_info));
    apn_info.using_proxy = USING_PROXY;
    strcpy(apn_info.apn, APN);
    strcpy(apn_info.proxy_address, PROXY_IP);
    apn_info.proxy_port = PROXY_PORT;
    ret = vm_gsm_gprs_set_customized_apn_info(&apn_info);
}

void start_doing(VM_TIMER_ID_NON_PRECISE tid, void* user_data)
{
	VMBOOL smsReady = vm_gsm_sms_is_sms_ready();
	vm_log_info("gsm is ready %d\n", smsReady);

//	vm_timer_delete_non_precise(tid);
#define USE_DA_PROXY
#ifdef USE_DA_PROXY
    set_custom_apn();
    g_bearer_hdl = vm_bearer_open(VM_BEARER_DATA_ACCOUNT_TYPE_GPRS_CUSTOMIZED_APN, NULL, bearer_callback, VM_BEARER_IPV4);
    #else
    g_bearer_hdl = vm_bearer_open(VM_BEARER_DATA_ACCOUNT_TYPE_GPRS_NONE_PROXY_APN, NULL, bearer_callback, VM_BEARER_IPV4);

#endif

    vm_log_debug("vm_bearer_open returns %d\n", g_bearer_hdl);
}

void handle_sysevt(VMINT message, VMINT param) 
{
    switch (message) 
    {
        case VM_EVENT_CREATE:
            vm_log_debug("start_doing about to start");

        	vm_timer_create_non_precise(START_TIMER, start_doing, NULL);
            break;

        case VM_EVENT_QUIT:
            break;
    }
}

#include "vmgsm_sim.h"

void vm_main(void) 
{
	VM_GSM_SIM_STATUS status;
	VMSTR imsi=NULL;
	VMSTR imei=NULL;

	VMBOOL has = vm_gsm_sim_has_card();
	int id = vm_gsm_sim_get_active_sim_card();
	imsi = (VMSTR)vm_gsm_sim_get_imsi(id);
	imei = (VMSTR)vm_gsm_sim_get_imei(id);
	status = vm_gsm_sim_get_card_status(id);
	VMBOOL smsReady = vm_gsm_sms_is_sms_ready();

	vm_log_debug("battery level is %d\n", LBattery.level());

	if (has == VM_TRUE)
	{
		vm_log_info("active sim id = %d, gsm is ready %d\n", id, smsReady);
		if(imsi != NULL)
		{
			vm_log_info("sim imsi = %s",(char*)imsi);
			vm_log_info("imei = %s",(char*)imei);
			vm_log_info("sim status = %d",(char*)status);
		}
		else
		{
			vm_log_info("query sim imsi fail\n");
		}
	}
	else
	{
		vm_log_info("no sim \n");
	}


	vm_pmng_register_system_event_callback(handle_sysevt);
}

