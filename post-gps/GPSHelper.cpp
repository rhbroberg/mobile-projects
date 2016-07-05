#include "GPSHelper.h"
#include "LGPS.h"
#include "vmlog.h"
#include "vmdatetime.h"
#include "vmsystem.h"
#include "vmlog.h"
#include "vmdcl.h"
#include "vmboard.h"

GPSHelper *This = NULL;

GPSHelper::GPSHelper()
  :	TimedTask("GPS")
  , _isSet(false)
  , _uart(VM_DCL_HANDLE_INVALID)
{
	This = this;
}

const bool
GPSHelper::updateRTC()
{
	_isSet = true;
	if (!_isSet)
	{
		if (LGPS.check_online() && (LGPS.get_position_fix() != '0'))
		{
			const unsigned char *utc_date_time = LGPS.get_utc_date_time();
			vm_date_time_t t;
			vm_log_info("setting time to GPS value");

			t.year = 2000 + utc_date_time[0];
			t.month = utc_date_time[1];
			t.day = utc_date_time[2];
			t.hour = utc_date_time[3];
			t.minute = utc_date_time[4];
			t.second = utc_date_time[5];

			VM_RESULT r = vm_time_set_date_time(&t);
			if (r < 0)
			{
				vm_log_info("setting time failed: %d", r);
			}
			else
			{
				_isSet = true;
			}
		}
	}
	return _isSet;
}

const bool
GPSHelper::createLocationMsg(const char *format, VMSTR message, const int rxLevel)
{
	bool result;

	if (result = sample())
	{
		sprintf((char *)message,
				format, // LGPS.get_status(),
				(LGPS.get_ns() == 'S') ? "-" : "", LGPS.get_latitude(),
				(LGPS.get_ew() == 'W') ? "-" : "", LGPS.get_longitude(),
				LGPS.get_altitude(), LGPS.get_course(), LGPS.get_speed(),
				LGPS.get_position_fix(), LGPS.get_sate_used(), rxLevel);
	}

	return result;
}

void
GPSHelper::write(const char *command)
{
    if (_uart != -1)
    {
        VMINT count = 0;
        VM_DCL_STATUS status;
        VM_DCL_BUFFER_LENGTH written = 0;

        /* write data */
        status = vm_dcl_write(_uart, (VM_DCL_BUFFER*) command, strlen(command), &written, vm_dcl_get_owner_id());

        /* continue to write data if write fails */
        while ((status < VM_DCL_STATUS_OK || written != strlen(command)) && (count < 3))
        {
            count++;
            status = vm_dcl_write(_uart, (VM_DCL_BUFFER*) command, strlen(command), &written, vm_dcl_get_owner_id());
        }
        vm_log_debug((char*)"write length = %d", written);
    }
}

const bool
GPSHelper::sample()
{
	unsigned char *utc_date_time = 0;
	bool status;

#ifdef NOPE
	if (status = LGPS.check_online())
	{
		utc_date_time = LGPS.get_utc_date_time();
		vm_log_info("GPS status/position fix/count/mode/mode2 is %c/%c/%d/%c/%c", LGPS.get_status(), LGPS.get_position_fix(), LGPS.get_sate_used(), LGPS.get_mode(), LGPS.get_mode2());
		vm_log_info("GPS UTC:%02d-%02d-%02d %02d:%02d:%02d", utc_date_time[0], utc_date_time[1], utc_date_time[2], utc_date_time[3], utc_date_time[4], utc_date_time[5]);
		vm_log_info("GPS lat/long is %c:%f/%c:%f", LGPS.get_ns(), LGPS.get_latitude(), LGPS.get_ew(), LGPS.get_longitude());
		vm_log_info("GPS speed/course/alt is %f/%f/%f", LGPS.get_speed(), LGPS.get_course(), LGPS.get_altitude());

		updateRTC();
	}
	else
	{
		vm_log_info("gps not online yet");
	}
	return status ? (LGPS.get_sate_used() > 0) : false; // ridiculous check, but this gps library doesn't let me query if data is valid
#endif
	return false;
}

const char *
GPSHelper::read()
{
    static VMCHAR data[100];
    VM_DCL_STATUS status;
    VM_DCL_BUFFER_LENGTH returned_len;
    /* read data into buffer */
    memset(data, 0, sizeof(data));

    status = vm_dcl_read(_uart, (VM_DCL_BUFFER *) data, sizeof(data) - 1, &returned_len, vm_dcl_get_owner_id());
    //	vm_log_info("status: %d; read %d bytes: '%s'", status, returned_len, data);
	while (returned_len == (sizeof(data) - 1))
	{
		vm_log_info("buffer overrun?");
	    status = vm_dcl_read(_uart, (VM_DCL_BUFFER *) data, sizeof(data) - 1, &returned_len, vm_dcl_get_owner_id());
		vm_log_info("again - status: %d; read %d bytes: '%s'", status, returned_len, data);
	}
    return (const char *) data;
}

// extend InterruptMapper to do this instead
void
uart_irq_handler(void *parameter, VM_DCL_EVENT event, VM_DCL_HANDLE device_handle)
{
	vm_dcl_callback_data_t *foo = (vm_dcl_callback_data_t *) parameter;
	vm_log_info("device handle is %x, p - %x, t - %x, %x, %x", device_handle, parameter, This,
			foo->local_parameters, foo->peer_buffer);

	if (event == VM_DCL_SIO_UART_READY_TO_READ)
    {
		This->wakeup();
    }
}

#include "vmboard.h"

void
GPSHelper::initUART()
{
	// move this block to start()?
	// pins gpio 10 and 11 on breakout board must be in UART role
	vm_dcl_config_pin_mode(VM_PIN_P8, VM_DCL_PIN_MODE_UART);
	vm_dcl_config_pin_mode(VM_PIN_P9, VM_DCL_PIN_MODE_UART);

	if (_uart == VM_DCL_HANDLE_INVALID)
	{
		vm_log_info("opening serial port 1");
		_uart = vm_dcl_open(VM_DCL_SIO_UART_PORT1, vm_dcl_get_owner_id());
	}
	// move this block to start()?

	if (_uart != VM_DCL_HANDLE_INVALID)
	{
		vm_dcl_sio_control_dcb_t config;

		vm_log_info("configuring serial port");
		config.owner_id = vm_dcl_get_owner_id();
		config.config.dsr_check = 0;
		config.config.baud_rate = VM_DCL_SIO_UART_BAUDRATE_9600;
		config.config.data_bits_per_char_length = VM_DCL_SIO_UART_BITS_PER_CHAR_LENGTH_8;
		config.config.parity = VM_DCL_SIO_UART_PARITY_NONE;
		config.config.stop_bits = VM_DCL_SIO_UART_STOP_BITS_1;
		config.config.flow_control = VM_DCL_SIO_UART_FLOW_CONTROL_NONE;
		config.config.sw_xoff_char = 0x13;
		config.config.sw_xon_char = 0x11;

		vm_dcl_control(_uart, VM_DCL_SIO_COMMAND_SET_DCB_CONFIG, (void *) &config);
		vm_dcl_register_callback(_uart, VM_DCL_SIO_UART_READY_TO_READ, (vm_dcl_callback) uart_irq_handler, (void *) this);
	}
}

const bool
GPSHelper::setup()
{
	initUART();
}

void
GPSHelper::loop()
{
	const char *str = read();
	vm_log_info("loop read %d bytes: '%s'", strlen(str), str);
}

void
GPSHelper::pauseHook()
{

}

void
GPSHelper::resumeHook()
{

}
