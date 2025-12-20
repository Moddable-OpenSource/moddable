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
#include "applib/graphics/gpath.h"
#include "kernel/pbl_malloc.h"
#include "system/passert.h"

#define MINIMUM_ARRAY_LEN (8)
static void prv_try_allocate_steps(xsMachine *the, PebbleGraphicsContext pgr, const size_t num_steps_increment);
#define TRY_ALLOCATE_STEPS_OR_RETURN_ERROR(num_steps_increment) prv_try_allocate_steps(the, pgr, num_steps_increment)

static void prv_fill_points(GContext *const ctx, GPoint *points, size_t num);
static void prv_add_pt(xsMachine *the, PebbleGraphicsContext pgr, RockyAPIPathStepType step_type);

void path2dResetState(PebbleGraphicsContext pgr)
{
	pgr->path_steps_num = 0;
	c_free(pgr->path_steps);
	pgr->path_steps = C_NULL;
	pgr->path_steps_array_len = 0;
}

void pebble_graphics_context_beginPath(xsMachine *the)
{
	PebbleGraphicsContext pgr = xsmcGetHostData(xsThis);
	path2dResetState(pgr);	
}

void pebble_graphics_context_moveTo(xsMachine *the)
{
	PebbleGraphicsContext pgr = xsmcGetHostData(xsThis);	
	prv_add_pt(the, pgr, RockyAPIPathStepType_MoveTo);
}

void pebble_graphics_context_lineTo(xsMachine *the)
{
	PebbleGraphicsContext pgr = xsmcGetHostData(xsThis);	
	prv_add_pt(the, pgr, RockyAPIPathStepType_LineTo);
}

void pebble_graphics_context_stroke(xsMachine *the)
{
  PebbleGraphicsContext pgr = xsmcGetHostData(xsThis);	
  GContext *const ctx = pgr->ctx;

  GPointPrecise pp = {.x = {0}, .y = {0}};
  bool moved_already = false;

  #define ASSIGN_P(new_p) do { \
    moved_already = true; \
    pp = new_p; \
  } while (0)

  for (size_t i = 0; i < pgr->path_steps_num; i++) {
    RockyAPIPathStep *const step = &pgr->path_steps[i];

    switch (step->type) {
      case RockyAPIPathStepType_MoveTo: {
        ASSIGN_P(step->pt.xy);
        break;
      }
      case RockyAPIPathStepType_LineTo: {
        if (moved_already) {
          graphics_line_draw_precise_stroked(ctx, pp, step->pt.xy);
        }
        ASSIGN_P(step->pt.xy);
        break;
      }
      case RockyAPIPathStepType_Arc: {
        if (moved_already) {
          const GPointPrecise pt_from = gpoint_from_polar_precise(
            &step->arc.center, (uint16_t)step->arc.radius.raw_value, step->arc.angle_start);
          graphics_line_draw_precise_stroked(ctx, pp, pt_from);
        }

        int32_t angle_start = step->arc.angle_start;
        int32_t angle_end = step->arc.angle_end;
        if (step->arc.anti_clockwise) {
          const int32_t t = angle_start;
          angle_start = angle_end;
          angle_end = t;
        }
        while (angle_end < angle_start) {
          angle_end += TRIG_MAX_ANGLE;
        }
        graphics_draw_arc_precise_internal(ctx, step->arc.center, step->arc.radius,
                                           angle_start, angle_end);

        const GPointPrecise pt_to = gpoint_from_polar_precise(
          &step->arc.center, (uint16_t)step->arc.radius.raw_value, step->arc.angle_end);
        ASSIGN_P(pt_to);
        break;
      }
      default:
        WTF;
    }
  }
  #undef ASSIGN_P
}

void pebble_graphics_context_arc(xsMachine *the)
{
  PebbleGraphicsContext pgr = xsmcGetHostData(xsThis);	
  GPointPrecise center;
  center.x = prv_fixed_s3_from_double(xsmcToNumber(xsArg(0)));
  center.y = prv_fixed_s3_from_double(xsmcToNumber(xsArg(1)));
  Fixed_S16_3 radius = prv_fixed_s3_from_double(xsmcToNumber(xsArg(2)));
  double angle_1 = adjustAngle(xsmcToNumber(xsArg(3)));
  double angle_2 = adjustAngle(xsmcToNumber(xsArg(4)));
  const bool anti_clockwise = (xsmcArgc >= 6) ? xsmcToBoolean(xsArg(5)) : false;

  // adjust for coordinate system
  center.x.raw_value -= FIXED_S16_3_HALF.raw_value;
  center.y.raw_value -= FIXED_S16_3_HALF.raw_value;

  TRY_ALLOCATE_STEPS_OR_RETURN_ERROR(1);

  pgr->path_steps[pgr->path_steps_num++] = (RockyAPIPathStep) {
    .type = RockyAPIPathStepType_Arc,
    .arc = (RockyAPIPathStepArc) {
      .center = center,
      .radius = radius,
      .angle_start = angle_1,
      .angle_end = angle_2,
      .anti_clockwise = anti_clockwise,
    },
  };
}

