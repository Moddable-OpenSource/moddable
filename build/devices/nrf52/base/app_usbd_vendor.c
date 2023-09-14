#include "ftdi_trace.h"
#include "sdk_common.h"

#include <string.h>
#include <ctype.h>
#include "app_usbd.h"
#include "app_usbd_vendor.h"
#include "app_usbd_string_desc.h"
#include "app_usbd_vendor_internal.h"

#define APP_USBD_VENDOR_COMM_IFACE_IDX 0
#define APP_USBD_VENDOR_DATA_IFACE_IDX 1

#define APP_USBD_VENDOR_COMM_EPIN_IDX 0
#define APP_USBD_VENDOR_DATA_EPIN_IDX 0
#define APP_USBD_VENDOR_DATA_EPOUT_IDX 1

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

static inline nrf_drv_usbd_ep_t vendor_comm_ep_in_addr_get(app_usbd_class_inst_t const *p_inst) {
	app_usbd_class_iface_conf_t const *class_iface;
	class_iface = app_usbd_class_iface_get(p_inst, APP_USBD_VENDOR_COMM_IFACE_IDX);

	app_usbd_class_ep_conf_t const *ep_cfg;
	ep_cfg = app_usbd_class_iface_ep_get(class_iface, APP_USBD_VENDOR_COMM_EPIN_IDX);

	return app_usbd_class_ep_address_get(ep_cfg);
}

static inline nrf_drv_usbd_ep_t vendor_data_ep_in_addr_get(app_usbd_class_inst_t const *p_inst) {
	app_usbd_class_iface_conf_t const *class_iface;
	class_iface = app_usbd_class_iface_get(p_inst, APP_USBD_VENDOR_DATA_IFACE_IDX);

	app_usbd_class_ep_conf_t const *ep_cfg;
	ep_cfg = app_usbd_class_iface_ep_get(class_iface, APP_USBD_VENDOR_DATA_EPIN_IDX);

	return app_usbd_class_ep_address_get(ep_cfg);
}

static inline nrf_drv_usbd_ep_t vendor_data_ep_out_addr_get(app_usbd_class_inst_t const *p_inst) {
	app_usbd_class_iface_conf_t const *class_iface;
	class_iface = app_usbd_class_iface_get(p_inst, APP_USBD_VENDOR_DATA_IFACE_IDX);

	app_usbd_class_ep_conf_t const *ep_cfg;
	ep_cfg = app_usbd_class_iface_ep_get(class_iface, APP_USBD_VENDOR_DATA_EPOUT_IDX);

	return app_usbd_class_ep_address_get(ep_cfg);
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

	return NRF_ERROR_NOT_SUPPORTED;
}

