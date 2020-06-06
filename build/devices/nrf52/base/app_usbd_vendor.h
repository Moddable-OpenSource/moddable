#ifndef _APP_USBD_VENDOR_H__
#define _APP_USBD_VENDOR_H__

#include <stdint.h>
#include <stdbool.h>

#include "nrf_drv_usbd.h"
//#include "nrf_block_dev.h"
#include "app_usbd_class_base.h"
#include "app_usbd.h"
#include "app_usbd_core.h"
#include "app_usbd_descriptor.h"

#include "app_usbd_vendor_types.h"
#include "app_usbd_vendor_internal.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    app_usbd_vendor_req_t   request;
    app_usbd_vendor_line_coding_t line_coding;
    uint16_t line_state;
    uint16_t serial_state;
} app_usbd_vendor_ctx_t;

#define APP_USBD_VENDOR_DATA_SPECIFIC_DEC app_usbd_vendor_ctx_t ctx;


APP_USBD_CLASS_TYPEDEF(app_usbd_vendor,			\
		APP_USBD_VENDOR_CONFIG(0, 0, 0, 0, 0),		\
		APP_USBD_VENDOR_INSTANCE_SPECIFIC_DEC,	\
		APP_USBD_VENDOR_DATA_SPECIFIC_DEC		\
	);

// events passed to user event handler
typedef enum app_usbd_vendor_user_event_e {
	VENDOR_REQUEST_LINE_STATE = 1,
	VENDOR_SET_LINE_STATE = 2,
} app_usbd_vendor_user_event_t;

#define APP_USBD_VENDOR_GLOBAL_DEF(instance_name, 	\
				user_ev_handler, 					\
				comm_ifc,							\
				data_ifc, 							\
				comm_epin,							\
				data_epin, 							\
				data_epout) 							\
	APP_USBD_VENDOR_GLOBAL_DEF_INTERNAL(instance_name,		\
				user_ev_handler,							\
				comm_ifc,									\
				data_ifc,									\
				comm_epin,									\
				data_epin,									\
				data_epout)									\

static inline app_usbd_class_inst_t const *
app_usbd_vendor_class_inst_get(app_usbd_vendor_t const *p_vendor) {
	return &p_vendor->base;
}

static inline app_usbd_vendor_t const *
app_usbd_vendor_class_get(app_usbd_class_inst_t const *p_inst) {
	return (app_usbd_vendor_t const *)p_inst;
}


#ifdef __cplusplus
}
#endif
#endif // _APP_USBD_VENDOR_H__

