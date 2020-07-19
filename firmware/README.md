# Firmware for Reading the BNKChipV4_E100 README
Started by Nathan Zimmerberg on 19 JUL 2020

**Authors** Nathan Zimmerberg (nhz2@cornell.edu)

Latest Revision: 19 JUL 2020

## Overview

The main goal of the firmware is to:

1. Setup a recording from the BNKChipV4_E100 with a specified framerate, reference electrode voltage(Vref), and number of frames. 
2. Run the recording. Generate the signals to drive the chip and analog to digital converters (ADC). Stream the data from the ADCs into a SD card.
3. Send the final data to the attached computer via USB, along with any errors that occurred.

## Hardware Overview

### Links

Micro controller - [Teensy 4.1](https://www.pjrc.com/teensy/)

Analog to digital converter [AD7866](https://www.analog.com/media/en/technical-documentation/data-sheets/AD7866.pdf)

100 electrode array chip [BNKChipV4_E100 very vague mostly unhelpful paper](https://doi.org/10.1016/j.bios.2012.09.058)

Digital to analog converter [DAC80501](https://www.ti.com/lit/ds/symlink/dac80501.pdf)

[Schematic and PCB layout](https://easyeda.com/nzimmerberg/amp-chip)

![A diagram of the phases of operation in making a single bit](diagrams/ChangingSignalsRecording.png)