static ret_code_t setup_req_std_out(app_usbd_class_inst_t const * p_inst,
									app_usbd_setup_evt_t const *  p_setup_ev) {

/*
ftdiTraceAndInt("setup_req_std_out: ", p_setup_ev->setup.bRequest);
	switch (p_setup_ev->setup.bRequest) {
		case VENDOR_REQUEST_LINE_STATE:			// 0
			if (p_setup_ev->setup.wLength.w != sizeof(app_usbd_vendor_line_coding_t)) {
				return NRF_ERROR_NOT_SUPPORTED;
			}
			return vendor_req_out_datastage(p_inst, p_setup_ev);
		case VENDOR_SET_LINE_STATE:				// 1
			return vendor_req_out_datastage(p_inst, p_setup_ev);
			break;
		default:
ftdiTrace(">- need to do something here");
			break;
	}
*/
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
		case APP_USBD_VENDOR_REQ_SET_LINE_CODING:			// 0x20	(32)
			if (p_setup_ev->setup.wLength.w != sizeof(app_usbd_vendor_line_coding_t)) {
				return NRF_ERROR_NOT_SUPPORTED;
			}
			return vendor_req_out_datastage(p_inst, p_setup_ev);
			break;
		case APP_USBD_VENDOR_REQ_SET_CONTROL_LINE_STATE:	// 0x22 (34)
			if (p_setup_ev->setup.wLength.w != 0) {
				return NRF_ERROR_NOT_SUPPORTED;
			}
			uint8_t old_line_state = p_vendor_ctx->line_state;
			p_vendor_ctx->line_state = p_setup_ev->setup.wValue.w;
			uint8_t new_line_state = p_vendor_ctx->line_state;

			if (old_line_state == new_line_state) {
				return NRF_SUCCESS;
			}
			
			const app_usbd_vendor_user_event_t ev = VENDOR_REQUEST_LINE_STATE;

			user_event_handler(p_inst, ev);

			const bool new_dtr = (new_line_state & APP_USBD_CDC_ACM_LINE_STATE_DTR) ? true : false;

			const app_usbd_vendor_user_event_t ev2 = new_dtr ?
				APP_USBD_VENDOR_USER_EVT_PORT_OPEN : 
				APP_USBD_VENDOR_USER_EVT_PORT_CLOSE;
			user_event_handler(p_inst, ev2);

            if (!new_dtr)
            {
                // Abort DATA endpoints on port close
                nrf_drv_usbd_ep_t ep;
                ep = vendor_data_ep_in_addr_get(p_inst);
                nrf_drv_usbd_ep_abort(ep);
                ep = vendor_data_ep_out_addr_get(p_inst);
                nrf_drv_usbd_ep_abort(ep);

                // Set rx transfers configuration to default state.
                p_vendor_ctx->rx_transfer[0].p_buf = NULL;
                p_vendor_ctx->rx_transfer[1].p_buf = NULL;
                p_vendor_ctx->bytes_left = 0;
                p_vendor_ctx->bytes_read = 0;
                p_vendor_ctx->last_read  = 0;
                p_vendor_ctx->cur_read   = 0;
                p_vendor_ctx->p_copy_pos = p_vendor_ctx->internal_rx_buf;
            }

			return NRF_SUCCESS;
	}

	return NRF_ERROR_NOT_SUPPORTED;
}


static bool vendor_consumer(nrf_drv_usbd_ep_transfer_t * p_next,
                            void *                       p_context,
                            size_t                       ep_size,
                            size_t                       data_size)
{
	app_usbd_vendor_ctx_t * p_vendor_ctx = (app_usbd_vendor_ctx_t *) p_context;
    p_next->size = data_size;

    if (data_size <= p_vendor_ctx->rx_transfer[0].read_left) {
        p_next->p_data.rx = p_vendor_ctx->rx_transfer[0].p_buf;

        p_vendor_ctx->rx_transfer[0].p_buf     += data_size;
        p_vendor_ctx->bytes_read               += data_size;
        p_vendor_ctx->rx_transfer[0].read_left -= data_size;
        return (p_vendor_ctx->rx_transfer[0].read_left) != 0;
    }
    else
    {
		p_next->p_data.rx = p_vendor_ctx->internal_rx_buf;

		p_vendor_ctx->cur_read = data_size;
        return false;
    }
}

static bool vendor_consumer_single_shoot(nrf_drv_usbd_ep_transfer_t * p_next,
                            void *                       p_context,
                            size_t                       ep_size,
                            size_t                       data_size)
{
	app_usbd_vendor_ctx_t * p_vendor_ctx = (app_usbd_vendor_ctx_t *) p_context;
    p_next->size = data_size;
    if (data_size <= p_vendor_ctx->rx_transfer[0].read_left)
    {
        p_next->p_data.rx = p_vendor_ctx->rx_transfer[0].p_buf;
        p_vendor_ctx->bytes_read               += data_size;
        p_vendor_ctx->rx_transfer[0].read_left -= data_size;
    }
    else
    {
		p_next->p_data.rx = p_vendor_ctx->internal_rx_buf;

		p_vendor_ctx->cur_read = data_size;
    }

	return false;
}



