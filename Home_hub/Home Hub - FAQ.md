#### What is the purpose of Farm Sense
Farm Sense produces remote monitoring equipment for farms and industrial applications. Our base product is Home Hub which receives remote sensor data via LoRa.  Our mission is to give peace of mind knowing they will be alerted if a sensor detects a scenario they need to know about and address.  We don't want to make our customers life more complicated by technology, but to keep them informed and give them peace of mind of their farm or industrial facility. 
#### What is the purpose of Home Hub
Farm Sense Home Hub serves as the final receiver for all peripheral sensor devices. Home Hub does not require internet access as it creates its own localized web site for the customer. Home Hub will serve as the local server to display sensor indication on intranet based web page customer can access via ESP32 wifi, or home network. User may choose to send data to Farm Sense if internet is available. Data storage will be capped on FarmSense server at 10Mb per free account. Users sending data to internet - data will be in its final raw formats to reduce data storage. Data includes processed sensor data, alarm setting and timestamp received. FarmSense and Home Hub web data displays should look identical and be familiar to the users no matter wich page they are viewing. 

The electronics will be enclosed in a junction box with LED indicators. LED indicators will serve as physical indication if their is an issue with a sensor in the field (issue no signal, configured reading alarm)
#### How do I setup Farm Sense Account?
If you setup your Home Hub to use FarmSense then you will be required to have a user name and password along with contact information.  This information can be updated and changed at anytime under your user profile.
#### What is a Farm Sense Account?
A Farm Sense account allows you to access your Home Hub information from anywhere in the world with secure access.  Each user is provided with 10Mb of free data storage with their purchase of a Home Hub.  Users may configure their Home Hub settings directly from Farm Sense Home Hub page just as if the user was locally connected to their Home Hub.

#### How do you use my information
We use limited details on the information you provide to know sensor use location.  This helps us better understand how many active customer sensors are in use. This also provides security in identifying any sensors that may have been stolen from you.
#### How do I setup Home Hub?
1. Plug in 5V DC power source to Home Hub. Power Indication light will turn on.
2. Navigate to local intranet site name: 
3. Create new account or log in to existing
4. Search for perfirial sensor device or type in sensor ID
5. Setup device alarms when you want to be notified LED blink type
6. close application.

#### Where will Home Hub get its information?
Home hub will take in all readings from other farm sense sensors in the field and process the info as needed.  Home Hub will self identify sensors in the area that you can identify and map to your home hub OR you can map the sensor unique ID.

#### How do I setup a new peripheral device
Home hub must be able to detect new devices sending info via LoRa RYLR889. User logs into their local Home Hub or online account, identifeis new devices or can tyupe in unique ID of sensor. In Home Hub you can name your sensros so they are easily identified. Once a Sensor is setup you can setup the alarms. Users can delete, remove existing, ignore sensors that are nto theirs.  User can access ignore list to reidentify a sensor they mistakenly are ignoring. Users can rename and change alarms at any time via edit, save buttons.
- Sensor must be powered and sending info for Home Hub to identify it.
- If sensor is not charged you can map the sensor unique ID in Home Hub so when it sends info it will be received.
- Alarm setting can determine how a user is notified of an alarm activation - first phase will be the LED lights.

Customer has the ability to set alarms per peripheral device connected.
#### How does Home Hub know a sensor and its values
The sensor will either do the calculation or Home Hub will need to do the calculation.  We need to keep the Home Hub internet agnostic as possible. Perhaps the sensors store information for Home Hub to learn what the data is and how to calculate final output. So when Home Hub first connects to sensor when setup the sensor sends data on what the sensor is, name, description, calculations Home Hub needs to perform and with what data. 
#### How will Home Hub be updated with software updates?
A Home Hub is not required to get updates. However it does have the ability should we need to push an update.  The user will need to enable their online account and log in. Once a user is logged in and data transfer is enabled then Home Hub will receive over the air updates via its wifi.
#### How will Home Hub Sensors be updated with software updates?
Sensors should never need to be updated in the field. However, if an issue is found then it would be ideal that user would connect to receive data. Home Hub would be updated and as part of that the sensor updates could be received and sent to the sensors. Sensors would auto reboot once the update was received.  
#### How are sensors powered?
Sensors are powered with a LiPo battery that is charged by solar cells on each sensor. Battery power level is sent to the Home Hub so users can monitor. Since sensors are on remote power then it is important that each sensor is ultra conservative on its power usage.  Each sensor is preset with optimal settings to conserve power but give enough updates and information to home hub.  A user may change the update frequency via Home Hub sensor settings.

