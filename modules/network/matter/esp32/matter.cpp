/*
 * Copyright (c) 2023 Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Runtime.
 * 
 *   The Moddable SDK Runtime is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Runtime is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 * 
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with the Moddable SDK Runtime.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "xs.h"
#include "xsmc.h"
#include "xsHost.h"

#include "mc.xs.h"			// for xsID_ values
#include "mc.defines.h"

#include <esp_matter.h>
#include <esp_matter_console.h>
#include <esp_matter_ota.h>

#include <app/server/CommissioningWindowManager.h>
#include <app/server/Server.h>

#define STANDARD_BRIGHTNESS 100
#define STANDARD_HUE 360
#define STANDARD_SATURATION 100
#define STANDARD_TEMPERATURE_FACTOR 1000000

/** Matter max values (used for remapping attributes) */
#define MATTER_BRIGHTNESS 254
#define MATTER_HUE 254
#define MATTER_SATURATION 254
#define MATTER_TEMPERATURE_FACTOR 1000000

/** Default attribute values used during initialization */
#define DEFAULT_POWER true
#define DEFAULT_BRIGHTNESS 64
#define DEFAULT_HUE 128
#define DEFAULT_SATURATION 254

uint16_t light_endpoint_id = 0;

static void matterDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
void xs_matter_mark(xsMachine *the, void *it, xsMarkRoot markRoot);

const xsHostHooks ICACHE_RODATA_ATTR xsMatterHooks = {
	xs_matter_destructor,
	xs_matter_mark,
	NULL};

struct MatterRecord
{
	xsMachine *the;
	xsSlot obj;

	xsSlot *onChanged;
};
typedef struct MatterRecord MatterRecord;
typedef struct MatterRecord *Matter;

using namespace esp_matter;
using namespace esp_matter::attribute;
using namespace esp_matter::endpoint;
using namespace chip::app::Clusters;

static void app_event_cb(const ChipDeviceEvent *event, intptr_t arg)
{
	switch (event->Type)
	{
	case chip::DeviceLayer::DeviceEventType::kInterfaceIpAddressChanged:
		// ESP_LOGI(TAG, "Interface IP Address changed");
		break;

	case chip::DeviceLayer::DeviceEventType::kCommissioningComplete:
		// ESP_LOGI(TAG, "Commissioning complete");
		break;

	case chip::DeviceLayer::DeviceEventType::kFailSafeTimerExpired:
		// ESP_LOGI(TAG, "Commissioning failed, fail safe timer expired");
		break;

	case chip::DeviceLayer::DeviceEventType::kCommissioningSessionStarted:
		// ESP_LOGI(TAG, "Commissioning session started");
		break;

	case chip::DeviceLayer::DeviceEventType::kCommissioningSessionStopped:
		// ESP_LOGI(TAG, "Commissioning session stopped");
		break;

	case chip::DeviceLayer::DeviceEventType::kCommissioningWindowOpened:
		// ESP_LOGI(TAG, "Commissioning window opened");
		break;

	case chip::DeviceLayer::DeviceEventType::kCommissioningWindowClosed:
		// ESP_LOGI(TAG, "Commissioning window closed");
		break;

	// case chip::DeviceLayer::DeviceEventType::kFabricRemoved:
	// {
	// 	// ESP_LOGI(TAG, "Fabric removed successfully");
	// 	// if (chip::Server::GetInstance().GetFabricTable().FabricCount() == 0)
	// 	// {
	// 	// 	chip::CommissioningWindowManager &commissionMgr = chip::Server::GetInstance().GetCommissioningWindowManager();
	// 	// 	constexpr auto kTimeoutSeconds = chip::System::Clock::Seconds16(k_timeout_seconds);
	// 	// 	if (!commissionMgr.IsCommissioningWindowOpen())
	// 	// 	{
	// 	// 		/* After removing last fabric, this example does not remove the Wi-Fi credentials
	// 	// 		 * and still has IP connectivity so, only advertising on DNS-SD.
	// 	// 		 */
	// 	// 		CHIP_ERROR err = commissionMgr.OpenBasicCommissioningWindow(kTimeoutSeconds,
	// 	// 																	chip::CommissioningWindowAdvertisement::kDnssdOnly);
	// 	// 		if (err != CHIP_NO_ERROR)
	// 	// 		{
	// 	// 			ESP_LOGE(TAG, "Failed to open commissioning window, err:%" CHIP_ERROR_FORMAT, err.Format());
	// 	// 		}
	// 	// 	}
	// 	// }
	// 	// break;
	// }

	// case chip::DeviceLayer::DeviceEventType::kFabricWillBeRemoved:
	// 	// ESP_LOGI(TAG, "Fabric will be removed");
	// 	break;

	// case chip::DeviceLayer::DeviceEventType::kFabricUpdated:
	// 	// ESP_LOGI(TAG, "Fabric is updated");
	// 	break;

	// case chip::DeviceLayer::DeviceEventType::kFabricCommitted:
	// 	// ESP_LOGI(TAG, "Fabric is committed");
	// 	break;
	default:
		break;
	}
}

static esp_err_t app_identification_cb(identification::callback_type_t type, uint16_t endpoint_id, uint8_t effect_id,
									   void *priv_data)
{
	// ESP_LOGI(TAG, "Identification callback: type: %d, effect: %d", type, effect_id);
	return ESP_OK;
}

