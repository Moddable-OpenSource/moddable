/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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

#include "piuMC.h"

static PiuBoolean SpanEquals(PiuCoordinate* former, PiuCoordinate* current);
static void SpanMinMax(PiuCoordinate* span, PiuCoordinate* min, PiuCoordinate* max);

void PiuRegionNew(xsMachine* the, PiuCoordinate available)
{
	PiuRegion* region;
	xsResult = xsNewHostObject(NULL);
	xsSetHostChunk(xsResult, NULL, sizeof(xsSlot*) + ((1 + available) * sizeof(PiuCoordinate)));
	region = PIU(Region, xsResult);
	(*region)->reference = xsToReference(xsResult);
	(*region)->available = available;
	(*region)->data[0] = 5;
}

PiuBoolean PiuRegionCombine(PiuRegion* region0, PiuRegion* region1, PiuRegion* region2, PiuCoordinate op) 
{
	PiuCoordinate* current0 = (*region0)->data;
	PiuCoordinate* limit0 = current0 + (*region0)->available;
	PiuCoordinate left0, top0, right0, bottom0;	
	PiuCoordinate* span0;	
	PiuCoordinate segmentCount0;	
	
	PiuCoordinate* current1 = (*region1)->data;
	PiuCoordinate* limit1 = current1 + *current1;
	PiuCoordinate left1, top1, right1, bottom1;	
	PiuCoordinate* span1 = NULL;	
	PiuCoordinate segmentCount1 = 0;	
	PiuCoordinate* segment1;	
	PiuCoordinate* segmentLimit1;	
	
	PiuCoordinate* current2 = (*region2)->data;
	PiuCoordinate* limit2 = current2 + *current2;
	PiuCoordinate left2, top2, right2, bottom2;	
	PiuCoordinate* span2 = NULL;	
	PiuCoordinate segmentCount2 = 0;	
	PiuCoordinate* segment2;	
	PiuCoordinate* segmentLimit2;	
	
	PiuCoordinate* former = NULL;	
	PiuCoordinate test, c, y, flag, old, x;
	
	current1++;
	left1 = *current1++;
	top1 = *current1++;
	right1 = left1 + *current1++;
	bottom1 = top1 + *current1++;
	current2++;
	left2 = *current2++;
	top2 = *current2++;
	right2 = left2 + *current2++;
	bottom2 = top2 + *current2++;

	if (current1 == limit1)
		return (op & 1) ? PiuRegionEmpty(region0) : PiuRegionCopy(region0, region2);
	if (current2 == limit2)
		return (op & 2) ? PiuRegionEmpty(region0) : PiuRegionCopy(region0, region1);
	if (op & 3) {
		if ((right2 <= left1) || (right1 <= left2) || (bottom2 <= top1) || (bottom1 <= top2))
			return (op & 2) ? PiuRegionEmpty(region0) : PiuRegionCopy(region0, region1);
		left0 = (right1 > right2) ? right1 : right2;
		right0 = (left1 > left2) ? left2 : left1;
	}
	else {
		left0 = (left1 > left2) ? left2 : left1;
		right0 = (right1 > right2) ? right1 : right2;
	}
	current0 += 5;
	
	if (*current1 < *current2) {
		if (op & 2) {
			while ((current1 < limit1) && (*current1 < *current2)) {
				current1++;
				c = segmentCount1  = *current1++;
				span1 = current1;
				current1 += c;
			}
		}
		else {
			while ((current1 < limit1) && (*current1 < *current2)) {
				y = *current1++;
				c = segmentCount1 = *current1++;
				span1 = current1;
				if (current0 + 2 + c > limit0)
					return 0;
				*current0++ = y;
				span0 = current0;
				*current0++ = c;
				while (c) {
					*current0++ = *current1++;
					c--;
				}
				if (op & 3)
					SpanMinMax(span0, &left0, &right0);
			}
			former = span0;
		}
	}
	else if (*current2 < *current1) {
		if (op & 1) {
			while ((current2 < limit2) && (*current2 < *current1)) {
				current2++;
				c = segmentCount2  = *current2++;
				span2 = current2;
				current2 += c;
			}
		}
		else {
			while ((current2 < limit2) && (*current2 < *current1)) {
				y = *current2++;
				c = segmentCount2  = *current2++;
				span2 = current2;
				if (current0 + 2 + c > limit0)
					return 0;
				*current0++ = y;
				span0 = current0;
				*current0++ = c;
				while (c) {
					*current0++ = *current2++;
					c--;
				}
				if (op & 3)
					SpanMinMax(span0, &left0, &right0);
			}
			former = span0;
		}
	}
	while ((current1 < limit1) && (current2 < limit2)) {
		if ((test = *current1 - *current2) <= 0) {
			y = *current1++;
			segmentCount1 = *current1++;
			span1 = current1;
			current1 += segmentCount1;
		}
		if (test >= 0) {
			y = *current2++;
			segmentCount2 = *current2++;
			span2 = current2;
			current2 += segmentCount2;
		}
		if ((!segmentCount1) && (!segmentCount2)) {
			if (current0 + 2 > limit0)
				return 0;
			*current0++ = y;
			span0 = current0;
			*current0++ = 0;
		}
		else if (!segmentCount1) {
			if (op & 1) {
				if (current0 + 2 > limit0)
					return 0;
				*current0++ = y;
				span0 = current0;
				*current0++ = 0;
			}
			else {
				c = segmentCount2;
				segment2 = span2;
				if (current0 + 2 + c > limit0)
					return 0;
				*current0++ = y;
				span0 = current0;
				*current0++ = c;
				while (c) {
					*current0++ = *segment2++;
					c--;
				}
			}
		}
		else if (!segmentCount2) {
			if (op & 2) {
				if (current0 + 2 > limit0)
					return 0;
				*current0++ = y;
				span0 = current0;
				*current0++ = 0;
			}
			else {
				c = segmentCount1;
				segment1 = span1;
				if (current0 + 2 + c > limit0)
					return 0;
				*current0++ = y;
				span0 = current0;
				*current0++ = c;
				while (c) {
					*current0++ = *segment1++;
					c--;
				}
			}
		}
		else {
			flag = 0;
			old = 0;
			segmentCount0 = 0;
			segment1 = span1;
			segmentLimit1 = segment1 + segmentCount1;
			segment2 = span2;
			segmentLimit2 = segment2 + segmentCount2;
			if (current0 + 2 + segmentCount1 + segmentCount2 > limit0)
				return 0;
			*current0++ = y;
			span0 = current0;
			current0++;
			while ((segment1 < segmentLimit1) && (segment2 < segmentLimit2)) {
				if ((test = *segment1 - *segment2) <= 0) {
					x = *segment1;
					flag ^= 1;
					segment1++;
				}
				if (test >= 0) {
					x = *segment2;
					flag ^= 2;
					segment2++;
				}
				if (op == 4) {
// 					PiuCoordinate xflag = (flag == 3) ? 0 : flag;
// 					PiuCoordinate xold = (old == 3) ? 0 : old;
// 					if ((xflag != xold) && ((xflag == 0) || (xold == 0))) {
// 						segmentCount0++;
// 						*current0++ = x;
// 					}
					if (((flag == 0) || (old == 0)) != ((flag == 3) || (old == 3))) {
						segmentCount0++;
						*current0++ = x;
					}
					old = flag;
				}
				else {
					if ((flag == op) || (old == op)) {
						segmentCount0++;
						*current0++ = x;
					}
					old = flag;
				}
			}
			if ((op & 2) == 0) {
				while (segment1 < segmentLimit1) {
					segmentCount0++;
					*current0++ = *segment1++;
				}
			}
			if ((op & 1) == 0) {
				while (segment2 < segmentLimit2) {
					segmentCount0++;
					*current0++ = *segment2++;
				}
			}
			*span0 = segmentCount0;
		}
		if (SpanEquals(former, span0))
			current0 = span0 - 1;
		else {
			if (op & 3)
				SpanMinMax(span0, &left0, &right0);
			former = span0;
		}
	}
	if ((op & 2) == 0) {
		while (current1 < limit1) {
			y = *current1++;
			span1 = current1;
			c = *current1++;
			if (SpanEquals(former, span1))
				current1 += c;
			else
				current1 -= 2;
			while (current1 < limit1) {
				y = *current1++;
				c = *current1++;
				if (current0 + 2 + c > limit0)
					return 0;
				*current0++ = y;
				span0 = current0;
				*current0++ = c;
				while (c) {
					*current0++ = *current1++;
					c--;
				}
				if (op & 3)
					SpanMinMax(span0, &left0, &right0);
			}
		}
	}
	if ((op & 1) == 0) {
		if (current2 < limit2) {
			y = *current2++;
			span2 = current2;
			c = *current2++;
			if (SpanEquals(former, span2))
				current2 += c;
			else
				current2 -= 2;
			while (current2 < limit2) {
				y = *current2++;
				c = *current2++;
				if (current0 + 2 + c > limit0)
					return 0;
				*current0++ = y;
				span0 = current0;
				*current0++ = c;
				while (c) {
					*current0++ = *current2++;
					c--;
				}
				if (op & 3)
					SpanMinMax(span0, &left0, &right0);
			}
		}
	}
	former = (*region0)->data;
	top0 = former[5];
	bottom0 = *(current0 - 2);
	if ((left0 < right0) && (top0 < bottom0)) {
		former[0] = (PiuCoordinate)(current0 - former);
		former[1] = left0;
		former[2] = top0;
		former[3] = right0 - left0;
		former[4] = bottom0 - top0;
		return 1;
	}
	return PiuRegionEmpty(region0);
}

