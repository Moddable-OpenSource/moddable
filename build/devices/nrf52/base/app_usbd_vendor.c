#include "ftdi_trace.h"
#include "sdk_common.h"

#include <string.h>
#include <ctype.h>
#include "app_usbd.h"
#include "app_usbd_vendor.h"
#include "app_usbd_string_desc.h"
#include "app_usbd_vendor_internal.h"

static ret_code_t vendor_req_out_datastage(app_usbd_class_inst_t const *p_inst,
									   app_usbd_setup_evt_t const *p_setup_ev);

static inline app_usbd_vendor_t const *vendor_get(app_usbd_class_inst_t const *p_inst) {
	ASSERT(p_inst != NULL);
	return (app_usbd_vendor_t const *)p_inst;
}

static inline app_usbd_vendor_ctx_t *vendor_ctx_get(app_usbd_vendor_t const *p_vendor) {
	ASSERT(p_vendor != NULL);
	ASSERT(p_vendor->specific.p_data != NULL);
	return &p_vendor->specific.p_data->ctx;
}

static ret_code_t setup_req_std_in(app_usbd_class_inst_t const *p_inst,
									app_usbd_setup_evt_t const *p_setup_ev) {
ftdiTrace("setup_req_std_in - bRequest");
ftdiTraceInt(p_setup_ev->setup.bRequest);
	if (((app_usbd_setup_req_rec(p_setup_ev->setup.bmRequestType) == APP_USBD_SETUP_REQREC_INTERFACE)
		&&
		(p_setup_ev->setup.bRequest == APP_USBD_SETUP_STDREQ_GET_DESCRIPTOR))) {
		size_t dsc_len = 0;
		size_t max_size;

		uint8_t * p_trans_buff = app_usbd_core_setup_transfer_buff_get(&max_size);

		// find descriptor in class internals
		ret_code_t ret = app_usbd_class_descriptor_find(
			p_inst,
			p_setup_ev->setup.wValue.hb,
			p_setup_ev->setup.wValue.lb,
			p_trans_buff,
			&dsc_len);

		if (ret != NRF_ERROR_NOT_FOUND) {
			ASSERT(dsc_len < NRF_DRV_USBD_EPSIZE);
			return app_usbd_core_setup_rsp(&(p_setup_ev->setup), p_trans_buff, dsc_len);
		}
		else ftdiTrace("ERROR NO DESCRIPTOR");
	}

ftdiTrace(" -- not supported\n");
	return NRF_ERROR_NOT_SUPPORTED;
}

static ret_code_t setup_req_std_out(app_usbd_class_inst_t const * p_inst,
									app_usbd_setup_evt_t const *  p_setup_ev) {
ftdiTrace(">- setup_req_std_out - setup.bRequest is:");
ftdiTraceInt(p_setup_ev->setup.bRequest);
	switch (p_setup_ev->setup.bRequest) {
		case VENDOR_REQUEST_LINE_STATE:
			if (p_setup_ev->setup.wLength.w != sizeof(app_usbd_vendor_line_coding_t)) {
				ftdiTrace("VENDOR LINE_STATE - bad size");
				return NRF_ERROR_NOT_SUPPORTED;
			}
			return vendor_req_out_datastage(p_inst, p_setup_ev);
		case VENDOR_SET_LINE_STATE:
			break;
		default:
ftdiTrace(">- need to do something here");
			break;
	}
	return NRF_ERROR_NOT_SUPPORTED;
}

static inline void user_event_handler(app_usbd_class_inst_t const *p_inst,
									  app_usbd_vendor_user_event_t event) {
	app_usbd_vendor_t const *p_vendor = vendor_get(p_inst);
	if (p_vendor->specific.inst.user_ev_handler != NULL) {
		p_vendor->specific.inst.user_ev_handler(p_inst, event);
	}
}

static ret_code_t setup_req_class_in(app_usbd_class_inst_t const *p_inst,
									 app_usbd_setup_evt_t const *p_setup_ev) {
	app_usbd_vendor_t const *p_vendor = vendor_get(p_inst);
	app_usbd_vendor_ctx_t *p_vendor_ctx = vendor_ctx_get(p_vendor);
ftdiTrace("setup_req_class_in");
ftdiTraceInt(p_setup_ev->setup.bRequest);
	switch (p_setup_ev->setup.bRequest) {
		case APP_USBD_VENDOR_REQ_GET_LINE_CODING:
			{
			if (p_setup_ev->setup.wLength.w != sizeof(app_usbd_vendor_line_coding_t)) {
ftdiTrace(" -- get_line_coding - size is bad\n");
				return NRF_ERROR_NOT_SUPPORTED;
			}
			return app_usbd_core_setup_rsp(&p_setup_ev->setup, &p_vendor_ctx->line_coding, sizeof(app_usbd_vendor_line_coding_t));
			}
		default:
			break;
	}

	return NRF_ERROR_NOT_SUPPORTED;
}

