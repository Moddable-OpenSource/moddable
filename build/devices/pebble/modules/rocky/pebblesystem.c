/*
 * Copyright (c) 2025  Moddable Tech, Inc.
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
#include "pebblegraphics.h"

#include "mc.xs.h"      // for xsID_ values

#include "system/logging.h"
#include "resource/resource_ids.auto.h"
#include "font_resource_table.auto.h"		//@@ normally included by system_resource, but not using that here
#include "resource/resource_storage_impl.h"

static void *gSystemResources;
static xsUnsignedValue gSystemResourcesLength;
 
typedef struct {
	uint32_t offset;
	uint32_t length;
} Resource;

static bool prv_get_resource(uint32_t resource_id, Resource *res);

void pebble_system_setResources(xsMachine *the)
{
	xsmcGetBufferReadable(xsArg(0), &gSystemResources, &gSystemResourcesLength);
}

void pebble_system_getResource(xsMachine *the)
{
	int id = xsmcToInteger(xsArg(0));
	Resource res;
	if (!prv_get_resource(id, &res))
		xsUnknownError("not found");

	xsSlot tmp;
	xsResult = xsNewHostObject(C_NULL);
	xsmcSetHostBuffer(xsResult, res.offset + (uint8_t *)gSystemResources, res.length);
	xsmcSetInteger(tmp, res.length);
	xsmcDefine(xsResult, xsID_byteLength, tmp, xsDontDelete | xsDontSet);
	xsmcPetrifyHostBuffer(xsResult);
}

void pebble_sysem_gbitmap_destructor(void *data)
{
	GBitmap *bitmap = (GBitmap *)data;
	if (bitmap)
		gbitmap_deinit(bitmap);
}

void pebble_system_getBitmap(xsMachine *the)
{
	int id = xsmcToInteger(xsArg(0));
	GBitmap bitmap;

	if (!gbitmap_init_with_resource(&bitmap, id))
		xsUnknownError("not found");

	xsResult = xsNewHostObject(pebble_sysem_gbitmap_destructor);
	xsmcSetHostChunk(xsResult, (void *)&bitmap, sizeof(bitmap));
}

/*
	adapted  from emscripten / emscripten_resources.c
*/

static uint32_t prv_read(uint32_t offset, void *data, size_t num_bytes) {
	if ((offset + num_bytes) >  gSystemResourcesLength)
		return 0;

	c_memmove(data, offset + (uint8_t *)gSystemResources, num_bytes);
	return num_bytes;
}

static void prv_get_manifest(ResourceManifest *manifest) {
  prv_read(0, manifest, sizeof(ResourceManifest));
}

static bool prv_get_table_entry(ResTableEntry *entry, uint32_t index) {
  uint32_t addr = sizeof(ResourceManifest) + index * sizeof(ResTableEntry);
  return prv_read(addr, entry, sizeof(ResTableEntry));
}

static bool prv_get_resource(uint32_t resource_id, Resource *res) {
  *res = (Resource){
    .length = 0,
    .offset = 0,
  };

  ResourceManifest manifest;
  prv_get_manifest(&manifest);

  if (resource_id > manifest.num_resources) {
      PBL_LOG(LOG_LEVEL_DEBUG, "resource id %d > %d is out of range\n", (int)resource_id,
                  (int)manifest.num_resources);
    return false;
  }

  ResTableEntry entry;
  if (!prv_get_table_entry(&entry, resource_id - 1)) {
    PBL_LOG(LOG_LEVEL_DEBUG, "%s: Failed to read table entry for %d\n", __FILE__, (int)resource_id);
    return false;
  }

  if ((entry.resource_id != resource_id) ||
      (entry.length == 0)) {
    // empty resource
    PBL_LOG(LOG_LEVEL_DEBUG, "%s: Invalid resource for %d\n", __FILE__, (int)resource_id);
    return false;
  }

  res->offset = SYSTEM_STORE_METADATA_BYTES + entry.offset;
  res->length = entry.length;

  return true;
}

