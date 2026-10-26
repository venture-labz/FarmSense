#### What is the purpose of Farm Sense
Farm Sense produces remote monitoring equipment for farms and industrial applications. Our base product is Home Hub which receives remote sensor data via LoRa.  Our mission is to give peace of mind knowing they will be alerted if a sensor detects a scenario they need to know about and address.  We don't want to make our customers life more complicated by technology, but to keep them informed and give them peace of mind of their farm or industrial facility. 
#### What is the purpose of Home Hub
Farm Sense Home Hub receives remote sensor data over LoRa. The Seeed Studio XIAO ESP32-C3 version is the **Mini Home Hub**: a lightweight gateway that relays sensor data to the Farm Sense cloud service when internet access and cloud account configuration are available. Cloud forwarding is a product requirement; it is not yet implemented in the current prototype.

The Mini also hosts a simple local website, reachable through its own Wi-Fi or the customer's home network. Customers can view current sensor readings in tiles, manage sensors, configure Wi-Fi, and watch incoming sensor events without internet access or a cloud account. Maps are available only in the cloud version's scope, not on the Mini. Local and cloud screens should use consistent terminology and familiar navigation, but need not have identical features or layouts.

Data storage will be capped on the Farm Sense server at 10Mb per free account. Cloud data includes processed sensor data, alarm settings, and receipt timestamps in a compact format. Cloud transport, authentication, and account provisioning still require implementation design.

#### Why is the Mini Home Hub limited to a lightweight local site?
The latest prototype build occupies 1,073,147 bytes, or 81% of the current 1,310,720-byte firmware app slot. This leaves limited room for the intended expanded feature set; it does not mean the board cannot host a website. The Mini's scope prioritizes receiving sensor data, cloud forwarding, and a lightweight local interface. Rich map features move to the cloud version. Repeater use is a possible future role for this hardware, not a current requirement or an implemented capability.

The electronics will be enclosed in a junction box with LED indicators. LED indicators will serve as physical indication if their is an issue with a sensor in the field (issue no signal, configured reading alarm)
#### How do I setup Farm Sense Account?
If you setup your Home Hub to use FarmSense then you will be required to have a user name and password along with contact information.  This information can be updated and changed at anytime under your user profile.
#### What is a Farm Sense Account?
A Farm Sense account allows you to access your Home Hub information from anywhere in the world with secure access.  Each user is provided with 10Mb of free data storage with their purchase of a Home Hub.  Users may configure their Home Hub settings directly from Farm Sense Home Hub page just as if the user was locally connected to their Home Hub.

#### How do you use my information
We use limited details on the information you provide to know sensor use location.  This helps us better understand how many active customer sensors are in use. This also provides security in identifying any sensors that may have been stolen from you.
#### How do I setup Home Hub?
1. Plug in 5V DC power source to Home Hub. Power Indication light will turn on.
2. Connect to the Mini Home Hub's local Wi-Fi and open its locally hosted website. Use the Home Wi-Fi screen to enter the customer's Wi-Fi network name and password so the Mini can reach the internet and be accessed over that network. The Mini's own Wi-Fi remains available. Wi-Fi connection status and cloud connection status must be shown separately; joining Wi-Fi alone does not prove the cloud is reachable.
3. Create or sign in to a local Home Hub account. A Farm Sense online account is optional; basic local operation does not require internet access.
4. Find reporting unclaimed sensors or enter a sensor's unique ID. When an ID is entered before the sensor reports, Home Hub waits for a report with that ID.
5. Claim the sensor, review its reported information, and configure its name, alarms, grouping, and location.
6. Use sensor tiles and the Live data view to monitor the network locally. Configure a Farm Sense cloud account for cloud forwarding and access to cloud features, including maps, when those integrations are implemented.

#### Where will Home Hub get its information?
Home Hub receives readings from Farm Sense sensors over LoRa and processes them as needed. Sensors identify themselves by unique ID. Home Hub shows new, unclaimed sensors that report nearby; the customer can claim sensors that belong to their network and ignore sensors that do not.

#### How do I setup a new peripheral device
The customer can select a reporting unclaimed sensor or enter its unique ID. If the sensor has not reported yet, Home Hub waits for a matching report before completing setup. Users can claim sensors, rename them, assign alarms, group them, set their locations, ignore sensors that are not theirs, and restore a sensor from the ignore list. Changes are saved from the sensor's edit screen. Alarm settings are configured per sensor; the first notification method is the Home Hub LED indicators.

When a sensor first connects, it sends a setup packet describing its identity, type, name or description, available data, and any information Home Hub needs to interpret that data. Home Hub processes the setup information and reports whether setup is complete so the sensor knows its configuration state.

#### How do I view and organize my sensors?
The local Mini Home Hub website shows sensors and their current available data in tiles. Customers can search, sort, and group sensors by location or customer-defined titles such as Farm 1 or Farm 2. An Add Sensor button at the top opens a screen for entering a unique ID. Selecting a sensor opens a focused screen with its available received data and editable properties, including name, alarms, grouping, and location. Local account setup, sign-in/sign-out, discovery, claiming, setup acknowledgement, and ignore/restore remain part of the Mini interface. A simple upper-right navigation menu provides access to Sensors, Live data, and Home Wi-Fi.

Map viewing and sensor placement on a map belong to the cloud version. The Mini does not provide a map, tile-source configuration, or offline map downloads. Editable location information remains available locally. No offline-map capability is promised for the cloud version in this baseline.

#### How can I see data as it arrives?
The signed-in Live data view shows a readable feed of incoming sensor readings and setup/status events, with the sensor name and receipt time. If an accurate clock is unavailable, the view uses elapsed time rather than presenting an invented date or time. Customers can filter by sensor and pause/resume the display; pausing the display does not stop sensor reception or cloud forwarding.

The feed updates automatically and retains only a bounded set of recent events. It is a local monitoring view, not permanent history or a raw packet console. The retention bound will be selected and verified against available memory during implementation.

#### What happens when internet access is unavailable?
The Mini continues receiving sensor readings and providing local monitoring, Live data, and sensor management. Cloud connectivity is shown as unavailable, separately from Wi-Fi connectivity. Once the connection is restored, forwarding resumes with new readings. The Mini does not promise to queue or recover readings missed by the cloud during an outage. A cloud account is not required for basic local access.
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
### Seeed Studio XIAO ESP32-C3 — Mini Home Hub
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