PiuBoolean PiuRegionCopy(PiuRegion* region, PiuRegion* original)
{
	PiuCoordinate available = (*region)->available;
	PiuCoordinate length = (*original)->data[0];
	if (available >= length) {
		c_memcpy((*region)->data, (*original)->data, length * sizeof(PiuCoordinate));
		return 1;
	}
	return 0;
}

PiuBoolean PiuRegionEmpty(PiuRegion* region)
{
	PiuCoordinate* data = (*region)->data;
	if ((*region)->available < 5)
		return 0;
	data[0] = 5;
	data[1] = 0;
	data[2] = 0;
	data[3] = 0;
	data[4] = 0;
	return 1;
}

void PiuRegionOffset(PiuRegion* region, PiuCoordinate dx, PiuCoordinate dy)
{
	if (dx || dy) {
		PiuCoordinate* data = (*region)->data;
		PiuCoordinate* limit = data + (*data - 2);
		PocoCoordinate count;
		data++;
		*data += dx;
		data++;
		*data += dy;
		data++;
		data++;
		data++;
		while (data < limit) {
			*data += dy;
			data++;
			count = *data++;
			while (count) {
				*data += dx;
				data++;
				count--;
				*data += dx;
				data++;
				count--;
			}
		}
		*data += dy;
	}
}