static ret_code_t vendor_req_out_data_cb(nrf_drv_usbd_ep_status_t status, void *p_context) {
	if (status != NRF_USBD_EP_OK) {
		return NRF_ERROR_INTERNAL;
	}
	app_usbd_vendor_t const *p_vendor = p_context;
	app_usbd_vendor_ctx_t *p_vendor_ctx = vendor_ctx_get(p_vendor);

	switch(p_vendor_ctx->request.type) {
		case APP_USBD_VENDOR_REQ_SET_LINE_CODING:
			memcpy(&p_vendor_ctx->line_coding,
				&p_vendor_ctx->request.payload.line_coding,
				sizeof(app_usbd_vendor_line_coding_t));
			break;
		default:
			return NRF_ERROR_NOT_SUPPORTED;
	}
	return NRF_SUCCESS;
}

static ret_code_t vendor_req_out_datastage(app_usbd_class_inst_t const *p_inst,
									   app_usbd_setup_evt_t const *p_setup_ev) {
	app_usbd_vendor_t const *p_vendor = vendor_get(p_inst);
	app_usbd_vendor_ctx_t *p_vendor_ctx = vendor_ctx_get(p_vendor);

ftdiTrace("req_out_datastage");
	p_vendor_ctx->request.type = p_setup_ev->setup.bRequest;
	p_vendor_ctx->request.len = p_setup_ev->setup.wLength.w;

	NRF_DRV_USBD_TRANSFER_OUT(transfer, &p_vendor_ctx->request.payload,
								p_vendor_ctx->request.len);

	ret_code_t ret;
	CRITICAL_REGION_ENTER();
	ret = app_usbd_ep_transfer(NRF_DRV_USBD_EPOUT0, &transfer);
	if (ret == NRF_SUCCESS) {
		const app_usbd_core_setup_data_handler_desc_t desc = {
			.handler = vendor_req_out_data_cb,
			.p_context = (void*)p_vendor
		};

		ret = app_usbd_core_setup_data_handler_set(NRF_DRV_USBD_EPOUT0, &desc);
	}
	CRITICAL_REGION_EXIT();
	return ret;
}

static ret_code_t setup_req_class_out(app_usbd_class_inst_t const *p_inst,
									  app_usbd_setup_evt_t const *p_setup_ev) {
	app_usbd_vendor_t const *p_vendor = vendor_get(p_inst);
	app_usbd_vendor_ctx_t *p_vendor_ctx = vendor_ctx_get(p_vendor);

	switch (p_setup_ev->setup.bRequest) {
		case APP_USBD_VENDOR_REQ_SET_LINE_CODING:
			if (p_setup_ev->setup.wLength.w != sizeof(app_usbd_vendor_line_coding_t))
				return NRF_ERROR_NOT_SUPPORTED;
			return vendor_req_out_datastage(p_inst, p_setup_ev);
			break;
		case APP_USBD_VENDOR_REQ_SET_CONTROL_LINE_STATE:
			if (p_setup_ev->setup.wLength.w != 0)
				return NRF_ERROR_NOT_SUPPORTED;
			uint8_t old_line_state = p_vendor_ctx->line_state;
			p_vendor_ctx->line_state = p_setup_ev->setup.wValue.w;
			uint8_t new_line_state = p_vendor_ctx->line_state;

			if (old_line_state == new_line_state)
				return NRF_SUCCESS;
			
			const app_usbd_vendor_user_event_t ev = VENDOR_REQUEST_LINE_STATE;

			user_event_handler(p_inst, ev);
			return NRF_SUCCESS;
	}

	return NRF_ERROR_NOT_SUPPORTED;
}

