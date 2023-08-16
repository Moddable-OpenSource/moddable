#ifndef __APP_USBD_VENDOR_INTERNAL_H__
#define __APP_USBD_VENDOR_INTERNAL_H__

#ifdef __cplusplus
extern"C" {
#endif

APP_USBD_CLASS_FORWARD(app_usbd_vendor);

enum app_usbd_vendor_user_event_e;

typedef void (*app_usbd_vendor_user_ev_handler_t)(
	app_usbd_class_inst_t const *p_inst,
	enum app_usbd_vendor_user_event_e event);

typedef struct {
	uint8_t comm_interface;
	uint8_t comm_epin;
	uint8_t data_interface;
	uint8_t data_epout;
	uint8_t data_epin;
	uint16_t protocol;
	app_usbd_vendor_user_ev_handler_t user_ev_handler;
	uint8_t *p_ep_interval;
} app_usbd_vendor_inst_t;

typedef enum {
	APP_USBD_VENDOR_x					// internal module state
} app_usbd_vendor_state_t;

// cdc line coding structure
typedef struct {
	uint8_t dwDTERate[4];
	uint8_t bCharFormat;
	uint8_t bParityType;
	uint8_t bDataBits;
} app_usbd_vendor_line_coding_t;

typedef struct {
	uint8_t bmRequestType;
	uint8_t bRequest;
	uint16_t wValue;
	uint16_t wIndex;
	uint16_t wLength;
	uint16_t serial_state;
} app_usbd_vendor_notify_t;

typedef struct {
	uint8_t	type;
	uint8_t	len;
	union {
		app_usbd_vendor_line_coding_t line_coding;
		app_usbd_vendor_notify_t notify;
	} payload;
} app_usbd_vendor_req_t;

/*
typedef struct {
	app_usbd_vendor_req_t	request;
	app_usbd_vendor_line_coding_t line_coding;
	uint16_t line_state;
	uint16_t serial_state;
} app_usbd_vendor_ctx_t;
**/

#define APP_USBD_VENDOR_CONFIG(comm_iface, epin_comm, data_iface, epin_data, epout_data) \
	 ((comm_iface, epin_comm), (data_iface, epin_data, epout_data))

#define APP_USBD_VENDOR_INSTANCE_SPECIFIC_DEC app_usbd_vendor_inst_t inst;

extern const app_usbd_class_methods_t app_usbd_vendor_class_methods;

#define APP_USBD_VENDOR_INST_CONFIG(user_event_handler, 	\
									comm_ifc, 				\
									comm_ein, 				\
									data_ifc, 				\
									data_ein, 				\
									data_eout, 				\
									cdc_protocol,			\
									ep_list) 				\
    .inst = {										\
        .user_ev_handler = user_event_handler,      \
        .comm_interface = comm_ifc,                 \
        .comm_epin = comm_ein,                      \
        .data_interface = data_ifc,                 \
        .data_epin = data_ein,                      \
        .data_epout = data_eout,                    \
		.protocol = cdc_protocol,					\
		.p_ep_interval = ep_list					\
	}

//		#define APP_USBD_VENDOR_DATA_SPECIFIC_DEC app_usbd_vendor_ctx_t ctx;

		#define APP_USBD_VENDOR_DEFAULT_INTERVAL	0x10

		extern const app_usbd_class_methods_t app_usbd_vendor_class_methods;

		#define APP_USBD_VENDOR_GLOBAL_DEF_INTERNAL(instance_name,		\
													user_ev_handler,	\
													comm_ifc,			\
													data_ifc,			\
													comm_ein,			\
													data_ein,			\
													data_eout,			\
													protocol)	\
		static uint8_t CONCAT_2(instance_name, _ep) = {		\
			(APP_USBD_EXTRACT_INTERVAL_FLAG(comm_ein) ? APP_USBD_EXTRACT_INTERVAL_VALUE(comm_ein) : APP_USBD_VENDOR_DEFAULT_INTERVAL)};	\
		APP_USBD_CLASS_INST_GLOBAL_DEF(		\
			instance_name,					\
			app_usbd_vendor,				\
			&app_usbd_vendor_class_methods,		\
			APP_USBD_VENDOR_CONFIG(comm_ifc, comm_ein, data_ifc, data_ein, data_eout),	\
			(APP_USBD_VENDOR_INST_CONFIG(user_ev_handler,		\
										 comm_ifc,				\
										 comm_ein,				\
										 data_ifc,				\
										 data_ein,				\
										 data_eout,				\
										 protocol,				\
						&CONCAT_2(instance_name, _ep)))			\
	)

#define APP_USBD_VENDOR_REQ_SET_LINE_CODING 		0x20	//32
#define APP_USBD_VENDOR_REQ_GET_LINE_CODING 		0x21	//33
#define APP_USBD_VENDOR_REQ_SET_CONTROL_LINE_STATE	0x22	//34

#ifdef __cplusplus
}
#endif

#endif // __APP_USBD_VENDOR_INTERNAL_H__
