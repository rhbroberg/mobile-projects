#include "Uart.h"
#include "vmdcl.h"
#include "vmlog.h"
#include "vmboard.h"
#include "vmdcl_sio.h"
#include "vmsystem.h"
#include "string.h"
#include "SysWrapper.h"

Uart *Uart::_instance = NULL;

Uart::Uart()
 : _uart(VM_DCL_HANDLE_INVALID)
{
	_instance = this;
}

void
Uart::init()
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

		vm_log_info("registering callback for device %x", _uart);
#ifdef NOTYET
		sysWrapper.dcl_control(_uart, VM_DCL_SIO_COMMAND_SET_DCB_CONFIG, (void *) &config);
		sysWrapper.dcl_register_callback(_uart, VM_DCL_SIO_UART_READY_TO_READ, (vm_dcl_callback) Uart::triggered, NULL);
#else
		vm_dcl_control(_uart, VM_DCL_SIO_COMMAND_SET_DCB_CONFIG, (void *) &config);
		vm_dcl_register_callback(_uart, VM_DCL_SIO_UART_READY_TO_READ, (vm_dcl_callback) Uart::triggered, NULL);
#endif
	}
}

const bool
Uart::read(char *buf, const unsigned long len)
{
    VM_DCL_STATUS status;
    VM_DCL_BUFFER_LENGTH sent;
    memset(buf, 0, len);

    status = vm_dcl_read(_uart, (VM_DCL_BUFFER *) buf, len - 1, &sent, vm_dcl_get_owner_id());
    // vm_log_info("status: %d; read %d bytes: '%s'", status, sent, buf);

    return sent == (len - 1);
}

void
Uart::write(const char *command)
{
    if (_uart != -1)
    {
        VMINT count = 0;
        VM_DCL_STATUS status;
        VM_DCL_BUFFER_LENGTH written = 0;

        status = vm_dcl_write(_uart, (VM_DCL_BUFFER*) command, strlen(command), &written, vm_dcl_get_owner_id());

        /* continue to write data if write fails */
        while ((status < VM_DCL_STATUS_OK || written != strlen(command)) && (count < 3))
        {
            vm_log_debug("retrying write: %d/%d: %d", written, strlen(command), ++count);
            status = vm_dcl_write(_uart, (VM_DCL_BUFFER*) command, strlen(command), &written, vm_dcl_get_owner_id());
        }
        vm_log_debug("sent '%s' to gps [%d]", command, written);
    }
}

void
Uart::setHook(std::function<void(void)> hook)
{
	_interruptHook = hook;
}

/* static */
void
Uart::triggered(void *user_data, VM_DCL_EVENT event, VM_DCL_HANDLE device_handle)
{
	// vm_log_info("interrupt: device = %x, event is %d", device_handle, event);

    // call lambda if defined
    if ((_instance->_interruptHook) && (event == VM_DCL_SIO_UART_READY_TO_READ))
    {
    	_instance->_interruptHook();
    }
    else
    {
    	vm_log_info("unknown event %d for handle %d", event, device_handle);
    }
}