static ret_code_t setup_event_handler(app_usbd_class_inst_t const *p_inst,
									  app_usbd_setup_evt_t const *p_setup_ev) {
	ASSERT(p_inst != NULL);
	ASSERT(p_setup_ev != NULL);

//	ftdiTrace(" - vendor setup_event_handler (bmRequest)");
//	ftdiTraceInt(p_setup_ev->setup.bmRequestType);
	if (app_usbd_setup_req_dir(p_setup_ev->setup.bmRequestType) == APP_USBD_SETUP_REQDIR_IN) {
		switch (app_usbd_setup_req_typ(p_setup_ev->setup.bmRequestType)) {
			case APP_USBD_SETUP_REQTYPE_STD:
				return setup_req_std_in(p_inst, p_setup_ev);
			case APP_USBD_SETUP_REQTYPE_CLASS:
				return setup_req_class_in(p_inst, p_setup_ev);
			default:
				break;
		}
	}
	else {	/* APP_USBD_SETUP_REQDIR_OUT */
		switch (app_usbd_setup_req_typ(p_setup_ev->setup.bmRequestType)) {
			case APP_USBD_SETUP_REQTYPE_STD:
				return setup_req_std_out(p_inst, p_setup_ev);
			case APP_USBD_SETUP_REQTYPE_CLASS:
				return setup_req_class_out(p_inst, p_setup_ev);
			default:
	ftdiTrace(" - -- vendor out - do req_class_out");
				return setup_req_class_out(p_inst, p_setup_ev);
				break;
		}
	}

	return NRF_ERROR_NOT_SUPPORTED;
}

static void vendor_reset_port(app_usbd_class_inst_t const *p_inst) {
	app_usbd_vendor_t const *p_vendor = vendor_get(p_inst);
	app_usbd_vendor_ctx_t   *p_vendor_ctx = vendor_ctx_get(p_vendor);

	p_vendor_ctx->line_state = 0b11;
}

static ret_code_t vendor_event_handler(app_usbd_class_inst_t const *p_inst,
									   app_usbd_complex_evt_t const *p_event) {
	ASSERT(p_inst  != NULL);
	ASSERT(p_event != NULL);

	ret_code_t ret = NRF_SUCCESS;
	switch (p_event->app_evt.type) {
		case APP_USBD_EVT_DRV_SOF:		// 0
			break;
		case APP_USBD_EVT_DRV_RESET:	// 1
			vendor_reset_port(p_inst);
			break;
		case APP_USBD_EVT_DRV_SUSPEND:	// 2
			break;
		case APP_USBD_EVT_DRV_RESUME:	// 3
			break;
		case APP_USBD_EVT_DRV_WUREQ:	// 4
			break;
		case APP_USBD_EVT_DRV_SETUP:	// 5
			ret  = setup_event_handler(p_inst, (app_usbd_setup_evt_t const *) p_event);
			break;
		case APP_USBD_EVT_DRV_EPTRANSFER:	// 6
			break;
		case APP_USBD_EVT_POWER_DETECTED:	// 8
			break;
		case APP_USBD_EVT_POWER_REMOVED:	// 9
			vendor_reset_port(p_inst);
			break;
		case APP_USBD_EVT_POWER_READY:		// 10
			break;
		case APP_USBD_EVT_INST_APPEND:		// 11
			break;
		case APP_USBD_EVT_INST_REMOVE:
			break;
		case APP_USBD_EVT_STARTED:			// 13
			break;
		case APP_USBD_EVT_STOPPED:
			break;
		case APP_USBD_EVT_STATE_CHANGED:	// 15
			break;
		default:
ftdiTrace("vendor NOT_SUPPORTED ?");
			ret = NRF_ERROR_NOT_SUPPORTED;
			break;
	}

	return ret;
}