Depending on the sensor every sensor will at least send an "Alive" packet to Home hub at least once a day. This transmission will tell home hub that the sensor is alive. Battery level, sensor reading, location, and other necessary info will be sent.
#### How do you identify Home Hub and Sensors?
Every unit produced by Farm Sense is provided a unique ID wihch distinguishes it in the system. 
Home Hub IDs=A-YYYYMMDD-##
Sensor IDs=S-YYYYMMDD-##

## Hardware Specifications
### Seed Studio XIAO ESP32C3
D01 - indicator light (blue)
D02 - not used
D03 - not used
D04 - SCL
D05 - SDA
D06 - RYLR998 RXD
D07 - RYLR998 TXD
D08 - not used
D09 - not used
D10 - not used
BAT - may be connected to lLIPO battery 3.7 2000mAh
GND - Connected to periferal devices
3V3 - Power periferial devices
VUSB - Powered via 5V DC

### SCL & SDA devices
- Absolute Pressure Sensor BMP280 / BME280
	- Address not given
- I/O Expansion - MCP23017-E/SP-ND
	- Address 0x20


### I/O Expansion - MCP23017-E/SP-ND
Up to 16 LED indicators to signal to user that there is an issue with peripheral device in the field. User is able to map device to LED via web interface. User can indicate blink, or on, depending on alarm.

### RYLR998
Devices in the field and the home hub use the RYLR998 to relay information to the home hub. 

