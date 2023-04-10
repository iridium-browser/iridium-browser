---
breadcrumbs:
- - /chromium-os
  - Chromium OS
page_name: cable-and-adapter-tips-and-tricks
title: USB Type-C Cable and Adapter Tips and Tricks
---

This page provides reference information for manufacturers of USB-C parts. It
addresses common misunderstandings and errors in building legacy cables (Type-A
or microB to USB-C) and power adapters. For complete specifications, tolerances
and application rules, see the latest version of the [USB Type-C
specification](http://www.usb.org/developers/docs/).

## Legacy Cables

The following table describes the type of resistor and configuration (pull-up or
pull-down) that is required for each type of legacy cable. All cables must be
capable of supporting 3A, regardless of Rp/Rd.

<table>
<tr>
<td>**Resistor Rp (pull-up between VBUS and CC)**</td>
<td>**Resistor Rd (pull-down between CC and GND)**</td>
<td>**ID Pin**</td>
<td>**USB-C Spec Section**</td>
</tr>
<tr>
<td>USB Type-C plug to USB 3.1 Type-A plug</td>
<td>56 kΩ Rp</td>
<td>Open</td>
<td>Section 3.5.1</td>
</tr>
<tr>
<td>USB-C plug to USB 2.0 Type-A plug</td>
<td>56 kΩ Rp</td>
<td>Open</td>
<td>Section 3.5.2</td>
</tr>
<tr>
<td>USB-C plug to Type-A receptacle</td>
<td>Open</td>
<td>5.1 kΩ Rd</td>
<td>Section 3.6.1</td>
</tr>
<tr>
<td>USB-C receptacle to microB plug</td>
<td>Not allowed per the spec</td>
<td>Section 2.2</td>
</tr>
<tr>
<td>USB-C plug to microB plug</td>
<td>Open</td>
<td>5.1 kΩ Rd</td>
<td>unconnected</td>
<td>Section 3.5.7</td>
</tr>
<tr>
<td>USB-C plug to microB receptacle</td>
<td>56 kΩ Rp</td>
<td>Open</td>
<td>unconnected</td>
<td>Section 3.6.2</td>
</tr>
</table>

### Cable FAQ

*   **My legacy cable conforms to the specification and is rated for 3A.
            Can I use the Rp 3A resistor?**
    *   No. You must use the 56 kΩ Rp. The resistor indicates the
                capability of the power adapter, not the capability of the
                cable. It is not safe to use the 3A Rp: if the power adapter is
                not capable of providing 3A it could overheat.
*   **Why does my cable need to be rated for 3A if only the standard Rp
            is used?**
    *   The standard Rp indicates that the power sink needs to use some
                other method of finding what current the power source can
                provide. This method could be BC1.2 or a proprietary discovery
                scheme (such as the voltage set on D+/D-). Using these methods,
                a sink could discover the source is capable of up to 3A. *All
                cables must be capable of supporting 3A.*

## Power Adapters

For USB-C power adapters, the following table specifies the resistor type and
configuration.

<table>
<tr>
<td>**Pull-up on CC1**</td>
<td>**Pull-up on CC2**</td>
<td>**VBUS**</td>
</tr>
<tr>
<td>5V 3A power adapter with USB-C receptacle</td>
<td>10 kΩ</td>
<td>10 kΩ</td>
<td>Cold</td>
</tr>
<tr>
<td>5V 1.5A power adapter with USB-C receptacle</td>
<td>22 kΩ</td>
<td>22 kΩ</td>
<td>Cold</td>
</tr>
<tr>
<td>5V 3A power adapter with captive USB-C cable</td>
<td>10 kΩ</td>
<td>open</td>
<td>Cold or Hot</td>
</tr>
<tr>
<td>5V 1.5A power adapter with captive USB-C cable</td>
<td>22 kΩ</td>
<td>open</td>
<td>Cold or Hot</td>
</tr>
</table>

VBUS Cold : When nothing is attached to the USB-C receptacle or plug, VBUS must
be 0V or vSafe0V. 5V shall be applied to VBUS only when a UFP is detected by
monitoring voltage on the CC pin. 5V should only be applied when voltage vRd on
CC is 0.85V &lt; vRd &lt; 2.45V for a 3A power source. Please see Tables 4-23,
4-24, and 4-25 of the Type-C specification for the appropriate values of vRd
minimum and maximum voltages for Default USB Power, 1.5A, and 3.0A levels.

VBUS Hot : When nothing is attached to the USB-C plug, 5V may be applied to
VBUS.

Important Note : If your charger implements USB Power Delivery, regardless of
connector type (Receptacle or captive cable), VBUS Cold is required.

### USB Battery Charging v1.2

For chargers with a USB-C receptacle, it is highly recommended the port also
support USB Battery Charging v1.2 in order to allow legacy devices using Type-A
plugs or Micro-B receptacles to charge.

To implement a BC1.2 Dedicated Charging Port (DCP), D+ and D- lines in the
receptacle must be shorted together. Please see the [USB Battery Charging v1.2
Spec](http://www.usb.org/developers/docs/devclass_docs/BCv1.2_070312.zip) for
more details on how to implement DCP or CDP.

### USB PD Power Rules

Power adapters with maximum power &lt;= 15W may support USB Power Delivery.
Power adapters with maximum power &gt; 15W must support USB Power Delivery. When
initially specifying the voltage and current capability of an adapter which
supplies &gt; 15W, pay close attention to Power Rules, in USB PD R2.0 V1.2
Section 10.

[<img alt="image"
src="/chromium-os/cable-and-adapter-tips-and-tricks/SourcePowerRule.png">](/chromium-os/cable-and-adapter-tips-and-tricks/SourcePowerRule.png)

USB PD Revision 2.0 specifies normative voltage rails of 5V, 9V, 15V, and 20V.
In order to support a particular voltage rail, all voltage rails lower must be
supported up to 3A.

### Power Adapter FAQ

*   **In a 5V 3A or 5V 1.5A supply can I connect CC1 and CC2 and use a
            single shared resistor?**
    *   No. This design fails with active cables, emarked cables or any
                device that requests Vconn. These cables have an Ra pulldown on
                one of the pins, preventing accurate detection of CC voltage if
                the pins are shorted at the adapter.
*   **May a Type-C charger support a proprietary method of changing
            VBUS** in addition to or instead of USB Power Delivery? (For
            example, Qualcomm QuickCharge, MediaTek PumpExpress, others)
    *   No. Section 4.8.2 of the USB Type-C Specification explicitly
                forbids proprietary methods that change VBUS from the default
                voltage defined by USB 2.0 and USB 3.1 specifications (maximum
                5.5V). This applies to both power sources and power sinks. If
                the power adapter incorporates a Type-C plug or a Type-C
                receptacle, that connector must not support any dynamic voltage
                method other than USB Power Delivery.