static bool vendor_feed_descriptors(app_usbd_class_descriptor_ctx_t * p_ctx,
									app_usbd_class_inst_t const* p_inst,
									uint8_t* p_buff,
									size_t max_size) {
	static uint8_t ifaces = 0;
	ifaces = app_usbd_class_iface_count_get(p_inst);
	app_usbd_vendor_t const *p_vendor = vendor_get(p_inst);

//ftdiTrace("vendor_feed_Descirptors");

    APP_USBD_CLASS_DESCRIPTOR_BEGIN(p_ctx, p_buff, max_size)

    /* INTERFACE ASSOCIATION DESCRIPTOR comm */
	APP_USBD_CLASS_DESCRIPTOR_WRITE(0x08);			// length
	APP_USBD_CLASS_DESCRIPTOR_WRITE(APP_USBD_DESCRIPTOR_INTERFACE_ASSOCIATION);
	APP_USBD_CLASS_DESCRIPTOR_WRITE(p_vendor->specific.inst.comm_interface);
	APP_USBD_CLASS_DESCRIPTOR_WRITE(0x02);			// bInterfaceCount
	APP_USBD_CLASS_DESCRIPTOR_WRITE(APP_USBD_VENDOR_CLASS);
	APP_USBD_CLASS_DESCRIPTOR_WRITE(APP_USBD_VENDOR_SUBCLASS);
	APP_USBD_CLASS_DESCRIPTOR_WRITE(APP_USBD_VENDOR_PROTOCOL_RUNTIME);
	APP_USBD_CLASS_DESCRIPTOR_WRITE(0x00);		// iFunction

	static uint8_t i = 0;
	for (i=0; i<ifaces; i++) {
	    /* INTERFACE DESCRIPTOR */
		APP_USBD_CLASS_DESCRIPTOR_WRITE(sizeof(app_usbd_descriptor_iface_t)); // bLength
		APP_USBD_CLASS_DESCRIPTOR_WRITE(APP_USBD_DESCRIPTOR_INTERFACE); // bDescriptorType

		static app_usbd_class_iface_conf_t const *p_cur_iface = NULL;
		p_cur_iface = app_usbd_class_iface_get(p_inst, i);

		APP_USBD_CLASS_DESCRIPTOR_WRITE(app_usbd_class_iface_number_get(p_cur_iface)); // bInterfaceNumber
		APP_USBD_CLASS_DESCRIPTOR_WRITE(0x00); // bAlternateSetting
		APP_USBD_CLASS_DESCRIPTOR_WRITE(app_usbd_class_iface_ep_count_get(p_cur_iface)); // bNumEndpoints

		if (p_vendor->specific.inst.comm_interface == app_usbd_class_iface_number_get(p_cur_iface)) {
			APP_USBD_CLASS_DESCRIPTOR_WRITE(APP_USBD_VENDOR_CLASS); // bInterfaceClass
			APP_USBD_CLASS_DESCRIPTOR_WRITE(APP_USBD_VENDOR_COMM_SUBCLASS); // bInterfaceSubClass
			APP_USBD_CLASS_DESCRIPTOR_WRITE(p_vendor->specific.inst.protocol); // bProtocol
		}
		else if (p_vendor->specific.inst.data_interface == app_usbd_class_iface_number_get(p_cur_iface)) {
			APP_USBD_CLASS_DESCRIPTOR_WRITE(APP_USBD_VENDOR_CLASS); // bInterfaceClass
			APP_USBD_CLASS_DESCRIPTOR_WRITE(APP_USBD_VENDOR_DATA_SUBCLASS); // bInterfaceSubClass
			APP_USBD_CLASS_DESCRIPTOR_WRITE(0x00); // bProtocol
		}
		else {
			ASSERT(0);
		}

		APP_USBD_CLASS_DESCRIPTOR_WRITE(0x00); // iInterface
#if 0
		if (p_vendor->specific.inst.comm_interface == app_usbd_class_iface_number_get(p_cur_iface)) {
			/* header */
			APP_USBD_CLASS_DESCRIPTOR_WRITE(0x05);		//bLength
			APP_USBD_CLASS_DESCRIPTOR_WRITE(APP_USBD_VENDOR_CS_INTERFACE);
			APP_USBD_CLASS_DESCRIPTOR_WRITE(APP_USBD_VENDOR_SCS_HEADER);
			APP_USBD_CLASS_DESCRIPTOR_WRITE(0x10);		// bcdCDC LSB
			APP_USBD_CLASS_DESCRIPTOR_WRITE(0x01);		// bcdCDC MSB

			/* CALL MANAGEMENT DESCRIPTOR */
			APP_USBD_CLASS_DESCRIPTOR_WRITE(0x05); // bLength
			APP_USBD_CLASS_DESCRIPTOR_WRITE(APP_USBD_VENDOR_CS_INTERFACE); // bDescriptorType = Class Specific Interface
			APP_USBD_CLASS_DESCRIPTOR_WRITE(APP_USBD_VENDOR_SCS_CALL_MGMT); // bDescriptorSubtype = Call Management Functional Descriptor
			APP_USBD_CLASS_DESCRIPTOR_WRITE(0x03); // bmCapabilities
			APP_USBD_CLASS_DESCRIPTOR_WRITE(p_vendor->specific.inst.data_interface); // bDataInterface

			/* ABSTRACT CONTROL MANAGEMENT DESCRIPTOR */
			APP_USBD_CLASS_DESCRIPTOR_WRITE(0x04); // bLength
			APP_USBD_CLASS_DESCRIPTOR_WRITE(APP_USBD_VENDOR_CS_INTERFACE); // bDescriptorType = Class Specific Interface
			APP_USBD_CLASS_DESCRIPTOR_WRITE(APP_USBD_VENDOR_SCS_ACM); // bDescriptorSubtype = Abstract Control Management Functional Descriptor
			APP_USBD_CLASS_DESCRIPTOR_WRITE(0x02); // bmCapabilities

			/* UNION DESCRIPTOR */
			APP_USBD_CLASS_DESCRIPTOR_WRITE(0x05);
			APP_USBD_CLASS_DESCRIPTOR_WRITE(APP_USBD_VENDOR_CS_INTERFACE); // bDescriptorType = Class Specific Interface
			APP_USBD_CLASS_DESCRIPTOR_WRITE(APP_USBD_VENDOR_SCS_UNION); // bDescriptorSubtype = Union Functional Descriptor
			APP_USBD_CLASS_DESCRIPTOR_WRITE(p_vendor->specific.inst.comm_interface); // bControlInterface
			APP_USBD_CLASS_DESCRIPTOR_WRITE(p_vendor->specific.inst.data_interface); // bSubordinateInterface
		}
		else if (p_vendor->specific.inst.data_interface == app_usbd_class_iface_number_get(p_cur_iface)) {
			;
		}
		else {
			ASSERT(0);
		}
#endif

		/* ENDPOINT DESCRIPTORS */
		static uint8_t endpoints=0, j=0;
		endpoints = app_usbd_class_iface_ep_count_get(p_cur_iface);

		for (j=0; j<endpoints; j++) {
			APP_USBD_CLASS_DESCRIPTOR_WRITE(0x07);		// Length
			APP_USBD_CLASS_DESCRIPTOR_WRITE(APP_USBD_DESCRIPTOR_ENDPOINT);	// bDescriptorType = Endpoint

			static app_usbd_class_ep_conf_t const *p_cur_ep = NULL;
			p_cur_ep = app_usbd_class_iface_ep_get(p_cur_iface, j);
			APP_USBD_CLASS_DESCRIPTOR_WRITE(app_usbd_class_ep_address_get(p_cur_ep));	// bEndpointAddress

			if (p_vendor->specific.inst.comm_interface == app_usbd_class_iface_number_get(p_cur_iface)) {
				APP_USBD_CLASS_DESCRIPTOR_WRITE(APP_USBD_DESCRIPTOR_EP_ATTR_TYPE_INTERRUPT);		// bmAttributes
			}
			else if (p_vendor->specific.inst.data_interface == app_usbd_class_iface_number_get(p_cur_iface)) {
				APP_USBD_CLASS_DESCRIPTOR_WRITE(APP_USBD_DESCRIPTOR_EP_ATTR_TYPE_BULK);		// bmAttributes
			}
			else {
				ASSERT(0);
			}
			
			APP_USBD_CLASS_DESCRIPTOR_WRITE(LSB_16(NRF_DRV_USBD_EPSIZE));	// wMaxPacketSize LSB
			APP_USBD_CLASS_DESCRIPTOR_WRITE(MSB_16(NRF_DRV_USBD_EPSIZE));	// wMaxPacketSize MSB

			if (p_vendor->specific.inst.comm_interface == app_usbd_class_iface_number_get(p_cur_iface)) {
				APP_USBD_CLASS_DESCRIPTOR_WRITE(p_vendor->specific.inst.p_ep_interval[0]);	// bInterval
			}
			else if (p_vendor->specific.inst.data_interface == app_usbd_class_iface_number_get(p_cur_iface)) {
				APP_USBD_CLASS_DESCRIPTOR_WRITE(0x00);		// bInterval
			}
			else {
				ASSERT(0);
			}
		}
	}

    APP_USBD_CLASS_DESCRIPTOR_END();
}

const app_usbd_class_methods_t app_usbd_vendor_class_methods = {
        .event_handler    = vendor_event_handler,
        .feed_descriptors = vendor_feed_descriptors,
};

