#include "Arduino.h"
#include "vmlog.h"
#include "vmhttps.h"
#include "vmgsm_gprs.h"
#include "stdio.h"

VMUINT8 g_channel_id;
VMINT g_read_seg_num;

VMUINT32 cachedRequestId;
VMUINT8 cachedChannelId;
int firstSend = 1;

#ifdef USE_TMOBILE
#define CUST_APN "T-MOBILE"
#define PROXY_ADDRESS   "92.242.140.21"
#define USING_PROXY VM_TRUE
#endif

#define CUST_APN "wholesale"
#define PROXY_ADDRESS   "0.0.0.0"
#define USING_PROXY VM_FALSE

#define PROXY_PORT  80              /* The proxy port */
#define VMHTTPS_TEST_DELAY 60000    /* 60 seconds */

VMCHAR myUrl[1024];

static void https_send_request_set_channel_rsp_cb(VMUINT32 req_id, VMUINT8 channel_id, VM_HTTPS_RESULT result) {
  VMINT ret = -1;

  if (firstSend)
    {
      vm_log_debug("first time sending");
      firstSend = 0;
      cachedRequestId = req_id;
      cachedChannelId = channel_id;
    }
  else
    {
      vm_log_debug("using cached settings");
      req_id = cachedRequestId;
      channel_id = cachedChannelId;
    }

  vm_log_debug("sending request now");

  vm_log_debug((const char *)myUrl);

  ret = vm_https_send_request(0, /* Request ID */
      VM_HTTPS_METHOD_GET, /* HTTP Method Constant */
      VM_HTTPS_OPTION_NO_CACHE, /* HTTP request options */
      VM_HTTPS_DATA_TYPE_BUFFER, /* Reply type (wps_data_type_enum) */
      100, /* bytes of data to be sent in reply at a time. If data is more that this, multiple response would be there */
      (VMSTR) myUrl, /* The request URL */
      strlen((const char *)myUrl), /* The request URL length */
      NULL, /* The request header */
      0, /* The request header length */
      NULL, 0);

  if (ret != 0) {
      vm_https_unset_channel(channel_id);
  }
}

static void https_unset_channel_rsp_cb(VMUINT8 channel_id, VM_HTTPS_RESULT result) {
  vm_log_debug("https_unset_channel_rsp_cb()");
  firstSend = 1;
}
static void https_send_release_all_req_rsp_cb(VM_HTTPS_RESULT result) {
  vm_log_debug("https_send_release_all_req_rsp_cb()");
}
static void https_send_termination_ind_cb(void) {
  vm_log_debug("https_send_termination_ind_cb()");
}
static void https_send_read_request_rsp_cb(VMUINT16 request_id, VM_HTTPS_RESULT result,
    VMUINT16 status, VMINT32 cause, VM_HTTPS_PROTOCOL protocol,
    VMUINT32 content_length, VMBOOL more, VMSTR content_type,
    VMUINT8 content_type_len, VMSTR new_url, VMUINT32 new_url_len,
    VMSTR reply_header, VMUINT32 reply_header_len,
    VMSTR reply_segment, VMUINT32 reply_segment_len)
{
  VMINT ret = -1;
  vm_log_debug("https_send_request_rsp_cb()");
  if (result != 0) {
      vm_https_cancel(request_id);
      vm_https_unset_channel(g_channel_id);
  } else {
      vm_log_debug("reply_content:%s", reply_segment);
      ret = vm_https_read_content(request_id, ++g_read_seg_num, 100);
      if (reply_segment != NULL && (ret != 0)) {
          vm_log_debug("read_content returned non-zero but reply-segment was non-NULL; cancelling to try again next time");
          vm_https_cancel(request_id);
          vm_https_unset_channel(g_channel_id);
          firstSend = 1;
      }
  }
}

static void https_send_read_read_content_rsp_cb(VMUINT16 request_id, VMUINT8 seq_num,
    VM_HTTPS_RESULT result, VMBOOL more,
    short int * reply_segment, VMUINT32 reply_segment_len)
{
  VMINT ret = -1;
  vm_log_debug("reply_content:%s", reply_segment);
  if (more > 0) {
      ret = vm_https_read_content(request_id, /* Request ID */
          ++g_read_seg_num, /* Sequence number (for debug purpose) */
          100); /* The suggested segment data length of replied data in the peer buffer of
                 response. 0 means use reply_segment_len in MSG_ID_WPS_HTTP_REQ or
                 read_segment_length in previous request. */
      if (ret != 0) {
          vm_https_cancel(request_id);
          vm_https_unset_channel(g_channel_id);
      }
  } else {
      /* don't want to send more requests, so unset channel */
#ifdef NO_KEEP_R
      vm_https_cancel(request_id);
      vm_https_unset_channel(g_channel_id);
      g_channel_id = 0;
      g_read_seg_num = 0;
#endif
  }
}
static void https_send_cancel_rsp_cb(VMUINT16 request_id, VM_HTTPS_RESULT result) {
  vm_log_debug("https_send_cancel_rsp_cb()");
}
static void https_send_status_query_rsp_cb(VMUINT8 status) {
  vm_log_debug("https_send_status_query_rsp_cb()");
}

void set_custom_apn(void) {
  vm_gsm_gprs_apn_info_t apn_info;

  memset(&apn_info, 0, sizeof(apn_info));
  strcpy((char *)apn_info.apn, (const char *)CUST_APN);
  strcpy((char *)apn_info.proxy_address, (const char *)PROXY_ADDRESS);
  apn_info.proxy_port = PROXY_PORT;
  apn_info.using_proxy = USING_PROXY;
  vm_gsm_gprs_set_customized_apn_info(&apn_info);
}

void https_send_request()
{
  if (firstSend)
    {
      /*----------------------------------------------------------------*/
      /* Local Variables                                                */
      /*----------------------------------------------------------------*/
      VMINT ret = -1;
      VM_BEARER_DATA_ACCOUNT_TYPE apn = VM_BEARER_DATA_ACCOUNT_TYPE_GPRS_CUSTOMIZED_APN;
      vm_https_callbacks_t callbacks = { https_send_request_set_channel_rsp_cb,
          https_unset_channel_rsp_cb, https_send_release_all_req_rsp_cb,
          https_send_termination_ind_cb, https_send_read_request_rsp_cb,
          https_send_read_read_content_rsp_cb, https_send_cancel_rsp_cb,
          https_send_status_query_rsp_cb };
      /*----------------------------------------------------------------*/
      /* Code Body                                                      */
      /*----------------------------------------------------------------*/

      do {
          set_custom_apn();
          ret = vm_https_register_context_and_callback(apn, &callbacks);

          if (ret != 0) {
              break;
          }

          /* set network profile information */
          ret = vm_https_set_channel(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
      } while (0);
    }
  else
    {
      VM_HTTPS_RESULT unused;
      vm_log_debug("calling send request directly 2nd time around");
      https_send_request_set_channel_rsp_cb(0, 0, unused);
    }
}