static esp_err_t app_attribute_update_cb(attribute::callback_type_t type, uint16_t endpoint_id, uint32_t cluster_id,
										 uint32_t attribute_id, esp_matter_attr_val_t *val, void *priv_data)
{
	esp_err_t err = ESP_OK;
	Matter matter = (Matter)priv_data;

	if (type == PRE_UPDATE)
	{
		if (endpoint_id == light_endpoint_id)
		{
			if (cluster_id == OnOff::Id)
			{
				// if (attribute_id == OnOff::Attributes::OnOff::Id)
				// {
				// 	err = app_driver_light_set_power(handle, val);
				// }
			}
			else if (cluster_id == LevelControl::Id)
			{
				if (attribute_id == LevelControl::Attributes::CurrentLevel::Id)
				{
					modMessagePostToMachine(matter->the, (uint8_t *)&(val->val.i), sizeof(val->val.i), matterDeliver, priv_data);
				}
			}
			else if (cluster_id == ColorControl::Id)
			{
				// if (attribute_id == ColorControl::Attributes::CurrentHue::Id)
				// {
				// 	err = app_driver_light_set_hue(handle, val);
				// }
				// else if (attribute_id == ColorControl::Attributes::CurrentSaturation::Id)
				// {
				// 	err = app_driver_light_set_saturation(handle, val);
				// }
				// else if (attribute_id == ColorControl::Attributes::ColorTemperatureMireds::Id)
				// {
				// 	err = app_driver_light_set_temperature(handle, val);
				// }
			}
		}
		/* Driver update */
		// app_driver_handle_t driver_handle = (app_driver_handle_t)priv_data;
		// err = app_driver_attribute_update(driver_handle, endpoint_id, cluster_id, attribute_id, val);
	}

	return err;
}

void xs_matter_mark(xsMachine *the, void *it, xsMarkRoot markRoot)
{
	Matter matter = (Matter)it;

	(*markRoot)(the, matter->onChanged);
}

void xs_matter_destructor(void *data)
{
	Matter matter = (Matter)data;

	if (!matter)
		return;

	c_free(matter);
}

void xs_matter_close(xsMachine *the)
{
	Matter matter = (Matter)xsmcGetHostData(xsThis);

	if (matter && xsmcGetHostDataValidate(xsThis, (void *)&xsMatterHooks))
	{
		xsForget(matter->obj);
		xs_matter_destructor(matter);
		xsmcSetHostData(xsThis, NULL);
		xsmcSetHostDestructor(xsThis, NULL);
	}
}

void xs_matter_constructor(xsMachine *the)
{
	Matter matter;
	xsSlot *onChanged;
	xsSlot slot;

	xsmcGet(slot, xsArg(0), xsID_onChanged);
	onChanged = fxToReference(the, &slot);

	if (!onChanged)
		xsRangeError("callback is required");

	matter = (Matter)c_malloc(sizeof(MatterRecord));
	if (!matter)
		xsRangeError("no memory");
	xsmcSetHostData(xsThis, matter);
	matter->the = the;
	matter->onChanged = onChanged;
	matter->obj = xsThis;
	xsRemember(matter->obj);
	xsSetHostHooks(xsThis, (xsHostHooks *)&xsMatterHooks);

	esp_err_t err = ESP_OK;
	node::config_t node_config;
	node_t *node = node::create(&node_config, app_attribute_update_cb, app_identification_cb);

	color_temperature_light::config_t light_config;
	light_config.on_off.on_off = DEFAULT_POWER;
	light_config.on_off.lighting.start_up_on_off = nullptr;
	light_config.level_control.current_level = DEFAULT_BRIGHTNESS;
	light_config.level_control.lighting.start_up_current_level = DEFAULT_BRIGHTNESS;
	light_config.color_control.color_mode = EMBER_ZCL_COLOR_MODE_COLOR_TEMPERATURE;
	light_config.color_control.enhanced_color_mode = EMBER_ZCL_COLOR_MODE_COLOR_TEMPERATURE;
	light_config.color_control.color_temperature.startup_color_temperature_mireds = nullptr;
	endpoint_t *endpoint = color_temperature_light::create(node, &light_config, ENDPOINT_FLAG_NONE, matter);

	/* These node and endpoint handles can be used to create/add other endpoints and clusters. */
	// if (!node || !endpoint)
	// {
	// 	ESP_LOGE(TAG, "Matter node creation failed");
	// }

	light_endpoint_id = endpoint::get_id(endpoint);
	// ESP_LOGI(TAG, "Light created with endpoint_id %d", light_endpoint_id);

	/* Add additional features to the node */
	cluster_t *cluster = cluster::get(endpoint, ColorControl::Id);
	cluster::color_control::feature::hue_saturation::config_t hue_saturation_config;
	hue_saturation_config.current_hue = DEFAULT_HUE;
	hue_saturation_config.current_saturation = DEFAULT_SATURATION;
	cluster::color_control::feature::hue_saturation::add(cluster, &hue_saturation_config);

	/* Matter start */
	err = esp_matter::start(app_event_cb);
	// if (err != ESP_OK)
	// {
	// 	ESP_LOGE(TAG, "Matter start failed: %d", err);
	// }


#if CONFIG_ENABLE_CHIP_SHELL
	esp_matter::console::diagnostics_register_commands();
	esp_matter::console::wifi_register_commands();
	esp_matter::console::init();
#endif
}

static void matterDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	Matter matter = (Matter)refcon;
	int *i = (int * )message;

	xsBeginHost(matter->the);
	xsmcVars(1);
	xsmcSetInteger(xsVar(0), *i);
	xsCallFunction1(xsReference(matter->onChanged), matter->obj, xsVar(0));
	xsEndHost(matter->the);
}