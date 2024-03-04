/*
 * Copyright (c) 2016-2024  Moddable Tech, Inc.
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

#include "mc.xs.h"
#include "xsHost.h"

void Math_quadEaseIn(xsMachine* the)
{
	double fraction = xsToNumber(xsArg(0));
	xsResult = xsNumber(fraction * fraction);
}

void Math_quadEaseOut(xsMachine* the)
{
	double fraction = xsToNumber(xsArg(0));
	xsResult = xsNumber(-1 * fraction * (fraction - 2));
}

void Math_quadEaseInOut(xsMachine* the)
{
	double fraction = xsToNumber(xsArg(0));
	fraction *= 2;
	if (fraction < 1)
		xsResult = xsNumber(fraction * fraction / 2);
	else {
		fraction -= 1;
		xsResult = xsNumber((fraction * (fraction - 2) - 1) / -2);
	}
}

void Math_cubicEaseIn(xsMachine* the)
{
	double fraction = xsToNumber(xsArg(0));
	xsResult = xsNumber(fraction * fraction * fraction);
}

void Math_cubicEaseOut(xsMachine* the)
{
	double fraction = xsToNumber(xsArg(0));
	double f = fraction - 1;
	xsResult = xsNumber(f * f * f + 1);
}

void Math_cubicEaseInOut(xsMachine* the)
{
	double fraction = xsToNumber(xsArg(0));
	fraction *= 2;
	if (fraction < 1)
		xsResult = xsNumber(fraction * fraction * fraction / 2);
	else {
		fraction -= 2;
		xsResult = xsNumber((fraction * fraction * fraction + 2) / 2);
	}
}

void Math_quartEaseIn(xsMachine* the)
{
	double fraction = xsToNumber(xsArg(0));
	xsResult = xsNumber(fraction * fraction * fraction * fraction);
}

void Math_quartEaseOut(xsMachine* the)
{
	double fraction = xsToNumber(xsArg(0));
	double f = fraction - 1;
	xsResult = xsNumber(-1 * (f * f * f * f - 1));
}

void Math_quartEaseInOut(xsMachine* the)
{
	double fraction = xsToNumber(xsArg(0));
	fraction *= 2;
	if (fraction < 1)
		xsResult = xsNumber(fraction * fraction * fraction * fraction / 2);
	else {
		fraction -= 2;
		xsResult = xsNumber((fraction * fraction * fraction * fraction - 2) / -2);
	}
}

void Math_quintEaseIn(xsMachine* the)
{
	double fraction = xsToNumber(xsArg(0));
	xsResult = xsNumber(fraction * fraction * fraction * fraction * fraction);
}

void Math_quintEaseOut(xsMachine* the)
{
	double fraction = xsToNumber(xsArg(0));
	double f = fraction - 1;
	xsResult = xsNumber(f * f * f * f * f + 1);
}

void Math_quintEaseInOut(xsMachine* the)
{
	double fraction = xsToNumber(xsArg(0));
	fraction *= 2;
	if (fraction < 1)
		xsResult = xsNumber(fraction * fraction * fraction * fraction * fraction / 2);
	else {
		fraction -= 2;
		xsResult = xsNumber((fraction * fraction * fraction * fraction * fraction + 2) / 2);
	}
}

void Math_sineEaseIn(xsMachine* the)
{
	double fraction = xsToNumber(xsArg(0));
	xsResult = xsNumber(-1 * cos(fraction * (C_M_PI / 2)) + 1);
}

void Math_sineEaseOut(xsMachine* the)
{
	double fraction = xsToNumber(xsArg(0));
	xsResult = xsNumber(sin(fraction * (C_M_PI / 2)));
}

void Math_sineEaseInOut(xsMachine* the)
{
	double fraction = xsToNumber(xsArg(0));
	xsResult = xsNumber(-1.0 / 2.0 * (cos(C_M_PI * fraction) - 1));
}

void Math_circularEaseIn(xsMachine* the)
{
	double fraction = xsToNumber(xsArg(0));
	xsResult = xsNumber(-1 * (sqrt(1 - fraction * fraction) - 1));
}

void Math_circularEaseOut(xsMachine* the)
{
	double fraction = xsToNumber(xsArg(0));
	fraction -= 1;
	xsResult = xsNumber(sqrt(1 - fraction * fraction));
}

void Math_circularEaseInOut(xsMachine* the)
{
	double fraction = xsToNumber(xsArg(0));
	fraction *= 2;
	if (fraction < 1)
		xsResult = xsNumber((sqrt(1 - fraction * fraction) - 1) / -2);
	else {
		fraction -= 2;
		xsResult = xsNumber((sqrt(1 - fraction * fraction) + 1) / 2);
	}
}

void Math_exponentialEaseIn(xsMachine* the)
{
	double fraction = xsToNumber(xsArg(0));
	xsResult = xsNumber((fraction == 0) ? 0 : pow(2, 10 * (fraction - 1)));
}

void Math_exponentialEaseOut(xsMachine* the)
{
	double fraction = xsToNumber(xsArg(0));
	xsResult = xsNumber((fraction == 1) ? 1 : (-pow(2, -10 * fraction) + 1));
}

void Math_exponentialEaseInOut(xsMachine* the)
{
	double fraction = xsToNumber(xsArg(0));
	if (fraction == 0) 
		xsResult = xsNumber(0);
	else if (fraction == 1) 
		xsResult = xsNumber(1);
	else {
		fraction *= 2;
		if (fraction < 1)
			xsResult = xsNumber(pow(2, 10 * (fraction - 1)) / 2);
		else
			xsResult = xsNumber((-pow(2, -10 * --fraction) + 2) / 2);
	}
}

void Math_backEaseIn(xsMachine* the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	double fraction = xsToNumber(xsArg(0));
	double s = (c > 1) ? xsToNumber(xsArg(1)) : 1.70158;
	xsResult = xsNumber(fraction * fraction * ((s + 1) * fraction - s));
}

void Math_backEaseOut(xsMachine* the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	double fraction = xsToNumber(xsArg(0));
	double s = (c > 1) ? xsToNumber(xsArg(1)) : 1.70158;
	fraction -= 1;
	xsResult = xsNumber(fraction * fraction * ((s + 1) * fraction + s) + 1);
}

void Math_backEaseInOut(xsMachine* the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	double fraction = xsToNumber(xsArg(0));
	double s = (c > 1) ? xsToNumber(xsArg(1)) : 1.70158;
	s *= 1.525;
	fraction *= 2;
	if (fraction < 1)
		xsResult = xsNumber((fraction * fraction * (s + 1) * fraction - s) / 2);
	else {
		fraction -= 2;
		xsResult = xsNumber((fraction * fraction * ((s + 1) * fraction + s) + 2) / 2);
	}
}

static double bounce(double fraction)
{
	if (fraction < (1 / 2.75))
		fraction = 7.5625 * fraction * fraction;
	else if (fraction < (2 / 2.75)) {
		fraction -= (1.5 / 2.75);
		fraction = 7.5625 * fraction * fraction + 0.75;
	}
	else if (fraction < (2.5 / 2.75)) {
		fraction -= (2.25 / 2.75);
		fraction = 7.5625 * fraction * fraction + 0.9375;
	}
	else {
		fraction -= (2.625 / 2.75);
		fraction = 7.5625 * fraction * fraction + 0.984375;
	}
	return fraction;
}

void Math_bounceEaseIn(xsMachine* the)
{
	double fraction = xsToNumber(xsArg(0));
	xsResult = xsNumber(1 - bounce(1 - fraction));
}

void Math_bounceEaseOut(xsMachine* the)
{
	double fraction = xsToNumber(xsArg(0));
	xsResult = xsNumber(bounce(fraction));
}

void Math_bounceEaseInOut(xsMachine* the)
{
	double fraction = xsToNumber(xsArg(0));
	if (fraction < 0.5)
		xsResult = xsNumber((1 - bounce(1 - (fraction * 2))) / 2);
	else
		xsResult = xsNumber(bounce(fraction * 2 - 1) / 2 + 0.5);
}

void Math_elasticEaseIn(xsMachine* the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	double fraction = xsToNumber(xsArg(0));
	if (fraction == 0)	
		xsResult = xsNumber(0);
	else if (fraction == 1) 
		xsResult = xsNumber(1);
	else {
		double a = (c > 1) ? xsToNumber(xsArg(1)) : 0;
		double p = (c > 2) ? xsToNumber(xsArg(2)) : 0;
		double s;
		if (!p) 
			p = 0.3;
		if (a < 1) {
			a = 1;
			s = p / 4;
		}
		else 
			s = p / (2 * C_M_PI) * asin(1 / a);
		fraction -= 1;
		xsResult = xsNumber(-(a * pow(2, 10 * fraction) * sin( (fraction - s) * (2 * C_M_PI) / p )));
	}
}

void Math_elasticEaseOut(xsMachine* the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	double fraction = xsToNumber(xsArg(0));
	if (fraction == 0)	
		xsResult = xsNumber(0);
	else if (fraction == 1) 
		xsResult = xsNumber(1);
	else {
		double a = (c > 1) ? xsToNumber(xsArg(1)) : 0;
		double p = (c > 2) ? xsToNumber(xsArg(2)) : 0;
		double s;
		if (!p) 
			p = 0.3;
		if (a < 1) { 
			a = 1;
			s = p / 4;
		} 
		else
			s = p / (2 * C_M_PI) * asin(1 / a);
		xsResult = xsNumber(a * pow(2, -10 * fraction ) * sin((fraction - s) * (2 * C_M_PI) / p ) + 1);
	}
}

void Math_elasticEaseInOut(xsMachine* the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	double fraction = xsToNumber(xsArg(0));
	if (fraction == 0) 
		xsResult = xsNumber(0);
	else if (fraction == 1) 
		xsResult = xsNumber(1);
	else {
		double a = (c > 1) ? xsToNumber(xsArg(1)) : 0;
		double p = (c > 2) ? xsToNumber(xsArg(2)) : 0;
		double s;
		fraction *= 2;
		if (!p)
			p = 0.45;
		if (a < 1) { 
			a = 1;
			s = p / 4;
		} 
		else 
			s = p / (2 * C_M_PI) * asin(1 / a);
		fraction -= 1;
		if (fraction < 0)
			xsResult = xsNumber((a * pow(2, 10 * fraction) * sin((fraction - s) * (2 * C_M_PI) / p )) / -2);
		else	
			xsResult = xsNumber(a * pow(2, -10 * fraction) * sin((fraction - s) * 2 * C_M_PI / p ) / 2 + 1);
	}
}
