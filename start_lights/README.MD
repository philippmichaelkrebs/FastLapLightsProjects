# FastLapLights

**"Gentlemen, start your engines!"** 🏁  

Bring FIA-style lighting to your slot car track with FastLapLights - a high-tech lighting and telemetry system for Carrera Digital tracks! 
Control WS2812b Start Lights, Pit Lane Lights and Track Flags with stunning LED animations that bring your races to life. Want more? FastLapLights als taps into the real-time data stream from the Carrera Control Unit, filters the most relevant messages, and outputs the via UART - perfect for powering external displays, mechanical 7-segment scoring pylons, or even fuel and pit-stop management systems. 
Whether you're racing for fun or building the ultimate home track, FastLapLights adds a whole new level of immersion, control and creativity.

Following Models are supported:
- Carrera Digital 124
- Carrera Digital 132

Preview:  
![](https://github.com/philippmichaelkrebs/FastLapLights/blob/main/images/signal_start_proc_start_light.gif?raw=true)


# Features

## Start Lights
Perfect race countdown sequence with WS2812B LEDs 🚦

You can switch between horizontal and vertical alignment for the start lights WS2812B led strips. Vertical alignment requires more soldering but looks more realistic, giving you the option to physically seperate the  lights - just like in real motorsport.
The system is designed to handle 40 leds. So both the front and the back of the lights illuminated. You nearly have a 360 degree visibility of the start lights.

## Pit Lane Lights
Manage pit stops like a real motorsport pro!

Instead of start loghts, you can choose to illuminate pit lane lights. This is an either-or option. Simpy pull down the specific pin.

## Track Flag System
RGB LEDs indicate race conditions (Green, Yellow, Red)

With a red and green led, you can animate track lights. It's also possible to use a rgb led with r and g connected only. Race status indicated by flags:
- Green: go, 
- Yellow: caution, 
- Red: stop
If you plan to use multiple RGB LEDs, you should use a mosfet to avoid overloading the uCs GPIOs.

## Real-Time Data Capture
Tap into live control unit telemetry!

The STM32 continuously processes data from the Carrera Control Unit. A filtered stream of the most relevant messages is available via the UART interface. This gives you a powerful foundation to build custom applications and add unique behavior to your racetrack setup. 
Whether it's a mechanical 7-segment scoring pylon or a smart pit stop prediction system, FastLapLights gives your cicuit the edge it deserves.

# Get Started

- Ask for the STM32 microcontroller
- Connect WS2812B & RGB LEDs according to the purpose.
- Solder power wires to the track
- Power up & lights out!


# Pinouts
ToDo

# UART
The data that is transmitted by the Control Unit is an asynchronous serial protocol. The chunks are of various size. Notably, the protocol doesn't use a traditional stop bit. The cu transmits data in a structured, continouos stream every 75 milli secs. Each transmission cycle contains 10 distinct messages sent in a strict and predictable sequence.

## UART Configuration
- Baud rate: 19200
- Data format: 8 data bits, no parity, 1 stop bit (8N1)
## Application Behavior
- Message Size: Each UART message is exactly 32 bytes in length.
- Contents: The message includes all relevant application data (e.g., race state, positions, fuel levels, etc.) and is a snapshot of the current race state.
- Transmission Rate: A new message is transmitted every 500 milliseconds.

## Message Decoding Example

The following C code snippet demonstrates how to decode the message and populate internal state structures:
(ToDo: update with crc)
```c 
deprecated
```

# Documentation
- [How the Lights Work](/start_lights/documents/how_the_lights_work.md)
- [Getting Started](/start_lights/documents/getting_started.md)

# Contribute & Support

Got ideas? Found a bug? Want to make it even cooler? 
Open an issue, submit a PR, or just say hi!

Developer: Philipp Krebs  
Contact: pmge.krebs@gmail.com  
Repo: https://github.com/philippmichaelkrebs  