static ret_code_t setup_event_handler(app_usbd_class_inst_t const *p_inst,
									  app_usbd_setup_evt_t const *p_setup_ev) {
	ASSERT(p_inst != NULL);
	ASSERT(p_setup_ev != NULL);

ftdiTraceAndbmReq("vendor req:", p_setup_ev->setup.bmRequestType);
	if (app_usbd_setup_req_dir(p_setup_ev->setup.bmRequestType) == APP_USBD_SETUP_REQDIR_IN) {
		switch (app_usbd_setup_req_typ(p_setup_ev->setup.bmRequestType)) {
			case APP_USBD_SETUP_REQTYPE_STD:
				return setup_req_std_in(p_inst, p_setup_ev);
			case APP_USBD_SETUP_REQTYPE_CLASS:
				return setup_req_class_in(p_inst, p_setup_ev);
			case APP_USBD_SETUP_REQTYPE_VENDOR:
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
			case APP_USBD_SETUP_REQTYPE_VENDOR:
				return setup_req_class_out(p_inst, p_setup_ev);
			default:
//				return setup_req_std_out(p_inst, p_setup_ev);
				break;
		}
	}

	return NRF_ERROR_NOT_SUPPORTED;
}

static void vendor_reset_port(app_usbd_class_inst_t const *p_inst) {
	app_usbd_vendor_t const *p_vendor = vendor_get(p_inst);
	app_usbd_vendor_ctx_t   *p_vendor_ctx = vendor_ctx_get(p_vendor);

	p_vendor_ctx->line_state = 0;

	p_vendor_ctx->rx_transfer[0].p_buf = NULL;
	p_vendor_ctx->rx_transfer[1].p_buf = NULL;
	p_vendor_ctx->bytes_left = 0;
	p_vendor_ctx->bytes_read = 0;
	p_vendor_ctx->last_read = 0;
	p_vendor_ctx->cur_read = 0;
	p_vendor_ctx->p_copy_pos = p_vendor_ctx->internal_rx_buf;
}

static ret_code_t vendor_rx_block_finished(app_usbd_class_inst_t const *p_inst) {
	app_usbd_vendor_t const *p_vendor = vendor_get(p_inst);
	app_usbd_vendor_ctx_t *p_vendor_ctx = vendor_ctx_get(p_vendor);
	nrf_drv_usbd_ep_t ep = vendor_data_ep_out_addr_get(p_inst);

	nrf_drv_usbd_handler_desc_t handler_desc = {
		.handler.consumer = vendor_consumer,
		.p_context = p_vendor_ctx
	};

	if (p_vendor_ctx->rx_transfer[0].read_left == 0) { // buffer completely filled by consumer
		p_vendor_ctx->last_read = p_vendor_ctx->bytes_read;
		p_vendor_ctx->bytes_read = 0;
		p_vendor_ctx->bytes_left = 0;

		if (p_vendor_ctx->rx_transfer[1].p_buf != NULL ) {
			p_vendor_ctx->rx_transfer[0] = p_vendor_ctx->rx_transfer[1];
			p_vendor_ctx->rx_transfer[1].p_buf = NULL;
			return app_usbd_ep_handled_transfer(ep, &handler_desc);
		}
		else {
			p_vendor_ctx->rx_transfer[0].p_buf = NULL;
			return NRF_SUCCESS;
		}
	}

	size_t bytes_read = p_vendor_ctx->cur_read;
	size_t bytes_to_copy = bytes_read;

	if (bytes_read > p_vendor_ctx->rx_transfer[0].read_left) {
		bytes_to_copy = p_vendor_ctx->rx_transfer[0].read_left;
	}
	memcpy(p_vendor_ctx->rx_transfer[0].p_buf,
		   p_vendor_ctx->internal_rx_buf,
			bytes_to_copy);

	// first buffer is full
	p_vendor_ctx->last_read = p_vendor_ctx->bytes_read + bytes_to_copy;
	p_vendor_ctx->bytes_read = 0;
	p_vendor_ctx->bytes_left = bytes_read - bytes_to_copy;
	p_vendor_ctx->p_copy_pos = p_vendor_ctx->internal_rx_buf + bytes_to_copy;

	if (p_vendor_ctx->rx_transfer[1].p_buf != NULL) {
		// if there is second transfer, copy it to first
		p_vendor_ctx->rx_transfer[0] = p_vendor_ctx->rx_transfer[1];
		p_vendor_ctx->rx_transfer[1].p_buf = NULL;

		while (p_vendor_ctx->bytes_left > 0) {
			if (p_vendor_ctx->bytes_left >= p_vendor_ctx->rx_transfer[0].read_left) {
				// if there are enough bytes left in internal buffer to
				// completely fill next transfer, we call user event handler
				// to obtain next buffer and continue double buffering.
				memcpy(p_vendor_ctx->rx_transfer[0].p_buf,
					   p_vendor_ctx->p_copy_pos,
					   p_vendor_ctx->rx_transfer[0].read_left);
				p_vendor_ctx->bytes_left -= p_vendor_ctx->rx_transfer[0].read_left;
				p_vendor_ctx->p_copy_pos += p_vendor_ctx->rx_transfer[0].read_left;
				p_vendor_ctx->last_read = p_vendor_ctx->rx_transfer[0].read_left;
				user_event_handler(p_inst, APP_USBD_VENDOR_USER_EVT_RX_DONE);
				if (p_vendor_ctx->rx_transfer[1].p_buf != NULL) {
					p_vendor_ctx->rx_transfer[0] = p_vendor_ctx->rx_transfer[1];
					p_vendor_ctx->rx_transfer[1].p_buf = NULL;
				}
				else {
					// if user does not specify a second buffer, all data
					// transfers are done and data left in internal buffer
					// is lost.
					p_vendor_ctx->rx_transfer[0].p_buf = NULL;
					break;
				}
			}
			else {
				memcpy(p_vendor_ctx->rx_transfer[0].p_buf,
					   p_vendor_ctx->p_copy_pos,
					   p_vendor_ctx->bytes_left);
				p_vendor_ctx->bytes_read = p_vendor_ctx->bytes_left;
				p_vendor_ctx->rx_transfer[0].read_left -= p_vendor_ctx->bytes_left;
				p_vendor_ctx->rx_transfer[0].p_buf += p_vendor_ctx->bytes_left;
				break;
			}
		}
	}
	else {
		p_vendor_ctx->rx_transfer[0].p_buf = NULL;
	}

	if (p_vendor_ctx->rx_transfer[0].p_buf != NULL) {
		return app_usbd_ep_handled_transfer(ep, &handler_desc);
	}

	return NRF_SUCCESS;
}

