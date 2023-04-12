/*
 * Copyright (c) 2023  Moddable Tech, Inc.
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

/*
    Renesas X9C Series - Digital Potentiometers - X9C102, X9C103, X9C104, X9C503
    https://www.renesas.com/us/en/products/analog-products/data-converters/digital-controlled-potentiometers-dcp/x9c104-digitally-controlled-potentiometer-xdcp
    Datasheet: https://www.renesas.com/us/en/document/dst/x9c102-x9c103-x9c104-x9c503-datasheet
    Reference Driver: https://github.com/lucyamy/LapX9C10X/blob/main/src/LapX9C10X.cpp
*/


import Timer from "timer"


class X9C
{
    #incrementPin;
    #upDownPin;
    #csNVRAMWritePin;
    #currentWiper;
    #minWiperValue = 0;
    #maxWiperValue = 99;

    constructor(options)
    {   
        const { increment, upDown, csNVRAMWrite } = options;
        try
        {
            this.#incrementPin = new increment.io({
                mode: increment.io.Output,
                pin: increment.pin
            });
            this.#upDownPin = new upDown.io({
                mode: upDown.io.Output,
                pin: upDown.pin
            });
            this.#csNVRAMWritePin = new csNVRAMWrite.io({
                mode: csNVRAMWrite.io.Output,
                pin: csNVRAMWrite.pin
            });
        }
        catch (e)
        {
            this.close();
            throw e;
        }

        this.#reset(1);
    }

    #reset(value)
    {
        if (value > (this.#maxWiperValue / 2) | 0)
        {
            this.#currentWiper = this.#maxWiperValue;
            this.#setWiper(this.#minWiperValue);
        }
        else
        {
            this.#currentWiper = this.#minWiperValue;
            this.#setWiper(this.#maxWiperValue);
        }

        this.#setWiper(value);

    }

    #setWiper(value)
    {
        let steps;
        if (!this.#inRange(value))
            throw new RangeError(`wiper range is 0-99`);

        if (value > this.#currentWiper)
        {
            this.#upDownPin.write(1);  
            steps = value - this.#currentWiper;
        } else if (value < this.#currentWiper)
        {
            this.#upDownPin.write(0);
            steps = this.#currentWiper - value;
        }
        else
        {
            return;
        }
        this.#incrementPin.write(1);
        this.#csNVRAMWritePin.write(0);

        for (let i = 0; i < steps; i++)
            this.#pulseInc();
        this.#csNVRAMWritePin.write(0);

        this.#currentWiper = value;
    }

    #pulseInc() 
    {
        this.#incrementPin.write(1);
        Timer.delay(1);
        this.#incrementPin.write(0);
        Timer.delay(1);
    }

    #inRange(value)
    {
        if (value < this.#minWiperValue || value > this.#maxWiperValue)
            return false
        return true
    }


    configure(options)
    {
        // configuration has been implemented this way for the following reasons:
        // * if a user wants to reset, it only makes sense for it to happen first
        //   as changing the wiper value before a reset would be overriden anyway
        // * an offset should only be performed last because either a reset or directly
        //   setting the wiper value after an offset would override the offset
        if (undefined !== options.reset)
            this.#reset(options.reset);
        
        if (undefined !== options.wiper)
        {
            let value = options.wiper;
            this.#setWiper(value);
        }
            
        if (undefined !== options.offset)
        {
            let value = this.#currentWiper + options.offset;
            if (!this.#inRange(value))
                throw new RangeError(`wiper range is 0-99`);
            this.#setWiper(value);
        }
    }

    get wiper()
    {
        return this.#currentWiper;
    }

    close()
    {
        this.#incrementPin?.close();
        this.#incrementPin = undefined;
        this.#upDownPin?.close();
        this.#upDownPin = undefined;
        this.#csNVRAMWritePin?.close();
        this.#csNVRAMWritePin = undefined;
        this.#currentWiper = undefined;
    }

}
export default X9C;
