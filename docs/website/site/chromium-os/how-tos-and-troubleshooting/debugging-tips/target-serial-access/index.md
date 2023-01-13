---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/how-tos-and-troubleshooting
  - How Tos and Troubleshooting
- - /chromium-os/how-tos-and-troubleshooting/debugging-tips
  - Chromium on Chromium OS Debugging Tips
page_name: target-serial-access
title: Target Serial Access
---

[TOC]

It is useful to be able to get a terminal on your target device. This page lists
some options for this. Instructions assume Ubuntu Lucid and a 115200 serial
link. Also you should be using a serial null modem cable.

## USB Serial Dongle

This is a common way of getting a serial port on a computer. See [TRENDnet USB
to Serial
converter](http://www.google.com/url?q=http%3A%2F%2Ftrendnet.com%2Fproducts%2Fproddetail.asp%3Fprod%3D150_TU-S9%26cat%3D49&sa=D&sntz=1&usg=AFrqEzej3O8N15gqmTe0cLvyXkQt6wK5uQ)
for example.

## ser2net

This is a convenient tool which allows you to 'telnet' into your board over a
serial link. These instructions assume you are using a single USB serial dongle
(/dev/ttyUSB0) but you can also use a real serial port (/dev/ttyS0) if you like.
If you have an FTDI serial device providing serial access then it will use
/dev/ttyACM0.

```none
sudo apt-get install ser2net
```

Edit /etc/ser2net.conf and add this line:

```none
4000:telnet:600:/dev/ttyUSB0:115200 8DATABITS NONE 1STOPBIT banner
```

Then restart so that ser2net sees your changes:

```none
sudo /etc/init.d/ser2net restart
```

You can now connect to the board with:

```none
telnet localhost 4000
```

To disconnect, press Ctrl-\\ then Ctrl-D.

## screen

Screen is a useful tool for maintaining connections which you can switch between
easily. It is often used to run a long-running job such that you can logout,
then later login, reconnect and check progress. You can also use screen to get a
serial link:

```none
screen -t 'ttyUSB0 115200 8n1' /dev/ttyUSB0 115200,-ixoff,-ixon
```

Reset the machine and U-Boot should start printing to your terminal.
For help, use Ctrl-A Ctrl-?. To disconnect, press Ctrl-A Ctrl-D and to quit the
session Ctrl-A Ctrl-\\

## minicom

Minicom is a terminal program, intended for use with a modem, that
works well as an intermediary to any remote system connect through a
serial port.
One big advantage of minicom is that it allows one to easily capture
text into a file.

### Minicom Installation

To install minicom, execute the following

```none
  sudo apt-get install minicom
```

### Minicom Configuration

```none
  sudo minicom -s
```

The configuration is done through a simple menu system. Use the options below
for a basic working configuration.

```none
  Select 'Serial Port Setup' using the arrow keys; press <Enter>
  Select 'A': enter the name of your serial port.  (ex: /dev/ttyUSB0)
  Select 'E': select 'E' (115200), then 'Q' (8N1)
  Set 'Hardware Flow Control' to 'No.
  Set 'Software Flow Control' to 'No'.
  Press <Enter>; this brings you back to the main menu (titled '[configuration]')
  Select 'Save setup as dfl'; press <Enter>.
  Select 'Exit from Minicom'; press <Enter>.
```

### Execution

Minicom needs access to the configured serial port. This means you
either need to change the permissions on that port to allow your
normal user to access the port in read/write mode, or run using
'sudo'.

```none
 sudo minicom
```

#### Minicom commands

All commands are prefixed with &lt;Ctrl-A&gt;, which will be abbreviated
C-a.

```none
<table>
```
```none&#10;<tr>&#10;```
```none&#10;<td> Key</td>&#10;```
```none&#10;<td> Result</td>&#10;```
```none&#10;</tr>&#10;```
```none&#10;<tr>&#10;```
```none&#10;<td>C-a Z&#10;</td>&#10;```
```none&#10;<td>Opens interactive menu</td>&#10;```
```none&#10;</tr>&#10;```
```none&#10;<tr>&#10;```
```none&#10;<td>C-a C&#10;</td>&#10;```
```none&#10;<td>Clear Screen</td>&#10;```
```none&#10;</tr>&#10;```
```none&#10;<tr>&#10;```
```none&#10;<td>C-a L</td>&#10;```
```none&#10;<td>Prompt for capture log file name.</td>&#10;```
```none&#10;</tr>&#10;```
```none&#10;<tr>&#10;```
```none&#10;<td>C-a L</td>&#10;```
```none&#10;<td>(after opening log file) Pause or close log file</td>&#10;```
```none&#10;</tr>&#10;```
```none&#10;<tr>&#10;```
```none&#10;<td>C-a Q&#10;</td>&#10;```
```none&#10;<td> Quit (w/o modem reset)</td>&#10;```
```none&#10;</tr>&#10;```
```none&#10;<tr>&#10;```
```none&#10;<td>C-a X &#10;</td>&#10;```
```none&#10;<td> Quit immediately</td>&#10;```
```none&#10;</tr>&#10;```
```none&#10;<tr>&#10;```
```none&#10;<td>C-a E</td>&#10;```
```none&#10;<td>Toggle echo of locally typed characters</td>&#10;```
```none&#10;</tr>&#10;```
```none&#10;<tr>&#10;```
```none&#10;<td>C-a W&#10;</td>&#10;```
```none&#10;<td>Toggle Line Wrap&#10;</td>&#10;```
```none&#10;</tr>&#10;```
```none&#10;<tr>&#10;```
```none&#10;<td>C-a T&#10;</td>&#10;```
```none&#10;<td>Select terminal settings</td>&#10;```
```none&#10;</tr>&#10;```
```none&#10;</table>&#10;```

## ssh

Once you have network access, and assuming you are using a test build, you can
use ssh to connect to your target.

```none
ssh chronos@192.168.2.3
(password test0000)
```
