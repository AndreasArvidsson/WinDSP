# WinDSP
Windows based DSP(Digital Signal Processor)

# Table of Contents
[Features](#Features)

[Install and run](#Install-and-run)
* [Prerequisites](#Prerequisites)
* [Installation](#Installation)

[Hardware and devices](#Hardware-and-devices)
* [Capture device](#Capture-device)
* [Render device](#Render-device)
* [Virtual Cable](#Virtual-Cable)
* [ASIO support](#ASIO-support)

[Web based configuration interface](#Web-based-configuration-interface)

[JSON config file](#JSON-config-file)
* [Config switching](#Config-switching)
* [Available channels](#Available-channels)
* [Config layout](#Config-layout)
* [Start WinDSP with Windows](#Start-WinDSP-with-Windows)
* [Hide and minimize](#Hide-and-minimize)
* [Description](#Description)
* [Devices](#Devices)
* [Basic routing](#Basic-routing)
* [Advanced routing](#Advanced-routing)
* [Conditional routing](#Conditional-routing)
* [Outputs](#Outputs)
* [Filters](#Filters)
* [References](#References)

[Filter parameters](#Filter-parameters)
* [Delay](#Delay)
* [PEQ (Parametric equalizer)](#PEQ-(Parametric-equalizer))
* [Shelf](#Shelf)
* [Band pass](#Band-pass)
* [Notch](#Notch)
* [Crossover](#Crossover)
* [Linkwitz Transform](#Linkwitz-Transform)
* [Biquad](#Biquad)
* [FIR (Finite Impulse Response)](#FIR-(Finite-Impulse-Response))

[Output filter parameters](#Output-filter-parameters)
* [Cancellation](#Cancellation)

[Errors and Warnings](#Errors-and-Warnings)

[Disclaimer](#Disclaimer)


# Features
* Route any input to any output. Any input can be routed to zero or many outputs.
* Add any filter to route or output.
* Filters like crossovers, PEQ, shelf, custom biquad(IIR), FIR, delay, gain and more.
* Uses double-precision(64bit) floating-points to calculate filters.
* Uses WASAPI(Windows Audio Session API) or ASIO to capture and manipulate audio streams.
* JSON based configuration file to easy set up your DSP.
* Web based configuration interface
* User friendly error and warning messages. Warns you about digital clipping. Using missing channels and more.


# Install and run

## Prerequisites

Prerequisites to run application
* Capture and render audio devices.
* Windows 10 or newer. May work on older OS. Feel free to try it out.
* [Microsoft Visual C++ Redistributable](https://www.microsoft.com/en-us/download/details.aspx?id=52685)

Prerequisites to run config editor
* [Java 8](https://java.com/en/download)

Prerequisites to compile source code
* [CoreLib](https://github.com/AndreasArvidsson/CoreLib)

## Installation
1. Download and install [Microsoft Visual C++ Redistributable](https://www.microsoft.com/en-us/download/details.aspx?id=52685).
1. Download and install [VB-Audio Virtual Cable](https://www.vb-audio.com/Cable/index.htm).
1. Set Virtual Cable as your default audio playback device.
1. Configure Virtual Cabel to use the same sample rate as your render device (/properties/advanced).
1. Configure Virtual Cabel to NOT allow applications to take exclusive control (/properties/advanced).
1. Configure Virtual Cable to use 7.1 surround (/configure).
1. Configure WinDSP.json configuration file. (Manually or via the web based config editor)
1. Start WinDSP.exe.


# Hardware and devices
Windows (as far as I know) doesn't allow manipulation of the audio stream during playback. You can however capture a playing audio stream from one device and play it back to another device.
This requires two devices: one capture device and one render/playback device.

## Capture device
* Receives audio from applications. Make it the default playback device.
* Have NO audio equipment/speakers attached.

## Render device
* Receives audio ONLY from WinDSP.
* Have audio equipment/speakers attached.

## Virtual Cable
If you don't have a spare soundcard in your computer to use for the capture device I can recommendend [VB-Audio Virtual Cable](https://www.vb-audio.com/Cable) which gives you a virtual audio device to use as the capture device.

## ASIO support
* WinDSP has experimental ASIO support. Please try it out.
* Optionally [VB-Audio Asio Bridge](https://www.vb-audio.com/Cable) can be used as an WASAPI render device to playback through an ASIO soundcard.

# Web based configuration interface
By popular demand there now is a web based configuration editor. Gone are the days when you have to manually edit the configuration file.

1. Start file `runConfigEdit.bat`
1. Browse to [http://localhost:8080](http://localhost:8080)

* Every change on the web page will update the config file in realtime.
* WinDSP.exe can be running at the same time.
* Config page can be reaced from other computers than the one running WinDSP by using the IP-adress. 
    * `http://192.168.1.10:8080`
* Defaults to edit `WinDSP.json` in the same folder. To use other config files:    
    * `java -jar WinDSP-configEditor.jar WinDSP-1.json`


# JSON config file
* Saving the config file will automatically restart WinDSP. No need to manually close and open the program.
* If you are not used to JSON use an editor like [Json Parser Online](http://json.parser.online.fr) to get the format correct.

## Config switching
* If you have multiple config files you can switch between them using the 0-9 buttons.
* Button '1' selects congfig file 'WinDSP-1.json', button '2' selects 'WinDSP-2.json' and so on.
* Button '0' select the default config file 'WinDSP.json'.

## Available channels
   * L: Front left
   * R: Front right
   * C: Center
   * SW: Subwoofer/LFE
   * SL: Surround left
   * SR: Surround right
   * SBL: Surround back left
   * SBR: Surround back right

## Config layout   
```json
{
    "startWithOS": false,
    "minimize": false,
    "hide": false,
    "debug": false,
    "description": "Default config",
    "devices": {
        "capture": "CABLE Input (VB-Audio Virtual Cable)",
        "render": "Focusrite USB AUDIO",
        "renderAsio": false,
        "asioBufferSize": 88,
        "asioNumChannels": 8
    },
    "basic": {
        "front": "Large",
        "center": "Sub",
        "subwoofer": "Sub",
        "surround": "Small",
        "surroundBack": "Off",
        "stereoBass": true,
        "expandSurround": true,
        "lfeGain": -3,
        "lowPass": {
            "type": "BUTTERWORTH",
            "order": 5,
            "freq": 80,
            "qOffset": 0.0
        },
        "highPass": {
            "type": "BUTTERWORTH",
            "order": 3,
            "freq": 80
        }
    },
    "advanced": {
        "L": [
            {
                "out": "L",
                "gain": -3.8,
                "delay": 2,
                "invert": false,
                "filters": []
            }
        ]
    },
    "outputs": [
        {
            "channels": [ "L", "R" ],
            "gain": -3.2,
            "delay": 4.5,
            "invert": false,
            "mute": false,
            "filters": []
        },
        {
            "channel": "C",
            "gain": -2
        }
    ]
}
```

## Start WinDSP with Windows
* Set startWithOS to true and WinDSP will start with the OS/Windows.

## Hide and minimize
* Set hide to true to hide window on startup and show tray icon.
* Set minimize to true to minimize window on startup.
* Hide supersedes minimize.
* Errors will show the window again, but not warnings.
* Double click tray icon to manually show hidden window again.
* Minimizing the window when hide is set to true will instead hide it

## Description
* Description of the config file to be displayed at startup.
* Optional. If not given no description will be shown.
* Useful when using multiple config files. eg: "Default", "Night mode", "Less bass".

## Debug
* Set to true to print debug data.
* Is shown in both application window and separate "WinDSP_log.txt" log file.

## Devices
* Devices contains the capture and render device names.
* If devices are not set the user will be queried from a list of available devices. Do **NOT** write these names yourself.
* ASIO parameters:
    * renderAsio: Set to true if the render device uses ASIO instead of WASAPI.
    * asioBufferSize: Sample size of the ASIO render buffer. Leave out to use sound card prefered size.
    * asioNumChannels: Number of channels to use for the ASIO render device. Leave out to use all.

## Basic routing
* Basic routing CAN'T be combined with advanced routing.
* Use basic node to specify speaker types. The DSP will automatically route input signal to correct output.
* The following speaker types are available:
    * Large: Speaker will play full range. Input will be sent to output unchanged.
    * Small: Speaker will not play bass. High frequencies will be sent to speaker and low frequencies to subwoofer.
    * Off: Speaker output is disabled. Send signal to the next best available channel. Output is disabled/silent.
    * Sub: Output is subwoofer. If not SW channel routing will be the same as Off. Difference is that the output is used for subwoofer.
* Available speaker modes per channel. First value is default.
   * front(L/R): **Large**, Small
   * center(C): **Large**, Small, Sub, Off
   * subwoofer(SW): **Sub**, Off
   * surround(SL/SR): **Large**, Small, Sub, Off
   * surroundBack(SBL/SBR): **Large**, Small, Sub, Off
* stereoBass: If true bass will be played in stereo. If false bass will be in mono.
   * Default value: false
   * Can't be used with only one subwoofer
   * Can be used with Large fronts. Bass from small channels will be sent to only one front speaker.
   * L/C/SL/SBL channels are considered left bass channels. If set to sub and stereoBass true.
   * R/SW/SR/SBR channels are considered right bass channels. If set to sub and stereoBass true.
* expandSurround: If true and surround back input is quiet(5.1 source) surround back will play surround channels track.
   * Default value: false
* lfeGain: Gain offset for mixing the LFE signal with other channels.
   * Default value: 0
   * Works with both subwoofers and front speakers.
* lowPass: Filter configuration for low pass filter. Is applied to Sub channels.
   * Default value: Butterworth 80Hz 5order(30dB/oct)
* highPass: Filter configuration for high pass filter. Is applied to Small channels.
   * Default value: Butterworth 80Hz 3order(18dB/oct)

## Advanced routing
* Advanced routing CAN'T be combined with basic routing.
* Use advanced node to manually specify each mapping between inputs and ouputs.
* By default all inputs are routed directly to the matching output. L to L, R to R and so on.
* To not use an input at all just add empty brackets: ```"L": {}```
* One input can have multiple outputs. Including the same output twice if needed.
* A route can have multiple filters like gain, delay, crossovers, peq and more.

## Conditional routing
* You can add conditions to a route. If the conditions is not met the route will not be active.
* For now the only condition available is to detect if an input channel is silent or not.
* The code below will route audio from surround(input) to surround back(output) if surround back(input) is silent.
```json
"inputs": {
   "SL": {
      "routes": [
         {
            "out": "SBL",
            "if": {
               "silent": "SBL"
            }
         }
      ]
   }
}
```

## Outputs
* Outputs contains each output channel. This is the sum of all the input routes to this channel.
* By default the output channels have no filters.
* If basic routing is used crossovers and gain filters are applied if not specified manually in output.
* An output can have multiple filters like gain, delay, crossovers, peq just like the input routes.
* An output can be muted or inverted.
* Each output node can be for one channel ```"channel": "L"``` or multiple ```"channels": ["L", "R"]```

## Filters
* The program handles all audio manipulation as filters. A filter can be a something complex as a crossover or something simple like gain.
* Some filters can only exist once in a route or output. eg. gain, delay, invert, mute.
* Other filters(in the filters array) can occur multiple times. eg. crossovers, shelf, PEQ, LT, custom biquad, FIR.

## References
A common user case is that multiple channels or routes share filter configurations. Instead of having to copy and paste these you can reference one JSON node from another.
```json
"filters": {
    "myPEQ": {
        "type": "PEQ",
        "freq": 320,
        "gain": -10,
        "q": 5
    }
},
"outputs": [
    {
        "channels": [ "L", "R" ],
        "filters": [
            { 
               "#ref": "filters/myPEQ" 
            }
        ]
    }
]
```


# Filter parameters

## Delay
* Requires: value
* Optional unitMeter parameter to use meters instead of milliseconds.
```json
"delay": 5.2
```
or
```json
"delay": {
    "value": 4.5,
    "unitMeter": true
}
```

## PEQ (Parametric equalizer)
* Requires: type, freq, gain, q
```json
{
    "type": "PEQ",
    "freq": 29.0,
    "gain": -13.0,
    "q": 4.5
}
```

## Shelf
* Type: LOW_SHELF or HIGH_SHELF    
* Requires: type, freq, gain    
* Q-value defaults to 0.707
```json
{
    "type": "LOW_SHELF",
    "freq": 100.0,
    "gain": -6.0,
    "q": 0.5
}
```

## Band pass
* Requires: type, freq, bandwidth    
* Gain defaults to 0dB
```json
{
    "type": "BAND_PASS",
    "freq": 100.0,
    "bandwidth": 3.5,
    "gain": 1.5
}
```

## Notch
* Requires: type, freq, bandwidth    
* Gain defaults to 0dB
```json
{
    "type": "NOTCH",
    "freq": 100.0,
    "bandwidth": 3.5,
    "gain": 1.5
}
```

## Crossover
* Type: LOW_PASS or HIGH_PASS    
* Requires: type, crossoverType, order, freq
* Crossover types are: BUTTERWORTH, LINKWITZ_RILEY, BESSEL and CUSTOM
* Butterworth is available in orders: 1 through 8
* Linkwitz-Riley is available in orders: 2, 4 and 8
* Bessel is available in orders: 2 through 8
* For all crossover types except cutsom a single optional Q-offset can be given. This is an offset of the existing Q-values to tweak basic crossovers. Default value: 0
* Crossover type custom requires Q values array. One Q-value per 2nd order filter. Give Q as -1 to get a 1st order filter.
```json
{
    "type": "LOW_PASS",
    "crossoverType": "BUTTERWORTH",
    "order": 4,
    "freq": 80.0,
    "qOffset": 0.2
}
```
```json
{
    "type": "LOW_PASS",
    "crossoverType": "CUSTOM",
    "order": 5,
    "freq": 80.0,
    "q": [
        -1.0
        0.707,
        0.5
    ]
}
```

## [Linkwitz Transform](https://www.minidsp.com/applications/advanced-tools/linkwitz-transform)    
* Requires: type, f0, q0, fp, qp
```json
{
    "type": "LINKWITZ_TRANSFORM",
    "f0": 30.0,
    "q0": 1.2,
    "fp": 10.0,
    "qp": 0.5
}
```

## [Biquad](https://www.minidsp.com/applications/advanced-tools/advanced-biquad-programming)    
* Create a custom biquad by giving the biquad coefficients.
* Biquad filters are also known as IIR(Infinite Impulse Response).
* b0-b2 are the feedforward values in the numerator and a0-a2 are the feedback values in the denominator.
* Multiple sets of coefficients can be given to create a cascading filter.
* Requires: type, values[b0, b1, b2, a1, a2]
* a0 defaults to 1.0
```json
{
    "type": "BIQUAD",
    "values": [{
        "b0": 0.2513790015131591,
        "b1": 0.5027580030263182,
        "b2": 0.2513790015131591,
        "a0": 1.0,
        "a1": -0.17124071441396285,
        "a2": 0.1767567204665992
    }]
}
```

## [FIR (Finite Impulse Response)](https://www.minidsp.com/applications/advanced-tools/rephase-fir-tool)    
* WinDSP loads a text file with the FIR parameters.
* Use a tool like [rePhase](https://sourceforge.net/projects/rephase) to calculate the FIR parameters.
* FIR file is either a text(.txt) file or a wave(.wav). 
* Wave file can be either LPCM(16/24/32bit) or float(32/64bit).
* Warning: FIR filters require much more CPU capacity then the other filters. WinDSP doesn't limit the number of taps you can input so use with care.
```json
{
   "type": "FIR",
   "file": "fir.txt"
},
{
   "type": "FIR",
   "file": "fir.wav"
}
```

* FIR text file format is one FIR tap per line as a floating point number.
* Use "32 / 64 bits floats mono (.txt)" option in reShape to generate parameters file.
```
0
0.0000000000000000025148658985616801
-0.000000000000000010019981273260004
0.000000000000000022669569961523929
-0.000000000000000040502459260231708
0.000000000000000063400956715765757
-0.000000000000000091367289519079492
```


# Output filter parameters
These are filter only available on outputs and not on routes.

## Cancellation
* Creates an inverted and delayed signal to cancel out the specified frequency. This is another way to remove peaks.
* Requires: freq    
* Gain defaults to 0dB
```json
"cancellation": {
    "freq": 28.0,
    "gain": -5
}
```


# Errors and Warnings
* An error is a problem serious enough that the program can't run. eg. missing devices, faulty config.
* An error will output a text message describing the problem and then restart the program. If the problem is corrected the program will start normally.
* A warning is a potential problem, but the program can still continue running. eg. Configured a route for a channel the audio device is lacking, the sum of all routes can go above 0dBFS on the output.
* All configured input and outputs the device is lacking will just be ignored.
* If you sum multiple inputs together and they play peak signals at the same time digital clipping will occur which creates distortion. Lower the output gain if a digital clipping warning is shown.


# Disclaimer
I test all my software to the best of my ability and this is a software I personally use in my own audio setup, but bugs can still occur and due to the nature of this software being audio playback related, loud uncomfortable high or low frequency distortions may be a possibility. I haven't had any problems like that myself and if I find any I will patch them out, but just be aware that you use this software on your own accord. With that said please feel free to enjoy it! :)
