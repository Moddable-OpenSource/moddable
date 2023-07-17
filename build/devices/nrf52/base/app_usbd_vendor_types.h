#ifndef __APP_USBD_VENDOR_TYPES_H__
#define __APP_USBD_VENDOR_TYPES_H__

#include "app_util.h"

#ifdef __cplusplus
extern "C" {
#endif

#define APP_USBD_VENDOR_CLASS	0xFF

typedef enum {
	APP_USBD_VENDOR_SUBCLASS = 0x01,
	APP_USBD_VENDOR_COMM_SUBCLASS = 0x02,
	APP_USBD_VENDOR_DATA_SUBCLASS = 0x03,
} app_usbd_vendor_subclass_t;

#define APP_USBD_VENDOR_CS_INTERFACE	0x24		// CDC_CS_INTERFACE
#define APP_USBD_VENDOR_CS_ENDPOINT		0x25		// CDC_CS_ENDPOINT

#define APP_USBD_VENDOR_SCS_HEADER		0x00
#define APP_USBD_VENDOR_SCS_CALL_MGMT	0x01
#define APP_USBD_VENDOR_SCS_ACM			0x02
#define APP_USBD_VENDOR_SCS_UNION		0x06

typedef enum {
	APP_USBD_VENDOR_PROTOCOL_RUNTIME = 0x01
} app_usbd_vendor_protocol_t;

/*
typedef enum {
	APP_USBD_VENDOR_REBOOT	= 0x00,
	APP_USBD_VENDOR_PROGRAM = 0x01
} app_usbd_vendor_req_t;
*/

typedef enum {
	APP_USBD_VENDOR_CS_FUNCTIONAL = 0x21,
} app_usbd_vendor_func_type_t;

typedef enum {
	APP_USBD_VENDOR_BIT_CAN_DNLOAD		= 0x01,	// bitCanDnload
	APP_USBD_VENDOR_BIT_CAB_UPLOAD		= 0x02,	// bitCanUpload
	APP_USBD_VENDOR_BIT_MANI_TOLERANT	= 0x04,	// bitManifestationTolerant
	APP_USBD_VENDOR_BIT_WILL_DETACH		= 0x08,	// bitWillDetach
} app_usbd_vendor_bm_attributes_t;

#pragma pack(push, 1)

typedef struct {
	uint8_t	bFunctionLength;
	uint8_t	bDescriptorType;
	uint8_t	bmAttributes;
} app_usbd_vendor_desc_func_t;

#pragma pack(pop)

#ifdef __cplusplus
}
#endif
#endif // __APP_USBD_VENDOR_TYPES_H__
