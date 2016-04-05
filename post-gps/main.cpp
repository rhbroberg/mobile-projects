/*
 This sample code is in public domain.

 This sample code is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

/* 
 This sample connects to HTTP(no secure) to retrieve index.html from labs.mediatek.com and print to vm_log.

 It calls the API vm_https_register_context_and_callback() to register the callback functions,
 then set the channel by vm_https_set_channel(), after the channel is established,
 it will send out the request by vm_https_send_request() and read the response by
 vm_https_read_content().

 You can change the url by modify macro VMHTTPS_TEST_URL.
 Before run this example, please set the APN information first by modify macros.
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
#include "vmstdlib.h"


#include "ResID.h"
#include "main.h"
#include "vmhttps.h"
#include "vmtimer.h"
#include "vmgsm_gprs.h"

#include "vmpwr.h"
#include "LGPS.h"
#include "LEDBlinker.h"
#include "LBattery.h"
LEDBlinker myBlinker;

void
showBatteryStats()
{
  int level = vm_pwr_get_battery_level();
  VMBOOL charging = vm_pwr_is_charging();

  vm_log_debug("battery level is %d, charging is %d\n", level, charging);
  if (charging)
    {
      myBlinker.change(LEDBlinker::color(LEDBlinker::green), 300);
    }
  else
    {
      myBlinker.change(LEDBlinker::color(LEDBlinker::green), 100, 200, 3);
    }
}

void getGPS()
{
  unsigned char *utc_date_time = 0;
  VMCHAR buffer[50] = {0,};

  if(LGPS.check_online())
    {
      myBlinker.change(LEDBlinker::color(LEDBlinker::red), 100);
      utc_date_time = LGPS.get_utc_date_time();
      sprintf(buffer, (VMCSTR) "GPS UTC:%d-%d-%d  %d:%d:%d\r\n", utc_date_time[0], utc_date_time[1], utc_date_time[2], utc_date_time[3], utc_date_time[4],utc_date_time[5]);
      vm_log_info((const char *)buffer);

      sprintf(buffer, (VMCSTR) "GPS status is %c\r\n", LGPS.get_status());
      vm_log_info((const char *)buffer);

      sprintf(buffer, (VMCSTR) "GPS latitude is %c:%f\r\n", LGPS.get_ns(), LGPS.get_latitude());
      vm_log_info((const char *)buffer);

      sprintf(buffer, (VMCSTR) "GPS longitude is %c:%f\r\n", LGPS.get_ew(), LGPS.get_longitude());
      vm_log_info((const char *)buffer);

      sprintf(buffer, (VMCSTR) "GPS speed is %f\r\n", LGPS.get_speed());
      vm_log_info((const char *)buffer);

      sprintf(buffer, (VMCSTR) "GPS course is %f\r\n", LGPS.get_course());
      vm_log_info((const char *)buffer);

      sprintf(buffer, (VMCSTR) "GPS position fix is %c\r\n", LGPS.get_position_fix());
      vm_log_info((const char *)buffer);

      sprintf(buffer, (VMCSTR) "GPS sate used is %d\r\n", LGPS.get_sate_used());
      vm_log_info((const char *)buffer);

      sprintf(buffer, (VMCSTR) "GPS altitude is %f\r\n", LGPS.get_altitude());
      vm_log_info((const char *)buffer);

      sprintf(buffer, (VMCSTR) "GPS mode is %c\r\n", LGPS.get_mode());
      vm_log_info((const char *)buffer);

      sprintf(buffer, (VMCSTR) "GPS mode2 is %c\r\n", LGPS.get_mode2());
      vm_log_info((const char *)buffer);
    }
  else
    {
      vm_log_info("gps not online yet");
    }
}


#include "vmgsm_cell.h"
#include "vmgsm_gprs.h"
#include "vmgsm_sim.h"
#include "vmgsm_sms.h"

vm_gsm_cell_info_t g_info; /* cell information data */