void pebble_graphics_context_rect(xsMachine *the)
{
  PebbleGraphicsContext pgr = xsmcGetHostData(xsThis);	

  TRY_ALLOCATE_STEPS_OR_RETURN_ERROR(5);

  if (xsmcArgc >= 4) {
    GRectPrecise rect;
    rect.origin.x = prv_fixed_s3_from_double(xsmcToNumber(xsArg(0)));
    rect.origin.y = prv_fixed_s3_from_double(xsmcToNumber(xsArg(1)));
    rect.size.w = prv_fixed_s3_from_double(xsmcToNumber(xsArg(2)));
    rect.size.h = prv_fixed_s3_from_double(xsmcToNumber(xsArg(3)));

    grect_precise_standardize(&rect);

    // shift rectangle into coordinate system
    const int16_t half_pt = FIXED_S16_3_HALF.raw_value;
    rect.origin.x.raw_value -= half_pt;
    rect.origin.y.raw_value -= half_pt;

    // special casing for our filling algorithm to match fillRect()
    const int16_t full_pt = FIXED_S16_3_ONE.raw_value;
    const int16_t delta_t = full_pt;
    const int16_t delta_r = full_pt;
    const int16_t delta_b = 0;
    const int16_t delta_l = 0;
    const GVectorPrecise delta_tl = GVectorPrecise(delta_l, delta_t);
    const GVectorPrecise delta_tr = GVectorPrecise(delta_r, delta_t);
    const GVectorPrecise delta_br = GVectorPrecise(delta_r, delta_b);
    const GVectorPrecise delta_bl = GVectorPrecise(delta_l, delta_b);

    const Fixed_S16_3 right = grect_precise_get_max_x(&rect);
    const Fixed_S16_3 bottom = grect_precise_get_max_y(&rect);

    // top left
    pgr->path_steps[pgr->path_steps_num++] = (RockyAPIPathStep) {
      .type = RockyAPIPathStepType_MoveTo,
      .pt.xy = rect.origin,
      .pt.fill_delta = delta_tl,
    };
    // top right
    pgr->path_steps[pgr->path_steps_num++] = (RockyAPIPathStep) {
      .type = RockyAPIPathStepType_LineTo,
      .pt.xy = GPointPrecise(right.raw_value, rect.origin.y.raw_value),
      .pt.fill_delta = delta_tr,
    };
    // bottom right
    pgr->path_steps[pgr->path_steps_num++] = (RockyAPIPathStep) {
      .type = RockyAPIPathStepType_LineTo,
      .pt.xy = GPointPrecise(right.raw_value, bottom.raw_value),
      .pt.fill_delta = delta_br,
    };
    // bottom left
    pgr->path_steps[pgr->path_steps_num++] = (RockyAPIPathStep) {
      .type = RockyAPIPathStepType_LineTo,
      .pt.xy = GPointPrecise(rect.origin.x.raw_value, bottom.raw_value),
      .pt.fill_delta = delta_bl,
    };
    // top left again, to close path
    pgr->path_steps[pgr->path_steps_num++] = (RockyAPIPathStep) {
      .type = RockyAPIPathStepType_LineTo,
      .pt.xy = rect.origin,
      .pt.fill_delta = delta_tl,
    };
  }
}

