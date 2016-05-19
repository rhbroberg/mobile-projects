#pragma once

#include <unordered_map>
#include <string>
#include "vmbt_cm.h"
#include <functional>

#include "gatt/UUIDBase.h"
#include "gatt/Service.h"

namespace gatt
{

class Server: public UUIDBase
{
public:
	Server(const VMUINT8 *hex, const char *name = NULL);

	void bindConnectionListener(std::function<void()> connect, std::function<void()> disconnect);
	void addService(Service *service);
	const bool enable();
	void changeName(const char *name);

protected:

	void clientActivity(const bool connected);
	void registerServices();
	Service *findService(const VMUINT8 *hex) const;
	Service *findService(const VM_BT_GATT_ATTRIBUTE_HANDLE key) const;

	Characteristic *findCharacteristic(const VM_BT_GATT_ATTRIBUTE_HANDLE key) const;
	const bool contextValid(const VM_BT_GATT_CONTEXT_HANDLE context) const;
	void setCallbacks();
	void start();

	// static void callbacks go here
	static void btcm_callback(VM_BT_CM_EVENT evt, void *param, void *user_data);

	static void register_server_callback(VM_BT_GATT_CONTEXT_HANDLE context_handle, VMBOOL status, VMUINT8 *app_uuid);
	static void service_added_callback(VMBOOL status, VM_BT_GATT_CONTEXT_HANDLE context_handle,
			vm_bt_gatt_service_info_t *srvc_id, VM_BT_GATT_SERVICE_HANDLE srvc_handle);
	static void characteristic_added_callback(VMBOOL status, VM_BT_GATT_CONTEXT_HANDLE context_handle,
			vm_bt_gatt_attribute_uuid_t *uuid, VM_BT_GATT_SERVICE_HANDLE srvc_handle,
			VM_BT_GATT_CHARACTERISTIC_HANDLE char_handle);
	static void descriptor_added_callback(VMBOOL status, VM_BT_GATT_CONTEXT_HANDLE context_handle,
			vm_bt_gatt_attribute_uuid_t *uuid, VM_BT_GATT_SERVICE_HANDLE srvc_handle,
			VM_BT_GATT_DESCRIPTOR_HANDLE descr_handle);
	static void service_started_callback(VMBOOL status, VM_BT_GATT_CONTEXT_HANDLE context_handle,
			VM_BT_GATT_SERVICE_HANDLE srvc_handle);
	static void listen_callback(VM_BT_GATT_CONTEXT_HANDLE context_handle, VMBOOL status);
	static void connection_callback(const vm_bt_gatt_connection_t *conn, VMBOOL connected,
			const vm_bt_gatt_address_t *bd_addr);
	static void request_read_callback(vm_bt_gatt_connection_t *conn, VMUINT16 trans_id, vm_bt_gatt_address_t *bd_addr,
			VM_BT_GATT_ATTRIBUTE_HANDLE attr_handle, VMUINT16 offset, VMBOOL is_long);
	static void request_write_callback(vm_bt_gatt_connection_t *conn, VMUINT16 trans_id, vm_bt_gatt_address_t *bd_addr,
			VM_BT_GATT_ATTRIBUTE_HANDLE attr_handle, vm_bt_gatt_attribute_value_t *value, VMUINT16 offset,
			VMBOOL need_rsp, VMBOOL is_prep);

	VMINT _handle;
	void *_context;
	vm_bt_gatt_server_callback_t _callbacks;
	std::unordered_map<std::string, Service *> _services;
	std::unordered_map<VM_BT_GATT_ATTRIBUTE_HANDLE, Service *> _byHandle;
	std::function <void()> _connect, _disconnect;
	static Server *_singleton;
};

}