size_t prv_resources_read(ResAppNum app_num,
                          uint32_t resource_id,
                          uint32_t offset,
                          uint8_t *buf,
                          size_t num_bytes) {
//  if (offset > INT_MAX || num_bytes > INT_MAX) {
//    return 0;
//  }

//  EmxCustomResource *custom_res = NULL;
//  if (app_num != SYSTEM_APP && (custom_res = prv_custom_resource_get(resource_id))) {
//    return custom_res->read(offset, buf, num_bytes);
//  }

  Resource resource = {};
  if (!prv_get_resource(resource_id, &resource)) {
    return 0;
  }

  if (offset + num_bytes > resource.length) {
    if (offset >= resource.length) {
      // Can't recover from trying to read from beyond the resource. Read nothing.
      printf("%s: Reading past the end of the resource!\n", __FILE__);
      return 0;
    }
    num_bytes = resource.length - offset;
  }

  return prv_read(offset + resource.offset, buf, num_bytes);
}

#if 0

#define NUM_SYSTEM_FONTS (ARRAY_LENGTH(s_font_resource_keys))
#define ARRAY_LENGTH(array) (sizeof((array))/sizeof((array)[0]))

//@@ this fails if font_key is NULL (fonts_get_fallback_font
GFont sys_font_get_system_font(const char *font_key)
{
  static FontInfo s_system_fonts_info_table[NUM_SYSTEM_FONTS + 1] = {};

  for (int i = 0; i < (int) NUM_SYSTEM_FONTS; ++i) {
    if (0 == strcmp(font_key, s_font_resource_keys[i].key_name)) {
      FontInfo *fontinfo = &s_system_fonts_info_table[i];
      uint32_t resource = s_font_resource_keys[i].resource_id;
      // if the font has not been initialized yet
      if (!fontinfo->loaded) {
        if (!text_resources_init_font(SYSTEM_APP,
            resource, 0, &s_system_fonts_info_table[i])) {
          // Can't initialize the font for some reason
          return C_NULL;
        }
      }
      return &s_system_fonts_info_table[i];
    }
  }
  return C_NULL;
}

ResAppNum sys_get_current_resource_num(void)
{
  return 0;
}

size_t sys_resource_size(ResAppNum app_num, uint32_t id)
{
	Resource resource;
	if (!prv_get_resource(id, &resource))
		return 0;
	return resource.length;
}

size_t sys_resource_load_range(ResAppNum app_num, uint32_t id, uint32_t start_bytes, uint8_t *buffer, size_t num_bytes)
{
	return prv_resources_read(app_num, id, start_bytes, buffer, num_bytes);
}

bool sys_resource_is_valid(ResAppNum app_num, uint32_t resource_id)
{
	return true;
}

uint32_t sys_resource_get_and_cache(ResAppNum app_num, uint32_t resource_id)
{
	return resource_id;
}

void sys_font_reload_font(FontInfo *fontinfo)
{
  text_resources_init_font(fontinfo->base.app_num, fontinfo->base.resource_id,
      fontinfo->extension.resource_id, fontinfo);
}

bool applib_resource_is_mmapped(const void *bytes)
{
	return ((uint8_t *)gSystemResources <= bytes) && (bytes < (((uint8_t *)gSystemResources) + gSystemResourcesLength));
}

void applib_resource_munmap_or_free(void *bytes)
{
	if (applib_resource_is_mmapped(bytes))
		return;

	c_free(bytes);
}

void *applib_resource_mmap_or_load(ResAppNum app_num, uint32_t id,
                                   size_t offset, size_t num_bytes, bool used_aligned)
{
	Resource resource;
	if (!prv_get_resource(id, &resource))
		return C_NULL;

	if ((num_bytes + offset) > (resource.offset + resource.length))
		return C_NULL;

	return ((uint8_t *)gSystemResources) + resource.offset + offset;
/*
    void *result = c_malloc(num_bytes + (used_aligned ? 7 : 0));
    if (!result || sys_resource_load_range(app_num, id, offset,
                                           result, num_bytes) != num_bytes) {
      c_free(result);
      return NULL;
    }
    return result;
*/
}

#endif