void pebble_graphics_context_closePath(xsMachine *the)
{
  // lineTo() back to most-recent .moveTo()
  PebbleGraphicsContext pgr = xsmcGetHostData(xsThis);	

  if (pgr->path_steps_num < 2)
    return;

  RockyAPIPathStep *step = &pgr->path_steps[pgr->path_steps_num - 1];

  // if the last step was a moveTo(), there's nothing to do
  if (step->type == RockyAPIPathStepType_MoveTo)
    return;

  TRY_ALLOCATE_STEPS_OR_RETURN_ERROR(1);

  do {
    step--;
    if (step->type == RockyAPIPathStepType_MoveTo) {
      // add a lintTo() at the end
      pgr->path_steps[pgr->path_steps_num++] = (RockyAPIPathStep) {
        .type = RockyAPIPathStepType_LineTo,
        .pt = step->pt,
      };
      break;
    }
  } while (step > &pgr->path_steps[0]);
}

static void prv_fill_points(GContext *const ctx, GPoint *points, size_t num)
{
  if (num < 3) {
    return;
  }

  GPath path = {
    .num_points = num,
    .points = points,
  };
  gpath_draw_filled(ctx, &path);
}

static GPointPrecise prv_point_add_vector_precise(GPointPrecise *pt, GVectorPrecise *v) {
  return GPointPrecise(
    pt->x.raw_value + v->dx.raw_value,
    pt->y.raw_value + v->dy.raw_value);
}

void pebble_graphics_context_fill(xsMachine *the)
{
  PebbleGraphicsContext pgr = xsmcGetHostData(xsThis);	
  GPoint *const points = c_calloc(pgr->path_steps_num, sizeof(*points));
  if (!points) {
    xsUnknownError("too many points to fill");
  }
  size_t points_num = 0;

  #define ADD_P(pt) \
    PBL_ASSERTN(points_num < pgr->path_steps_num); \
    points[points_num++] = GPointFromGPointPrecise( \
      prv_point_add_vector_precise(&(pt).xy, &(pt).fill_delta));

  for (size_t i = 0; i < pgr->path_steps_num; i++) {
    RockyAPIPathStep *const step = &pgr->path_steps[i];
    switch (step->type) {
      case RockyAPIPathStepType_MoveTo: {
        prv_fill_points(pgr->ctx, points, points_num);
        points_num = 0;
        ADD_P(step->pt);
        break;
      }
      case RockyAPIPathStepType_LineTo: {
        ADD_P(step->pt);
        break;
      }
      case RockyAPIPathStepType_Arc: {
		xsUnknownError("fill() does not support arc()");
        goto cleanup;
      }
    }
  }

  prv_fill_points(pgr->ctx, points, points_num);

cleanup:
  c_free(points);
}


static size_t prv_get_realloc_array_len(PebbleGraphicsContext pgr, const size_t required_array_len) {
  size_t len = pgr->path_steps_array_len ?: MINIMUM_ARRAY_LEN;
  while (required_array_len > len) {
    len *= 2;
  }
  return len;
}

static void prv_try_allocate_steps(xsMachine *the, PebbleGraphicsContext pgr, const size_t num_steps_increment) {
  const size_t required_array_len = (pgr->path_steps_num + num_steps_increment);
  if (required_array_len <= pgr->path_steps_array_len) {
    return;
  }
  const size_t new_array_len = prv_get_realloc_array_len(pgr, required_array_len);
  void *new_steps_array = c_realloc(pgr->path_steps,
                                       sizeof(RockyAPIPathStep) * new_array_len);
  if (!new_steps_array) {
    xsUnknownError("can't create more path steps");
  }
  pgr->path_steps = new_steps_array;
  pgr->path_steps_array_len = new_array_len;
}

#define TRY_ALLOCATE_STEPS_OR_RETURN_ERROR(num_steps_increment) prv_try_allocate_steps(the, pgr, num_steps_increment)

void prv_add_pt(xsMachine *the, PebbleGraphicsContext pgr, RockyAPIPathStepType step_type) {
  int argc = xsmcArgc;
  const double raw_x = argc > 0 ? (xsmcToNumber(xsArg(0)) - 0.5) * FIXED_S16_3_FACTOR : 0;
  const double raw_y = argc > 1 ? (xsmcToNumber(xsArg(1)) - 0.5) * FIXED_S16_3_FACTOR : 0;

  if (raw_x < INT16_MIN || raw_x > INT16_MAX || raw_y < INT16_MIN || raw_x > INT16_MAX) {
    xsRangeError("Value out of bounds");
  }

  TRY_ALLOCATE_STEPS_OR_RETURN_ERROR(1);

  pgr->path_steps[pgr->path_steps_num++] = (RockyAPIPathStep) {
    .type = step_type,
    .pt.xy = GPointPrecise((int16_t)raw_x, (int16_t)raw_y),
  };
}