const int
simStatus()
{
  VM_GSM_SIM_STATUS status;
  VMSTR imsi=NULL;
  VMSTR imei=NULL;

  /* Opens the cell when receiving AT command: AT+[1000]Test01 */
  VMINT result = vm_gsm_cell_open();
  vm_log_info("open result = %d",result);
  VMBOOL has = vm_gsm_sim_has_card();
  VM_GSM_SIM_ID id = vm_gsm_sim_get_active_sim_card();
  imsi = (VMSTR)vm_gsm_sim_get_imsi(id);
  imei = (VMSTR)vm_gsm_sim_get_imei(id);
  status = vm_gsm_sim_get_card_status(id);
  VMBOOL smsReady = vm_gsm_sms_is_sms_ready();
  vm_log_info("active sim id = %d, sms is ready %d\n", id, smsReady);

  vm_gsm_cell_get_current_cell_info(&g_info);
  vm_log_info("ar=%d, bsic=%d, rxlev=%d, mcc=%d, mnc=%d, lac=%d, ci=%d", g_info.arfcn, g_info.bsic, g_info.rxlev, g_info.mcc, g_info.mnc, g_info.lac, g_info.ci);

  if ((has == VM_TRUE) || true)
    {
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
  return g_info.rxlev;
}

#define VMHTTPS_TEST_URL "http://io.adafruit.com/api/groups/tracker/send.none?x-aio-key=b8929d313c50fe513da199b960043b344e2b3f1f&&lat=%s%f&long=%s%f&alt=%f&course=%f&speed=%f&fix=%c&satellites=%d&rxl=%d"
unsigned char  mqttDoit(void *);
void initTCP();
void https_send_request();

static void
myHttpSend(VM_TIMER_ID_NON_PRECISE timer_id, void *user_data)
{
  extern VMCHAR myUrl[1024];
  extern int firstSend;

  int rxl = simStatus();
  getGPS();
  sprintf(myUrl, (VMCSTR) VMHTTPS_TEST_URL, // LGPS.get_status(),
      (LGPS.get_ns() == 'S')?"-":"", LGPS.get_latitude(),
          (LGPS.get_ew() == 'W')? "-": "", LGPS.get_longitude(),
              LGPS.get_altitude(),
              LGPS.get_course(), LGPS.get_speed(), LGPS.get_position_fix(), LGPS.get_sate_used(),
              rxl);
  https_send_request();

  if (firstSend)
    {
      vm_timer_delete_non_precise(timer_id);
      vm_timer_create_non_precise(45000, myHttpSend, NULL);
    }
  else
    {
      vm_timer_delete_non_precise(timer_id);
      vm_timer_create_non_precise(10000, myHttpSend, NULL);
    }
}

static void mqttInit(VM_TIMER_ID_NON_PRECISE timer_id,
    void* user_data)
{
  vm_timer_delete_non_precise(timer_id);
  vm_log_debug("trying tcp pathway");
  myBlinker.change(LEDBlinker::green, 300, 300, 10);
  initTCP();
  myBlinker.change(LEDBlinker::red, 100, 500, 32);
}

static void ledtest(VM_TIMER_ID_NON_PRECISE timer_id, void *user_data)
{
  vm_log_debug("ledtest");
  myBlinker.change(LEDBlinker::green, 300, 300, 10);
  delay(10000);
  vm_log_debug("another test");
  myBlinker.change(LEDBlinker::blue, 100, 500, 10);
}

void handle_sysevt(VMINT message, VMINT param) {
  // no gsm until VM_EVENT_CELL_INFO_CHANGE received
  // shutdown GPS when VM_EVENT_CELL_INFO_CHANGE
  // battery notice on VM_EVENT_LOW_BATTERY
  vm_log_info("handle_sysevt received %d", message);
  switch (message) {
  case VM_EVENT_CREATE:
#define USE_HTTP
#ifdef USE_HTTP
    myBlinker.change(LEDBlinker::green, 3000, 3000, 1024);
    //	  vm_timer_create_non_precise(60000, ledtest, NULL);
    vm_timer_create_non_precise(60000, myHttpSend, NULL);

#else
    vm_timer_create_non_precise(1000, mqttInit, NULL);
#endif
    myBlinker.start();
    break;

  case VM_EVENT_QUIT:
    break;
  }
}

extern "C"
{
  void vm_main(void)
  {
    // maybe want to explicitly reset the gsm radio, since sometimes only a full power cycle seems to fix it?
    showBatteryStats();
    vm_pmng_register_system_event_callback(handle_sysevt);
  }
}

