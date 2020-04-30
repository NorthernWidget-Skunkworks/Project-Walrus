# The Walrus: A Pressure and Temperature Sensor for Water-Level and Barometric Measurements

![Walrus pup; photo by Max Smith](Documentation/images/WalrusPup_MaxSmith.jpg)

***Our Walrus sensor, like this Walrus pup, is equally at home on land or in the water.*** *[Photo by Max Smith](https://commons.wikimedia.org/wiki/File:Walrus_-_Kamogawa_Seaworld_-_pup_-1.jpg)*

## Overview

The **Walrus** is an open-source sensor that measures temperature and pressure. It may be used in the air as a barometer or underwater as part of a system -- combined with barometric compensation -- to obtain changes in water level.

![Walrus HardMount v0.2.0](Documentation/images/Walrus_HardMount_v020_top_annotated_20200427.png)
***Hard-mount variant of the Walrus.*** Major components are labeled. The pads to attach the watertight plug for the cable (i.e. the "hard mount"), on left, are currently unpopulated. These may be used for this kind of connector, or may have wires soldered directly to them before being embedded in an epoxy or other potting (i.e., waterproofing and encapsulating) compound.

### Technical specifications

#### Size

Dimensions: 42.0 x 12.7 cm

>> ADD MASS once I get access to a lab balance again <<

#### Operating ranges

**Power**
* Input Voltage: 3.3 to 5 V DC
* Logic Voltage: 0 to Vcc (Will operate at same logic level as input power supply)

**Sensors**
* Temperature: -40 to +85 C
* Pressure, typical: 0 to 2 atm (i.e. barometer to 10 m water depth)
* Pressure, maximum, untested: 0 to 30 atm (i.e. barometer to 290 m water depth)
* Cable length: 3 m (I2C) or 1000 m (RS-485).

The pressure operating range depends on the sensor element used. The [**MS5803-02BA**](https://www.te.com/commerce/DocumentDelivery/DDEController?Action=srchrtrv&DocNm=MS5803-02BA&DocType=Data+Sheet&DocLang=English) is the standard model that we use for pressure measurements of up to 10 m

The pressure transducer contains an internal temperature sensor, and a secondary (and more accurate) external temperature sensor is also mounted on the *Walrus*. This external sensor is the [MCP9808](http://www.microchip.com/mymicrochip/filehandler.aspx?ddocname=en556196) in an 8-MSOP package. This sensor has a range of -40 to 125 C.

RS-485 communications allow for a signal to be transmitted over long distances, but require a device that can decode this signal, such as our [Project-Longbow](https://github.com/NorthernWidget-Skunkworks/Project-Longbow).

#### Resolution

Pressure: +/- 1 mm water (0.1 mbar)
Temperature sensor within pressure transducer (typical): +/- 2.0 C
External temperature sensor (typical): +/- 0.5 C


## Circuit board and assembly

The Walrus comes in two forms:
* [Large form factor, through-hole mount](https://github.com/NorthernWidget-Skunkworks/Project-TPDH-Walrus/tree/master/Hardware_LargePTH): This is the legacy board with support for I2C and full-duplex RS-485 communication
* [Smaller form factor, solder pads for wires or a waterproof plug](https://github.com/NorthernWidget-Skunkworks/Project-TPDH-Walrus/tree/master/Hardware_HardMount): This current version makes better use of board space and utilizes a half-duplex RS-485 bus in addition to an I2C bus.

### Files

The files for production are located in folders starting with the word "Hardware". These folders contain:

* "\*.brd" and "\*.sch" files. These are the circuit board and its schematic for use with the EAGLE CAD software, which you can download for free: https://www.autodesk.com/products/eagle/free-download
* A "\*.csv" file that contains the "BoM" -- the bill of materials. This lists all required parts to construct the board
* A ZIP archive containing a set of "Gerber" files. These are the files that machines use to create a circuit board and a stencil to help you to apply solder paste (see below)

### Circuit board specifications

* 2-layer copper
* Minimum trace width: 0.2 mm
* Single-sided
* Surface mount components

All Walrus boards are **single-sided** and use only **surface-mount components** for rapid fabrication using standard equipment.

### Producing circuit boards

We strongly recommend that circuit boards be printed by a reputible manufacturer. Many such industries exist in China, India, and developing or developed countries.

### Populating circuit boards

Before you can start to populate the circuit board, meaning adding components to it, you will need solder paste and a stencil. If you have access to a laser cutter, you can create your own stencil (https://learn.adafruit.com/smt-manufacturing/laser-cut-stencils). If you do not, the company that prints the circuit boards should be able to provide one. If all else fails, you can try to cut out a stencil with a small knife, or use a solder-paste syringe, though both of these methods can lead to difficulties (such as bridged connections). Additional information on how to stencil can be found here: https://www.sparkfun.com/tutorials/58.

To help you in using the stencil, we provide STL files for 3D-printable plates in each of the hardware directories. These plates hold the board in place and provide a level surface on which to use the stencil.

>> ADD PLATE FOR HARDMOUNT <<

After this, you can use a tweezers and steady hands, or a pick-and-place machine, to gently place each component over the right pads, and in the right orientation. Do not worry if they are slightly misaligned; the surface tension of the solderi, once it is melted, will help to rotate the components into place.

### Reflowing circuit boards

"Reflowing" is the process of heating solder until it flows and attaches to components. A basic introduction is here: https://learn.sparkfun.com/tutorials/electronics-assembly/reflow

There are four major ways to reflow:
1. Using a professional oven (see link above)
2. With a customized toaster oven: See https://www.sparkfun.com/tutorials/60, and a basic internet search will show you many, many other ideas
3. You can also use a simple electrical skillet -- the kind that you might use to make some food. https://www.sparkfun.com/tutorials/59
4. Using a hot-air rework station; this is a bit time-intensive, but lets you control the heat, and I often use it for small boards. https://learn.sparkfun.com/tutorials/how-to-use-a-hot-air-rework-station/all

### Debugging circuit boards

After reflow is complete, check if everything worked. Do you have any jumped connections? Any remaining solder paste that hasn't been reflowed? This can be the trickiest part, and one for which access to a multimeter is essential. An oscilloscope and a digital logical analyzer can help too. Debugging electronic circuit boards is a process that requires significant skill and practice, so be sure that you can work with someone with some expertise. I would suggest an electrical engineering student or professor at a local university.


## Wiring and pinout

"Pin" is the name of the pin as labeled on the board. "Wire" is the color of the AlphaWire cable that we use; you may use something different based on what manufacturers exist where you are; that's okay, just record the color convention! (We sometimes use another cable in which we use a white wire for SDA and a green wire for SCL, but have found that AlphaWire brand works best.)

>> Add image of cable ends (optional) and find how to integrate with housing section; cabling is a distinct topic, but the step of attaching the cable lies within the housing-construction step <<

For I2C Mode:

| Pin | Wire | Name |
|-----|------|------|
|V+|Red|Vcc|		
|V-|Black|GND|
|SIG1|Brown|SDA|
|SIG2|Orange|SCL|

For RS-485 Mode:

| Pin | Wire | Name |
|-----|------|------|
|V+|Red|Vcc|		
|V-|Black|GND|
|SIG1|Brown|A|
|SIG2|Orange|B|


## Upload the firmware

Using an AVR ISP, upload the following firmware programs from the "Firmware" directory in this repository using the Arduino software:
* Large form factor PTH board, I2C mode: `Longbow_2BA_0x6B`
* Small form factor hard-mount board: `Longbow_2BA_0x6B_HalfDuplex`
>> @bschulz1701: check if this is correct <<

>> Link to resource for general Arduino install and upload, as well as for bootloading and AVR ISP, perhaps in Resnik repo, or in a more general Wiki <<

## Housing

![Walrus PTH v0.0.0 with enclosure](Documentation/images/Walrus_PTH_v000_with_enclosure_and_scale_cleaned_background_20200428.png)

***Walrus v0.0.0 PTH and its housing.*** *Left: 3D-printed top of the housing; the slits are to prevent large material from hitting the sensor. Center: the bottom of the housing. Self-tapping holes are to screw the board onto this housing. The cable entering the housing through a tightly-fitting hole on the "south" side of the image has not yet been stripped to expose the ends of the individual wires. A different variant of the housing is longer to allow the user to strip more of the cable if desired while maintaining the cable jacketing in contact with the hole. Top right: Walrus v0.0.0. This fully-functional initial version is significantly larger than the current unit. Note the pressure transducer (white circle) at top left, and the fact that this has pins and hardware for a full-duplex RS-485 connection. Bottom right: U.S. penny for scale; 19.05 mm (0.75 inch) diameter.*

>> SECOND IMAGE WITH HARDMOUNT <<

### How to build

#### Materials and components required

* 3D printer filament [type?]
* 4-conductor cable [diameter/type?]
* Two-part epoxy [information here?]
* Screws [3/size?]

#### 3D printing the housing

Subfolders of the "3Dprint" directory contain "STL" files that you can use with a program called "[Slic3r](https://slic3r.org/)" to create 3-D printable parts for your housing. Assuming that you have a Prusa 3D printer, you will want to download the [Prusa variant of Slic3r](https://www.prusa3d.com/prusaslicer/). This comes with the appropriate setup files and information to help you configure the printer and print your parts.

>> TO DO: Options for Hard Mount with and without the plug. Also: part number for plug. <<

#### Cabling

>> EXAMPLES WITH CABLE COLORS <<

>> EXAMPLES WITH PTH AND HARDMOUNT, OR JUST HARDMOUNT? <<

#### Potting

After creating the housing, pot the boards in 2-part epoxy. MAKE SURE that a cable of the desired length is attached beforehand! Be careful to fully cover everything except for the white inside membrane on the pressure transducer. If epoxy touches this, the board is likely rendered useless.

>> Parts list, incl. screws, filament, 3D printer info, etc. <<
>> Walk through assembly with images <<
>> Should I get a Prusa printer for home, or... <<

## Deployment

### Wiring to logger

Pass the cable through the housing to the logger, ideally using a cable gland that you can tighten. Attach:
* V- to GND or V-
* V+ to a positive voltage source between 3.3 and 5 V

If using the sensor in I2C mode, simply attach:
* SIG1 to SDA
* SIG2 to SCL

If using the sensor in RS-485 mode, attach the components as follows:
>> @bschulz1701: which is A and B in terms Rx/Tx +/- on the Longbow? <<

Then use another set of four wires to connect the Longbow to the logger:
* GND to GND
* Vin to a 3.3 to 5 V power source
* SDA to SDA
* SCL to SCL

Ensure that the cable is appropriately strain relieved.

>> Link to Margay / Resnik for more information on building logger housing <<

Once this is done, you can deploy your sensor in the river, lake, or atmosphere that you hope to measure. Remember that you must correct all pressure measuremennts for barometric pressure in order to recover a water level. If the water is saline or heavily sediment laden, you must know the salinity or sediment-adjusted density as well.


<br>
<a rel="license" href="http://creativecommons.org/licenses/by-sa/4.0/"><img alt="Creative Commons License" style="border-width:0" src="https://i.creativecommons.org/l/by-sa/4.0/88x31.png" /></a><br />This work is licensed under a <a rel="license" href="http://creativecommons.org/licenses/by-sa/4.0/">Creative Commons Attribution-ShareAlike 4.0 International License</a>.