static ret_code_t vendor_endpoint_ev(app_usbd_class_inst_t const *p_inst, app_usbd_complex_evt_t const *p_event) {
	if (vendor_comm_ep_in_addr_get(p_inst) == p_event->drv_evt.data.eptransfer.ep) {
		return NRF_SUCCESS;
	}
	ret_code_t ret;
	if (NRF_USBD_EPIN_CHECK(p_event->drv_evt.data.eptransfer.ep)) {
		switch (p_event->drv_evt.data.eptransfer.status) {
			case NRF_USBD_EP_OK:
				user_event_handler(p_inst, APP_USBD_VENDOR_USER_EVT_TX_DONE);
				return NRF_SUCCESS;
			case NRF_USBD_EP_ABORTED:
				return NRF_SUCCESS;
			default:
				return NRF_ERROR_INTERNAL;
		}
	}

	if (NRF_USBD_EPOUT_CHECK(p_event->drv_evt.data.eptransfer.ep)) {
		switch (p_event->drv_evt.data.eptransfer.status) {
			case NRF_USBD_EP_OK:
				ret = vendor_rx_block_finished(p_inst);
				user_event_handler(p_inst, APP_USBD_VENDOR_USER_EVT_RX_DONE);
				return ret;
			case NRF_USBD_EP_WAITING:
			case NRF_USBD_EP_ABORTED:
				return NRF_SUCCESS;
			default:
				return NRF_ERROR_INTERNAL;
		}
	}
	return NRF_ERROR_NOT_SUPPORTED;
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
			ret = vendor_endpoint_ev(p_inst, p_event);
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
			//MDK -- we get here
			break;
		default:
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
//		APP_USBD_CLASS_DESCRIPTOR_WRITE(sizeof(app_usbd_descriptor_iface_t)); // bLength
		APP_USBD_CLASS_DESCRIPTOR_WRITE(0x09); // bLength
		APP_USBD_CLASS_DESCRIPTOR_WRITE(APP_USBD_DESCRIPTOR_INTERFACE); // bDescriptorType

		static app_usbd_class_iface_conf_t const *p_cur_iface = NULL;
		p_cur_iface = app_usbd_class_iface_get(p_inst, i);

		APP_USBD_CLASS_DESCRIPTOR_WRITE(app_usbd_class_iface_number_get(p_cur_iface)); // bInterfaceNumber
		APP_USBD_CLASS_DESCRIPTOR_WRITE(0x00); // bAlternateSetting
		APP_USBD_CLASS_DESCRIPTOR_WRITE(app_usbd_class_iface_ep_count_get(p_cur_iface)); // bNumEndpoints

		if (p_vendor->specific.inst.comm_interface == app_usbd_class_iface_number_get(p_cur_iface)) {
			APP_USBD_CLASS_DESCRIPTOR_WRITE(APP_USBD_VENDOR_COMM_CLASS); // bInterfaceClass
			APP_USBD_CLASS_DESCRIPTOR_WRITE(APP_USBD_VENDOR_SUBCLASS_ACM); // bInterfaceSubClass
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

ret_code_t app_usbd_vendor_write(app_usbd_vendor_t const *p_vendor,
								const void *p_buf,
								size_t		length) {
	app_usbd_class_inst_t const *p_inst = app_usbd_vendor_class_inst_get(p_vendor);
	app_usbd_vendor_ctx_t *p_vendor_ctx = vendor_ctx_get(p_vendor);
	bool dtr_state = (p_vendor_ctx->line_state & APP_USBD_CDC_ACM_LINE_STATE_DTR) ? true : false;

	if (!dtr_state) {
		/* port is not opened */
		return NRF_ERROR_INVALID_STATE;
	}

	nrf_drv_usbd_ep_t ep = vendor_data_ep_in_addr_get(p_inst);

	if (APP_USBD_CDC_ACM_ZLP_ON_EPSIZE_WRITE && ((length % NRF_DRV_USBD_EPSIZE) == 0)) {
		NRF_DRV_USBD_TRANSFER_IN_ZLP(transfer, p_buf, length);
		return app_usbd_ep_transfer(ep, &transfer);
	}
	else {
		NRF_DRV_USBD_TRANSFER_IN(transfer, p_buf, length);
		return app_usbd_ep_transfer(ep, &transfer);
	}
}

size_t app_usbd_vendor_rx_size(app_usbd_vendor_t const *p_vendor) {
	app_usbd_vendor_ctx_t *p_vendor_ctx = vendor_ctx_get(p_vendor);
	return p_vendor_ctx->last_read;
}

size_t app_usbd_vendor_bytes_stored(app_usbd_vendor_t const *p_vendor) {
	app_usbd_vendor_ctx_t *p_vendor_ctx = vendor_ctx_get(p_vendor);
	return p_vendor_ctx->bytes_left;
}

ret_code_t app_usbd_vendor_read(app_usbd_vendor_t const *p_vendor,
								void *p_buf,
								size_t length) {
	ASSERT(p_buf != NULL);
	ret_code_t ret;
	app_usbd_vendor_ctx_t *p_vendor_ctx = vendor_ctx_get(p_vendor);
	if (0U == (p_vendor_ctx->line_state & APP_USBD_CDC_ACM_LINE_STATE_DTR)) {
		/* port not opened */
		return NRF_ERROR_INVALID_STATE;
	}

#if (APP_USBD_CONFIG_EVENT_QUEUE_ENABLE == 0)
	CRITICAL_REGION_ENTER();
#endif

	if (p_vendor_ctx->rx_transfer[0].p_buf == NULL) {
		if (p_vendor_ctx->bytes_left >= length) {
//_A		// this gets called a bunch
			memcpy(p_buf, p_vendor_ctx->p_copy_pos, length);
			p_vendor_ctx->bytes_left -= length;
			p_vendor_ctx->p_copy_pos += length;
			p_vendor_ctx->last_read = length;
			ret = NRF_SUCCESS;
		}
		else {
//_B		// this gets called at the end before the user's callback gets called
			p_vendor_ctx->rx_transfer[0].p_buf = p_buf;
			p_vendor_ctx->rx_transfer[0].read_left = length;
			nrf_drv_usbd_ep_t ep = vendor_data_ep_out_addr_get(app_usbd_vendor_class_inst_get(p_vendor));
			nrf_drv_usbd_handler_desc_t const handler_desc = {
				.handler.consumer = vendor_consumer,
				.p_context = p_vendor_ctx
			};

			if (p_vendor_ctx->bytes_left > 0) {
//_B_2
				memcpy(p_vendor_ctx->rx_transfer[0].p_buf,
					   p_vendor_ctx->p_copy_pos,
					   p_vendor_ctx->bytes_left);
				p_vendor_ctx->rx_transfer[0].read_left -= p_vendor_ctx->bytes_left;
				p_vendor_ctx->rx_transfer[0].p_buf += p_vendor_ctx->bytes_left;
				p_vendor_ctx->bytes_read = p_vendor_ctx->bytes_left;
				p_vendor_ctx->bytes_left = 0;

			}
			ret = app_usbd_ep_handled_transfer(ep, &handler_desc);
			if (ret == NRF_SUCCESS) {
				ret = NRF_ERROR_IO_PENDING;
			}
		}
	}
	else if (p_vendor_ctx->rx_transfer[1].p_buf == NULL) {
//_C
		p_vendor_ctx->rx_transfer[1].p_buf = p_buf;
		p_vendor_ctx->rx_transfer[1].read_left = length;
		ret = NRF_ERROR_IO_PENDING;
	}
	else {
//_D
		ret = NRF_ERROR_BUSY;
	}

#if (APP_USBD_CONFIG_EVENT_QUEUE_ENABLE == 0)
	CRITICAL_REGION_EXIT();
#endif

	return ret;
}

ret_code_t app_usbd_vendor_read_any(app_usbd_vendor_t const *p_vendor,
								void *p_buf,
								size_t length) {
	ASSERT(p_buf != NULL);
	ret_code_t ret;
	app_usbd_vendor_ctx_t *p_vendor_ctx = vendor_ctx_get(p_vendor);
	if (0U == (p_vendor_ctx->line_state & APP_USBD_CDC_ACM_LINE_STATE_DTR)) {
		/* port not opened */
		return NRF_ERROR_INVALID_STATE;
	}

#if (APP_USBD_CONFIG_EVENT_QUEUE_ENABLE == 0)
	CRITICAL_REGION_ENTER();
#endif
	if (p_vendor_ctx->bytes_left > 0) {
		size_t to_copy = MIN(length, p_vendor_ctx->bytes_left);
		memcpy(p_buf, p_vendor_ctx->p_copy_pos, to_copy);
		p_vendor_ctx->bytes_left -= to_copy;
		p_vendor_ctx->p_copy_pos += to_copy;
		p_vendor_ctx->last_read = to_copy;
		ret = NRF_SUCCESS;
	}
	else {
		if (p_vendor_ctx->rx_transfer[0].p_buf == NULL) {
			p_vendor_ctx->rx_transfer[0].p_buf = p_buf;
			p_vendor_ctx->rx_transfer[0].read_left = length;
			nrf_drv_usbd_ep_t ep = vendor_data_ep_out_addr_get(app_usbd_vendor_class_inst_get(p_vendor));
			nrf_drv_usbd_handler_desc_t const handler_desc = {
				.handler.consumer = vendor_consumer_single_shoot,
				.p_context = p_vendor_ctx
			};

			ret = app_usbd_ep_handled_transfer(ep, &handler_desc);
		}
		else {
			ret = NRF_ERROR_BUSY;
		}
	}

#if (APP_USBD_CONFIG_EVENT_QUEUE_ENABLE == 0)
	CRITICAL_REGION_EXIT();
#endif

	return ret;
}

