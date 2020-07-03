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
	uint8_t *p_buf;
	size_t read_left;
} vendor_rx_buffer_t;

typedef struct {
    app_usbd_vendor_req_t   request;
    app_usbd_vendor_line_coding_t line_coding;
    uint16_t line_state;
    uint16_t serial_state;

	vendor_rx_buffer_t rx_transfer[2];
	uint8_t internal_rx_buf[NRF_DRV_USBD_EPSIZE];
	uint8_t *p_copy_pos;
	size_t bytes_left;
	size_t bytes_read;
	size_t last_read;
	size_t cur_read;
} app_usbd_vendor_ctx_t;

#define APP_USBD_CDC_ACM_LINE_STATE_DTR  (1u << 0)  /**< Line state bit DTR.*/
#define APP_USBD_CDC_ACM_LINE_STATE_RTS  (1u << 1)  /**< Line state bit RTS.*/

#define APP_USBD_VENDOR_DATA_SPECIFIC_DEC app_usbd_vendor_ctx_t ctx;


APP_USBD_CLASS_TYPEDEF(app_usbd_vendor,			\
		APP_USBD_VENDOR_CONFIG(0, 0, 0, 0, 0),		\
		APP_USBD_VENDOR_INSTANCE_SPECIFIC_DEC,	\
		APP_USBD_VENDOR_DATA_SPECIFIC_DEC		\
	);

// events passed to user event handler
typedef enum app_usbd_vendor_user_event_e {
	VENDOR_REQUEST_LINE_STATE,
	VENDOR_SET_LINE_STATE,

	APP_USBD_VENDOR_USER_EVT_RX_DONE,
	APP_USBD_VENDOR_USER_EVT_TX_DONE,

	APP_USBD_VENDOR_USER_EVT_PORT_OPEN,
	APP_USBD_VENDOR_USER_EVT_PORT_CLOSE,
} app_usbd_vendor_user_event_t;

#define APP_USBD_VENDOR_GLOBAL_DEF(instance_name, 	\
				user_ev_handler, 					\
				comm_ifc,							\
				data_ifc, 							\
				comm_epin,							\
				data_epin, 							\
				data_epout,							\
				protocol) 							\
	APP_USBD_VENDOR_GLOBAL_DEF_INTERNAL(instance_name,		\
				user_ev_handler,							\
				comm_ifc,									\
				data_ifc,									\
				comm_epin,									\
				data_epin,									\
				data_epout,									\
				protocol)									\

static inline app_usbd_class_inst_t const *
app_usbd_vendor_class_inst_get(app_usbd_vendor_t const *p_vendor) {
	return &p_vendor->base;
}

static inline app_usbd_vendor_t const *
app_usbd_vendor_class_get(app_usbd_class_inst_t const *p_inst) {
	return (app_usbd_vendor_t const *)p_inst;
}

ret_code_t app_usbd_vendor_write(app_usbd_vendor_t const *p_vendor, const void *p_buf, size_t length);
ret_code_t app_usbd_vendor_read(app_usbd_vendor_t const *p_vendor, void *p_buf, size_t length);


#define APP_USBD_VENDOR_COMM_CLASS		0xff // vendor 0x02	// APP_USBD_CDC_COMM_CLASS
#define APP_USBD_VENDOR_DATA_CLASS		0xff // vendor 0x0A	// APP_USBD_CDC_COMM_CLASS
#define APP_USBD_VENDOR_SUBCLASS_ACM	0x02	// APP_USBD_CDC_SUBCLASS_ACM

#define APP_USBD_CDC_COMM_PROTOCOL_AT_V250	0x01


#define APP_USBD_VENDOR_IAD_DSC(interface_number, subclass, protocol)	\
    /*.bLength =           */ sizeof(app_usbd_descriptor_iad_t),          \
    /*.bDescriptorType =   */ APP_USBD_DESCRIPTOR_INTERFACE_ASSOCIATION,  \
    /*.bFirstInterface =   */ interface_number,                           \
    /*.bInterfaceCount =   */ 2,                                          \
    /*.bFunctionClass =    */ APP_USBD_VENDOR_COMM_CLASS,                    \
    /*.bFunctionSubClass = */ subclass,                                   \
    /*.bFunctionProtocol = */ protocol,                                   \
    /*.iFunction =         */ 0,                                          \

#define APP_USBD_VENDOR_COMM_INTERFACE_DSC(interface_number, subclass, protocol)  \
    /*.bLength =            */ sizeof(app_usbd_descriptor_iface_t),            \
    /*.bDescriptorType =    */ APP_USBD_DESCRIPTOR_INTERFACE,                  \
    /*.bInterfaceNumber =   */ interface_number,                               \
    /*.bAlternateSetting =  */ 0x00,                                           \
    /*.bNumEndpoints =      */ 1,                                              \
    /*.bInterfaceClass =    */ APP_USBD_VENDOR_COMM_CLASS,                        \
    /*.bInterfaceSubClass = */ subclass,                                       \
    /*.bInterfaceProtocol = */ protocol,                                       \
    /*.iInterface = 0,      */ 0x00,                                           \