PiuBoolean PiuRegionRectangle(PiuRegion* region, PiuCoordinate x, PiuCoordinate y, PiuDimension w, PiuDimension h)
{
	if (w && h) {
		PiuCoordinate* data = (*region)->data;
		if ((*region)->available < 11)
			return 0;
		data[0] = 11;
		data[1] = x;
		data[2] = y;
		data[3] = w;
		data[4] = h;
		data[5] = y;
		data[6] = 2;
		data[7] = x;
		data[8] = x + w;
		data[9] = y + h;
		data[10] = 0;
		return 1;
	}
	return PiuRegionEmpty(region);
}

PiuBoolean PiuRegionXOR(PiuRegion* region0, PiuRegion* region1, PiuRegion* region2) 
{
	PiuCoordinate* current0 = (*region0)->data;
	PiuCoordinate* limit0 = current0 + (*region0)->available;
	PiuCoordinate left0, top0, right0, bottom0;	
	PiuCoordinate* span0;	
	PiuCoordinate segmentCount0;	
	
	PiuCoordinate* current1 = (*region1)->data;
	PiuCoordinate* limit1 = current1 + *current1;
	PiuCoordinate left1, right1;	
	PiuCoordinate* span1 = NULL;	
	PiuCoordinate segmentCount1 = 0;	
	PiuCoordinate* segment1;	
	PiuCoordinate* segmentLimit1;	
	
	PiuCoordinate* current2 = (*region2)->data;
	PiuCoordinate* limit2 = current2 + *current2;
	PiuCoordinate left2, right2;	
	PiuCoordinate* span2 = NULL;	
	PiuCoordinate segmentCount2 = 0;	
	PiuCoordinate* segment2;	
	PiuCoordinate* segmentLimit2;	
	
	PiuCoordinate* former = NULL;	
	PiuCoordinate test, c, y, flag, old, x;
	
	current1++;
	left1 = *current1++;
	current1++;
	right1 = left1 + *current1++;
	current1++;
	current2++;
	left2 = *current2++;
	current2++;
	right2 = left2 + *current2++;
	current2++;

	if (current1 == limit1)
		return PiuRegionCopy(region0, region2);
	if (current2 == limit2)
		return PiuRegionCopy(region0, region1);
	left0 = (left1 > left2) ? left2 : left1;
	right0 = (right1 > right2) ? right1 : right2;
	current0 += 5;
	
	if (*current1 < *current2) {
		while ((current1 < limit1) && (*current1 < *current2)) {
			y = *current1++;
			c = segmentCount1 = *current1++;
			span1 = current1;
			if (current0 + 2 + c > limit0)
				return 0;
			*current0++ = y;
			span0 = current0;
			*current0++ = c;
			while (c) {
				*current0++ = *current1++;
				c--;
			}
		}
		former = span0;
	}
	else if (*current2 < *current1) {
		while ((current2 < limit2) && (*current2 < *current1)) {
			y = *current2++;
			c = segmentCount2  = *current2++;
			span2 = current2;
			if (current0 + 2 + c > limit0)
				return 0;
			*current0++ = y;
			span0 = current0;
			*current0++ = c;
			while (c) {
				*current0++ = *current2++;
				c--;
			}
		}
		former = span0;
	}
	while ((current1 < limit1) && (current2 < limit2)) {
		if ((test = *current1 - *current2) <= 0) {
			y = *current1++;
			segmentCount1 = *current1++;
			span1 = current1;
			current1 += segmentCount1;
		}
		if (test >= 0) {
			y = *current2++;
			segmentCount2 = *current2++;
			span2 = current2;
			current2 += segmentCount2;
		}
		if ((!segmentCount1) && (!segmentCount2)) {
			if (current0 + 2 > limit0)
				return 0;
			*current0++ = y;
			span0 = current0;
			*current0++ = 0;
		}
		else if (!segmentCount1) {
			c = segmentCount2;
			segment2 = span2;
			if (current0 + 2 + c > limit0)
				return 0;
			*current0++ = y;
			span0 = current0;
			*current0++ = c;
			while (c) {
				*current0++ = *segment2++;
				c--;
			}
		}
		else if (!segmentCount2) {
			c = segmentCount1;
			segment1 = span1;
			if (current0 + 2 + c > limit0)
				return 0;
			*current0++ = y;
			span0 = current0;
			*current0++ = c;
			while (c) {
				*current0++ = *segment1++;
				c--;
			}
		}
		else {
			flag = 0;
			old = 0;
			segmentCount0 = 0;
			segment1 = span1;
			segmentLimit1 = segment1 + segmentCount1;
			segment2 = span2;
			segmentLimit2 = segment2 + segmentCount2;
			if (current0 + 2 + segmentCount1 + segmentCount2 > limit0)
				return 0;
			*current0++ = y;
			span0 = current0;
			current0++;
			while ((segment1 < segmentLimit1) && (segment2 < segmentLimit2)) {
				if ((test = *segment1 - *segment2) <= 0) {
					x = *segment1;
					flag ^= 1;
					segment1++;
				}
				if (test >= 0) {
					x = *segment2;
					flag ^= 2;
					segment2++;
				}
				if (((flag == 0) || (old == 0)) != ((flag == 3) || (old == 3))) {
					segmentCount0++;
					*current0++ = x;
				}
				old = flag;
			}
			while (segment1 < segmentLimit1) {
				segmentCount0++;
				*current0++ = *segment1++;
			}
			while (segment2 < segmentLimit2) {
				segmentCount0++;
				*current0++ = *segment2++;
			}
			*span0 = segmentCount0;
		}
		if (SpanEquals(former, span0))
			current0 = span0 - 1;
		else
			former = span0;
	}
	while (current1 < limit1) {
		y = *current1++;
		span1 = current1;
		c = *current1++;
		if (SpanEquals(former, span1))
			current1 += c;
		else
			current1 -= 2;
		while (current1 < limit1) {
			y = *current1++;
			c = *current1++;
			if (current0 + 2 + c > limit0)
				return 0;
			*current0++ = y;
			span0 = current0;
			*current0++ = c;
			while (c) {
				*current0++ = *current1++;
				c--;
			}
		}
	}
	if (current2 < limit2) {
		y = *current2++;
		span2 = current2;
		c = *current2++;
		if (SpanEquals(former, span2))
			current2 += c;
		else
			current2 -= 2;
		while (current2 < limit2) {
			y = *current2++;
			c = *current2++;
			if (current0 + 2 + c > limit0)
				return 0;
			*current0++ = y;
			span0 = current0;
			*current0++ = c;
			while (c) {
				*current0++ = *current2++;
				c--;
			}
		}
	}
	former = (*region0)->data;
	top0 = former[5];
	bottom0 = *(current0 - 2);
	if ((left0 < right0) && (top0 < bottom0)) {
		former[0] = (PiuCoordinate)(current0 - former);
		former[1] = left0;
		former[2] = top0;
		former[3] = right0 - left0;
		former[4] = bottom0 - top0;
		return 1;
	}
	return PiuRegionEmpty(region0);
}

PiuBoolean SpanEquals(PiuCoordinate* former, PiuCoordinate* current) 
{
	PiuCoordinate c;
	if (former) {
		if ((c = *former++) != *current++)
			return 0;
		while (c) {
			if (*former++ != *current++)
				return 0;
			c--;
		}
	}
	else if (*current != 0)
		return 0;
	return 1;
}

void SpanMinMax(PiuCoordinate* span, PiuCoordinate* min, PiuCoordinate* max) 
{
	PiuCoordinate c = *span;
	if (c) {
		PiuCoordinate left = span[1];
		PiuCoordinate right = span[c];
		if (*min > left) *min = left;
		if (*max < right) *max = right;
	}
}









