#include "InterruptMapper.h"
#include <string.h>
#include "vmtype.h"
#include "vmlog.h"
#include "vmboard.h"
#include "vmdcl_eint.h"
#include "vmdcl_gpio.h"

using namespace gpstracker;

std::map<VM_DCL_HANDLE, InterruptMapper *> InterruptMapper::_selfByDevice;

InterruptMapper::InterruptMapper(const unsigned int pin, const bool direction, const unsigned int debounce, const bool sensitivity, const bool polarity)
: _pin(pin)
, _direction(direction)
, _debounceTime(debounce)
, _sensitivity(sensitivity)
, _autoPolarity(polarity)
{
}

void
InterruptMapper::setHook(std::function<void(const unsigned int, const bool)> hook)
{
	_interruptHook = hook;
}

const bool
InterruptMapper::level()
{
	vm_dcl_gpio_control_level_status_t gpio_input_data;
	VM_DCL_HANDLE gpio_handle = vm_dcl_open(VM_DCL_GPIO, _pin);
	vm_dcl_control(gpio_handle, VM_DCL_GPIO_COMMAND_READ, (void *) &gpio_input_data);

    vm_dcl_close(gpio_handle);
    return (gpio_input_data.level_status == VM_DCL_GPIO_IO_HIGH) ? true : false;
}

void
InterruptMapper::autoPolarity(const bool polarity)
{
	vm_dcl_eint_control_auto_change_polarity_t autome;
	autome.auto_change_polarity = polarity ? 1 : 0;

	VM_DCL_STATUS status = vm_dcl_control(_pinHandle, VM_DCL_EINT_COMMAND_SET_AUTO_CHANGE_POLARITY, (void*) &autome);
	if (status != VM_DCL_STATUS_OK)
	{
		vm_log_info("VM_DCL_EINT_COMMAND_SET_AUTO_CHANGE_POLARITY = %d", status);
	}
}

void
InterruptMapper::enable()
{
	vm_dcl_eint_control_config_t eint_config;
	vm_dcl_eint_control_sensitivity_t sens_data;
	vm_dcl_eint_control_hw_debounce_t debounce_time;
	VM_DCL_STATUS status;

	/* Resets the data structures */
	memset(&eint_config, 0, sizeof(vm_dcl_eint_control_config_t));
	memset(&sens_data, 0, sizeof(vm_dcl_eint_control_sensitivity_t));
	memset(&debounce_time, 0, sizeof(vm_dcl_eint_control_hw_debounce_t));

	vm_dcl_config_pin_mode(_pin, VM_DCL_PIN_MODE_EINT); /* Sets the pin _pin to EINT mode */

	/* Opens and attaches _pin EINT */
	_pinHandle = vm_dcl_open(VM_DCL_EINT, PIN2EINT(_pin));
	_selfByDevice[_pinHandle] = this;

	if (VM_DCL_HANDLE_INVALID == _pinHandle)
	{
		vm_log_info("open EINT error");
		return;
	}

	/* Usually, before configuring the EINT, we mask it firstly. */
	status = vm_dcl_control(_pinHandle, VM_DCL_EINT_COMMAND_MASK, NULL);
	if (status != VM_DCL_STATUS_OK)
	{
		vm_log_info("VM_DCL_EINT_COMMAND_MASK  = %d", status);
	}

	/* Registers the EINT callback */
	vm_log_info("device handle is %x", _pinHandle);

	status = vm_dcl_register_callback(_pinHandle, VM_DCL_EINT_EVENT_TRIGGER, InterruptMapper::triggered, NULL);
	if (status != VM_DCL_STATUS_OK)
	{
		vm_log_info("VM_DCL_EINT_EVENT_TRIGGER = %d", status);
	}

	/* Configures a FALLING edge to trigger */
	sens_data.sensitivity = _sensitivity ? 1 : 0;
	eint_config.act_polarity = _direction ? 1 : 0;

	autoPolarity(_autoPolarity);
	/* Sets the auto unmask for the EINT */
	eint_config.auto_unmask = 1;

	/* Sets the EINT sensitivity */
	status = vm_dcl_control(_pinHandle, VM_DCL_EINT_COMMAND_SET_SENSITIVITY, (void*) &sens_data);
	if (status != VM_DCL_STATUS_OK)
	{
		vm_log_info("VM_DCL_EINT_COMMAND_SET_SENSITIVITY = %d", status);
	}

	/* Sets debounce time to 1ms */
	debounce_time.debounce_time = _debounceTime;
	/* Sets debounce time */
	status = vm_dcl_control(_pinHandle, VM_DCL_EINT_COMMAND_SET_HW_DEBOUNCE, (void*) &debounce_time);
	if (status != VM_DCL_STATUS_OK)
	{
		vm_log_info("VM_DCL_EINT_COMMAND_SET_HW_DEBOUNCE = %d", status);
	}

	/* Usually, before configuring the EINT, we mask it firstly. */
	status = vm_dcl_control(_pinHandle, VM_DCL_EINT_COMMAND_MASK, NULL);
	if (status != VM_DCL_STATUS_OK)
	{
		vm_log_info("VM_DCL_EINT_COMMAND_MASK  = %d", status);
	}

	/* 1 means enabling the HW debounce; 0 means disabling. */
	eint_config.debounce_enable = (_debounceTime > 0) ? 1 : 0;

	/* Make sure to call this API at the end as the EINT will be unmasked in this statement. */
	status = vm_dcl_control(_pinHandle, VM_DCL_EINT_COMMAND_CONFIG, (void*) &eint_config);
	if (status != VM_DCL_STATUS_OK)
	{
		vm_log_info("VM_DCL_EINT_COMMAND_CONFIG = %d", status);
	}
//     vm_dcl_close(gpio_handle); ?
}

void
InterruptMapper::disable()
{

}

const unsigned short
InterruptMapper::pin() const
{
	return _pin;
}

/* static */
void
InterruptMapper::triggered(void *user_data, VM_DCL_EVENT event, VM_DCL_HANDLE device_handle)
{
    vm_log_info("interrupt: device = %x", device_handle);

    // no lambda-style callback works here; vm_dcl_register_callback() firmware is broken (as of 2016-06-08) and does
    // not allow passing of 'void * user_data' like all other callbacks in system.  Use a static map in a static method
    // to lookup the object associated with the device for method invocation
    auto search = _selfByDevice.find(device_handle);
	if (search != _selfByDevice.end())
	{
		InterruptMapper *me = search->second;
		vm_log_info("pin level is %d", me->level());

		// call lambda here
		if (me->_interruptHook)
		{
			me->_interruptHook(me->pin(), me->level());
		}
	}
}