The **[REYAX RYLR998](https://www.google.com/search?ibp=oshop&prds=pvt:hg,pvo:29,mid:576462693459907597,imageDocid:12365803489481099693,gpcid:14036538763176047603,headlineOfferDocid:6983100664999110964,catalogid:18234325081976999659,productDocid:3739232911707442329,rds:PC_14036538763176047603%7CPROD_PC_14036538763176047603&q=product&sa=X&ved=2ahUKEwinmaSH0bmXAxVoHoYAHdzdGdcQxa4PegYIAAgTEAE)** is ==a compact, long-range **LoRa transceiver module** designed for low-power spread-spectrum radio communication==. It is built on a **Nuvoton 32-bit MCU** combined with a high-performance **Semtech SX1262 LoRa engine**, making it a staple for IoT, industrial monitoring, and long-range DIY telemetry networks. [[1](https://www.youtube.com/watch?v=k5tU1Nbpr-U&t=915), [2](https://reyax.com/application/1), [3](https://gsasindia.com/blog/reyax-rylr998-lora-module-deep-dive)]

### Absolute Pressure Sensor BMP280 / BME280
Will be used with pressure sensing devices in the field such as 
Air Pressure Sensor Module 3.3-5V Digital Barometric Liquid Water Level Controller Board 0-40KPa. 
- **[BMP280](https://www.google.com/search?ibp=oshop&prds=pvt:hg,pvo:48,mid:576462886237903314,imageDocid:6000009340041714889,gpcid:4183135278495267420,headlineOfferDocid:14218821656228731219,catalogid:13411707836275443560,productDocid:8922389488103645512&q=product&sa=X&ved=2ahUKEwj11rCG0LmXAxVxoisGHXmgMiIQxa4PegoIAggACAAIHBAF) Measurements:** Barometric pressure (300 to 1100 hPa) and temperature (-40°C to +85°C). [[1](https://robocraze.com/blogs/post/what-is-bmp280), [2](https://apmonitor.com/dde/index.php/Main/ElevationPressure)]
- **[](https://www.google.com/search?ibp=oshop&prds=pvt:hg,pvo:48,mid:576462810929285420,imageDocid:1622460940324998939,gpcid:14908338027787888310,headlineOfferDocid:6211873710874498635,catalogid:15404362514241908978,productDocid:4856133979822695665,rds:PC_14908338027787888310%7CPROD_PC_14908338027787888310&q=product&sa=X&ved=2ahUKEwj11rCG0LmXAxVxoisGHXmgMiIQxa4PegoIAggACAAIHBAM)[](https://www.google.com/search?ibp=oshop&prds=pvt:hg,pvo:48,mid:576462810929285420,imageDocid:1622460940324998939,gpcid:14908338027787888310,headlineOfferDocid:6211873710874498635,catalogid:15404362514241908978,productDocid:4856133979822695665,rds:PC_14908338027787888310%7CPROD_PC_14908338027787888310&q=product&sa=X&ved=2ahUKEwj11rCG0LmXAxVxoisGHXmgMiIQxa4PegoIAggACAAIHBAM)[](https://www.google.com/search?ibp=oshop&prds=pvt:hg,pvo:48,mid:576462810929285420,imageDocid:1622460940324998939,gpcid:14908338027787888310,headlineOfferDocid:6211873710874498635,catalogid:15404362514241908978,productDocid:4856133979822695665,rds:PC_14908338027787888310%7CPROD_PC_14908338027787888310&q=product&sa=X&ved=2ahUKEwj11rCG0LmXAxVxoisGHXmgMiIQxa4PegoIAggACAAIHBAM)[BME280](https://www.google.com/search?ibp=oshop&prds=pvt:hg,pvo:48,mid:576462531178529489,imageDocid:1784551492449392664,gpcid:13122729653929979513,headlineOfferDocid:11617488668326255376,catalogid:7630123265487231139,productDocid:18446179551545603836&q=product&sa=X&ved=2ahUKEwj11rCG0LmXAxVxoisGHXmgMiIQxa4PegoIAggACAAIHBAN) Measurements:** Barometric pressure, temperature, and relative humidity (0% to 100%). [[1](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bme280-ds002.pdf), [2](https://learn.adafruit.com/adafruit-bme280-humidity-barometric-pressure-temperature-sensor-breakout?view=all)]
- **Accuracies:**
    - Pressure: ±1 hPa (absolute) / ±0.12 hPa (equivalent to ~±1 m altitude resolution).
    - Temperature: ±1.0°C.
    - Humidity (BME280): ±3%. [[1](https://blog.pishop.co.za/what-is-bme280-pinout-specifications-working/), [2](https://learn.adafruit.com/adafruit-bme280-humidity-barometric-pressure-temperature-sensor-breakout?view=all), [3](https://apmonitor.com/dde/index.php/Main/ElevationPressure)]
- **Power Consumption:** Very low, approximately 2.7 µA at a 1 Hz reading rate. [[1](https://www.utmel.com/components/bmp280-sensor-pinout-application-circuit-and-comparison?id=405)]
- **Communication Interfaces:** Supports both I²C and SPI protocols. [[1](https://robocraze.com/blogs/post/what-is-bmp280)]
- **Common Use Cases:** Weather stations, drones, indoor navigation, GPS elevation refinement, and IoT devices. [[1](https://www.flywing-tech.com/blog/bmp280-bme280-bme680-best-barometric-sensor-for-weather-stations/), [2](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bme280-ds002.pdf), [3](https://learn.adafruit.com/adafruit-bme280-humidity-barometric-pressure-temperature-sensor-breakout?view=all), [4](https://easyelecmodule.com/barometric-pressure-sensor-bmp280-bme280/)]