#define APP_USBD_VENDOR_DATA_INTERFACE_DSC(interface_number, subclass, protocol) \
    /*.bLength =            */ sizeof(app_usbd_descriptor_iface_t),           \
    /*.bDescriptorType =    */ APP_USBD_DESCRIPTOR_INTERFACE,                 \
    /*.bInterfaceNumber =   */ interface_number,                              \
    /*.bAlternateSetting =  */ 0x00,                                          \
    /*.bNumEndpoints =      */ 2,                                             \
    /*.bInterfaceClass =    */ APP_USBD_VENDOR_DATA_CLASS,                       \
    /*.bInterfaceSubClass = */ subclass,                                      \
    /*.bInterfaceProtocol = */ protocol,                                      \
    /*.iInterface = 0,      */ 0x00,                                          \

#define APP_USBD_VENDOR_COM_EP_DSC(endpoint_in, ep_size)                     \
    /*.bLength =          */ sizeof(app_usbd_descriptor_ep_t),            \
    /*.bDescriptorType =  */ APP_USBD_DESCRIPTOR_ENDPOINT,                \
    /*.bEndpointAddress = */ endpoint_in,                                 \
    /*.bmAttributes =     */ APP_USBD_DESCRIPTOR_EP_ATTR_TYPE_INTERRUPT,  \
    /*.wMaxPacketSize =   */ APP_USBD_U16_TO_RAW_DSC(ep_size),            \
    /*.bInterval =        */ 16,                                          \

#define APP_USBD_VENDOR_DATA_EP_DSC(endpoint_in, endpoint_out, ep_size) \
    /*.bLength =          */ sizeof(app_usbd_descriptor_ep_t),       \
    /*.bDescriptorType =  */ APP_USBD_DESCRIPTOR_ENDPOINT,           \
    /*.bEndpointAddress = */ endpoint_in,                            \
    /*.bmAttributes =     */ APP_USBD_DESCRIPTOR_EP_ATTR_TYPE_BULK,  \
    /*.wMaxPacketSize =   */ APP_USBD_U16_TO_RAW_DSC(ep_size),       \
    /*.bInterval =        */ 0,                                      \
    /*.bLength =          */ sizeof(app_usbd_descriptor_ep_t),       \
    /*.bDescriptorType =  */ APP_USBD_DESCRIPTOR_ENDPOINT,           \
    /*.bEndpointAddress = */ endpoint_out,                           \
    /*.bmAttributes =     */ APP_USBD_DESCRIPTOR_EP_ATTR_TYPE_BULK,  \
    /*.wMaxPacketSize =   */ APP_USBD_U16_TO_RAW_DSC(ep_size),       \
    /*.bInterval =        */ 0,                                      \

#define APP_USBD_VENDOR_HEADER_DSC(bcd_cdc)                           \
    /*.bLength =            */ sizeof(app_usbd_cdc_desc_header_t), \
    /*.bDescriptorType =    */ APP_USBD_VENDOR_CS_INTERFACE,          \
    /*.bDescriptorSubtype = */ APP_USBD_VENDOR_SCS_HEADER,            \
    /*.bcdCDC =             */ APP_USBD_U16_TO_RAW_DSC(bcd_cdc),   \

#define APP_USBD_VENDOR_CALL_MGMT_DSC(capabilities, data_interface)      \
    /*.bLength =            */ sizeof(app_usbd_cdc_desc_call_mgmt_t), \
    /*.bDescriptorType =    */ APP_USBD_VENDOR_CS_INTERFACE,             \
    /*.bDescriptorSubtype = */ APP_USBD_VENDOR_SCS_CALL_MGMT,            \
    /*.bmCapabilities =     */ capabilities,                          \
    /*.bDataInterface =     */ data_interface,                        \

#define APP_USBD_VENDOR_DSC(capabilities)                      \
    /*.bLength =            */ sizeof(app_usbd_cdc_desc_acm_t), \
    /*.bDescriptorType =    */ APP_USBD_VENDOR_CS_INTERFACE,       \
    /*.bDescriptorSubtype = */ APP_USBD_VENDOR_SCS_ACM,            \
    /*.bmCapabilities =     */ capabilities,                    \

#define APP_USBD_VENDOR_UNION_DSC(control_interface, ...)                                            \
    /*.bLength =               */ sizeof(app_usbd_cdc_desc_union_t) + (NUM_VA_ARGS(__VA_ARGS__)), \
    /*.bDescriptorType =       */ APP_USBD_VENDOR_CS_INTERFACE,                                      \
    /*.bDescriptorSubtype =    */ APP_USBD_VENDOR_SCS_UNION,                                         \
    /*.bControlInterface =     */ control_interface,                                              \
    /*.bSubordinateInterface = */ __VA_ARGS__,                                                    \

#ifdef __cplusplus
}
#endif
#endif // _APP_USBD_VENDOR_H__

