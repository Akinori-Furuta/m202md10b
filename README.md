# Readme

## Introduction

The m202md10b application controls the FUTABA M202MD10B VFD
character display panel. This application provides following features,

* Configure serial port.
* Send break signal to port.
  * With additional circuit, reset the panel by break signal.
* Translate command line control option into control character(s).
* Read stdin and send characters to the panel.

## Requirements

|Item|Description|
|:---|:---|
|OS|Cygwin on windows. or Linux.|
|IO|Serial port, supports 8 bits, no parity, 1 stop bit format, {1200, 2400, 4800, or 9600} bps speeds.

## Build

To build m202md10b application, run simply make in this directory
(sown as ${base} directory),

```bash
make
```

### Configure PC (on linux)

To use the serial port on the linux OSes. Add user to dialout group.
If you want add yourself use ${USER}.

```bash
sudo usermod -a -G dialout ${USER}
```

Check /etc/group, if the user joined to the dialout group.

```bash
cat /etc/group | grep ${USER}
```

If the user didn't joined to the dialout group, check /etc/group and
fix group settings. To reflect the group credential logout and login again.

## M202MD10B Interface Circuit

The FUTABA M202MD10B VFD panel requires following control signals,
|Panel Signal|Description|
|:-----------|:----------|
|DATA|Serial line, 8bit, no parity, 1 stop bit, {1200, 2400, 4800, or 9600} bps speeds.|
|#RST|Reset panel, active low.|
|BUSY|Busy, active high.|

Add interface circuit to control these signals.
[An example schematic "Futaba M202MD10B PC Interface
Board"](file://circuit/M202MD10BIF2.pdf "M202MD10B interface circuit")
implements following functions and connect to PC via USB-Serial interface.

|Panel signal|Implimentation|
|:-----------|:-------------|
|DATA|Serial TXD.|
|#RST|Translate serial break signal into #RST signal.|
|BUSY|Serial #CTS, Clear To Send, active low.|

### Configure M202MD10B

Demo scripts assume that the M202MD10B is configured to 2400bps speed.
Set the DIP switch on M202MD10B as follows,

|Locate|Name|Configuration|
|:-----|:---|:------------|
|1|CODE |ON or OFF, as you like|
|2|BAUD0|OFF|
|3|BAUD1|ON |
|4|TEST |OFF|

> Note: If you want to configure another serial speed to the M202MD10B,
> launch demos and test script with -B _speed_ option.

## Test and Demo

There are test and demo scripts in the ${base}/test directory.
These test scripts work with the circuit ["Futaba M202MD10B PC
Interface Board"](file://circuit/M202MD10BIF2.pdf "M202MD10B
interface circuit") which is described previous section.

### Run Test and Demo Scripts

Connect the M202MD10B to PC via the ["Futaba M202MD10B PC Interface
Board"](file://circuit/M202MD10BIF2.pdf "M202MD10B interface circuit").
And you check which node is the serial port connecting to the M202MD10B.
The node path will be appeared as /dev/ttyN (on cygwin),
/dev/ttyUSBn (on linux) or /dev/ttyACMn (on linux). _N_ or _n_ is the number of the port connected.

#### Display All Characters Demo

If configure serial speed as 2400bps.
```bash
cd ${base}/test
./char-show -F /dev/ttyUSBn 
```
If configure serial speed as you like, use -B _speed_.
```bash
cd ${base}/test
./char-show -F /dev/ttyUSBn -B speed
```

#### Control Code Effects Demo

If configure serial speed as 2400bps. Run demos as follows,
```bash
cd ${base}/test
./test-long-opt.sh -F /dev/ttyUSBn 
./test-short-opt.sh -F /dev/ttyUSBn 
```
These test scripts show how the m202md10b application command line options works,
|Script|Description|
|:--|:--|
|test-long-opt.sh |Shows long options to control display effects.|
|test-short-opt.sh|Shows short options to control display effects.|

If configure serial speed as you like, use -B _speed_.

```bash
cd ${base}/test
./test-long-opt.sh -F /dev/ttyUSBn -B speed
./test-short-opt.sh -F /dev/ttyUSBn -B speed
```

## Command Line m202md10b
The following shows m202md10db application command line syntax.

```bash
m202md10b --path=path [--options[=parameter] | "message"]... [-] [--]
```

Must specify serial port path by the option **--path=path** (in short format -F path). And it would be necessary to specify baud ratio by **--baud=_baud_** (in short format -B _baud_).

Options and messages to control VFD and display characters are send to port in order. For example, command line "hello" -f 2 "world" sends "hello\x09\x09world" to port.

Single dash '-' begin reading stdin and sends them to serial port. When reading from stdin ends (reached EOF), back to parsing command line.

Double dash '--' terminates parsing options, for example, command line "--" "-x" "-y" "-" sends "-x-y-" to serial port.

The following table shows command line options.

### Options
|short|long|description|
|:---|:---|:---|
|-F _path_|--path=_path_<br />--dev=_path_<br />--port=device |Set serial port device _path_.|
|-B _baud_|--baud=_baud_|Set baud rate.<br />baud = {1200, 2400<sup>(*)</sup>, 4800, 9600} |
|-C {y\|n}|--rts-cts={y\|n}|Set RTS-CTS flow control mode.<br />y: Enable, n: Disable<sup>(*)</sup>|
|-W _msec_|--wait=_msec_|Wait _msec_ milli second(s) per character. If you saw some characters are not shown on the panel display, specify this option and increase parameter value.|
|-R|--hard-reset<br />--break|Send hard reset (send break).|
|-a|--bright=4|Set bright level 4 (Max).|
|-b|--bright=3|Set bright level 3.|
|-c|--bright=2|Set bright level 2.|
|-d|--bright=1|Set bright level 1 (Min).|
|-h[_N_]|--back[=_N_]|Back cursor _N_ step(s).|
|-i[_N_]|--forward[=_N_]|Forward cursor _N_ step(s).|
|-j|--clear-all<br />--wipe|Clear all.|
|-l|--clear-all-goto-home<br />--cls|Clear all and Goto home.|
|-m|--goto-home<br />--home|Goto home.|
|-q|--round-trip-mode<br />--round|Set round trip mode.|
|-r|--scroll-mode<br />--scroll|Set scroll mode.|
|-s|--reset-blink-show-cursor|Reset blink and Show cursor.|
|-t|--hide-cursor<br />--hide|Hide cursor.|
|-u|--blink-cursor<br />--blink|Blink cursor.|
|-v|--under-line-cursor<br />--under|Under line cursor|
|-w|--block-cursor<br />--block|Block cursor.|
|-x|--reverse-cursor<br />--reverse|Reverse cursor.|
|-z _c_:_L0_:_L1_:_L2_:_L3_:_L4_:_L5_:_L6_|--define-char=_parameter_<br />--define=_parameter_|Define character _c_ bitmap as _L0_ to _L6_. _L0_ is top most, _L6_ is bottom most. LSB corresponds to left most pixel. _L0_.._L6_ are based on hex.<br />NOTE: The _parameter_ to long format option is same as short format option.|
|-[ _c_ \| _x,y_|--goto-position=_parameter_<br />--goto=_parameter_|Goto cursor position to _c_ or (_x_, _y_). _c_ = 0..39, _x_ = 0..19, _y_ = 0..1, The "Left Top" position is 0 or (0, 0). The "Right Bottom" position is 39 or (19, 1). NOTE: The _parameter_ to long format option is same as short format option.|
|-D|--debug|Debug mode|
|-H||Help short option usage|
||--help|Help long option usage|
|-||Read from stdin and send to display|
|--||End option.|
(*): Default value.
