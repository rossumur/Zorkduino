#Zorkduino
###Play Zork on your TV with an Arduino.

Who doesn’t love Zork?. Who doesn’t love Arduinos? Why not grab a few cheap components and build an Arduino gadget capable of playing all the classic Infocom games on your TV from the comfort of your couch.

<img src="https://raw.githubusercontent.com/rossumur/Zorkduino/master/docs/Title.jpg" width="100%"/>

A few years ago I ported a Z-Machine player to a [little arduino-like device](http://rossumblog.com/2011/04/19/zork-for-the-microtouch/). Ever since I have been meaning to get around to a project that would work on a TV with a real keyboard and once again rekindle fond memories of long nights playing Zork on my Atari 800.

###You will need:
* Arduino UNO, Pro, Pro Mini or equivalent.
* SD card or micro SD card + breakout board (from Adafruit, eBay etc).
* RCA A/V Cable (eBay).
* 470ohm, 1k and 100k resistors.
* Breadboard, wires etc.
* WebTV or MsnTV IR Keyboard or PS2 a nasty old PS2 keyboard (eBay).
* IR receiver TSOP38238,TSOP4838 or equivalent (Adafruit, Mouser etc).

##Building it

The schematic is very simple:

```
        +----------------+           +-----------------+
        |                |           |                 |
        |             13 |-----------| SCK    micro\SD |
        |             12 |-----------| MISO   card     |
        |             11 |-----------| MOSI   module   |
        |             10 |-----------| CS              |
        |                |      5v <-|                 |
        |                |     GND <-|                 |
        |    arduino     |           +-----------------+
        |    uno/pro     |
        |                |       5v <--+-+   IR Receiver
        |                |      GND <--|  )  TSOP4838 etc.
        |              8 |-------------+-+
        |                |
        |              6 |----[ 100k ]--------> AUDIO
        |                |
        |              9 |----[  1k  ]----+---> VIDEO
        |                |                |
        |              1 |----[ 470  ]----+
        |                |
        |              3 |----------------> *PS2 CLOCK
        |              2 |----------------> *PS2 DATA
        |                |
        +----------------+
```
###Layout on an Arduino Uno...
<img src="https://raw.githubusercontent.com/rossumur/Zorkduino/master/docs/Uno.jpg" width="100%"/>

###...and on a Mini Pro.
<img src="https://raw.githubusercontent.com/rossumur/Zorkduino/master/docs/ProMini.jpg" width="100%"/>

Just about any SD card or microsd card breakout will do. Some of the very cheap ones (<$1) don't have 5v to 3v3 level converters and may fry your SD card so caveat emptor. As always, Adafruit has nice ones.

WebTV keyboards come in various guises: WebTV, MsnTV, Displayer, UltimateTV etc. They all should work just fine. A few places have the nice Philips variant new for $11.95 w/ free shipping (search for 'SWK-8630'). This one comes with a nice PS2 IR receiver; more on it later.

<img src="https://raw.githubusercontent.com/rossumur/Zorkduino/master/docs/Keyboards.jpg" width="100%"/>

IR receivers come in a number of different forms. You are looking for a 38khz version with a known pinout: Some have the center pin as GND, some as V+. Make sure you know what kind you have. When in doubt, Adafruit.

I like using iPhone/iPod video cables for TV projects. Because the no longer work (their MFI chip long since revoked) they are inexpensive, are labeled internally and have a strain relief grommet.

If you have an IR keyboard then good for you. If not, connect your nasty old PS2 clock and data lines to pins 3 and 2 then order an IR keyboard.

###On the disk
The [`microsdfiles`](https://github.com/rossumur/Zorkduino/tree/master/microsdfiles) folder contains a `zd.mem` pagefile along with several sample games:

`tutorial.z3` Introduction to interactive fiction and a little bit of Zork I

`sampler1.z5`
Samples of Planetfall, Infidel, and The Witness.

`sampler2.z3`
Samples of  Zork I, Leather Goddesses of Phobos, and Trinity

`minizork.z3`
A nice big chunk of Zork I that was given away with the British Commodore users' magazine "Zzap! 64" no. 67. in 1990.

Copy these files to a freshly formatted sd or microsd card. You can find lots of other Zorkduino compatible games at the [Interactive Fiction Archive](http://www.ifarchive.org/). Insert the card and run the `zorkduino.ino` sketch from the [`zorkduino`](https://github.com/rossumur/Zorkduino/tree/master/zorkduino) folder. When it is all up and running, it should look like this (depending on how many games you found):

<a href="http://www.youtube.com/watch?feature=player_embedded&v=-4dWXJrqxUk
" target="_blank"><img src="http://img.youtube.com/vi/-4dWXJrqxUk/0.jpg" 
alt="IMAGE ALT TEXT HERE" width="100%" /></a>

##How it works
Squeezing Zork into the limited footprint of an Arduino proved to be a bit of a challenge. The code uses a port of Mark Howell and John Holder's JZIP, a Z-machine interpreter. The Z-machine was created in 1979 to play large (100k!) adventure games on small (8K!) personal computers. Long before Java the implementors at Infocom built a virtual machine capable of paging, loading and saving complete runtime state that ran on a wide variety of CPUs. Clever stuff.

The trouble is the Arduino only has 2k of ram. The Z-machine interpreter uses 2k for its stack alone, leaving no room for dynamic memory, disk buffers, video frame buffers, avr stack and other program state. The solution is to virtualize all stack and memory accesses from the interpreter down to a 160 byte cache and a 512 byte disk buffer. Thats where the `zd.mem` file comes in - a megabyte or so of virtual stack, memory and save-game slots.

Virtualizing everything slows things down a bit but serendipitously makes the Arduino perform at roughly the same speed as my old Atari 800. Video needs 912 bytes for the frame buffer, leaving ~464 bytes for the avr stack and all application and interpreter state.

###Video
The video is generated with a simple state machine on the Timer1 ISR. HSYNC interrupts occur at 15.73kHz and trigger state changes for video, audio and keyboard state machines.

Active video is 308x192 pixels, 38x24 8x8 charaters. Getting this resolution requires use of the UART in SPI mode running at top (8Mhz) speed to shift out pixels from either a character font (the Atari 800 default font) or a graphics font (an extravagant waste of 2k logo). Because the video is being emitted from the TX pin it is fun to open the serial monitor to watch what ntsc video looks like as text: almost at bit Matrixy.

###Audio
No homage to 70's and 80's computing would be complete without keyboard beeps and disk thweeps. Keystrokes generate a 1000hz feedback beep, sd card writes make a 3600hz mechanical click. These sounds are generated in the video isr by toggling pin 6 after a certain number of HSYNCs.

###Keyboard
WebTV IR is an unusual UART-like asych serial protocol. It runs at ~660 baud with 3.25 start bits, a 10 bit class code, 8 bit key code, parity and stop bit. Class codes are things like keyup, keydown etc. and key codes map physical keyboard layout. The video ISR counts HSYNCs between transitions on the IR data line and passes state change events to the keyboard code. 

The PS2 code is pretty conventional. A good explanation is [here](http://www.computer-engineering.org/ps2protocol/).

##Extra Credit
If you get the Philips $11.95 keyboard variant you get a PS2 IR receiver as well. The receiver can be...

* used as-is as a PS2 keyboard connected to pins 2 and 3.
* gutted and turned into a nice case.

I prefer the latter. The case has a nice slot in the back with a sliding door that is perfect for a microSD card. It has a nice IR window at the front and a TSOP31238 (5V center pin) IR receiver that can be harvested from the PCB. Get a cheap Pro/Mini + microsd breakout, add that iPod video cable and it looks like this:

<img src="https://raw.githubusercontent.com/rossumur/Zorkduino/master/docs/case.jpg" width="100%"/>

Drop me a line at rossumur@gmail.com if you have any trouble.

cheers

[rossumblog.com](http://rossumblog.com)

p.s. The maze in minizork is not the same layout as in Zork I. I have it if you need it.

