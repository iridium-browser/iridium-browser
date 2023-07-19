---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/getting-dev-hardware
  - Getting Developer Hardware
page_name: dev-hardware-list
title: Developer Hardware
---

[TOC]

***Warning: This page is for developers who both know how to build Chromium OS
and aren't afraid to take a screwdriver to their computer.***

## Introduction

The list at the bottom of this page is a place for developers to contribute
information about which developer systems are known to run Chromium OS,
including what functionality works and what is broken. Please include details
only on systems based on Chromium OS that **do not require changes to build**
(no additional drivers, new modules, changes to config files, and so on).
Systems listed here need to **run the first time** with the latest, unmodified
Chromium OS source code.

Please note that this list is not an endorsement of any particular system or
hardware. This list is for developers to have systems to test and help other
developers find hardware on which they can develop Chromium OS. We encourage all
hardware vendors to to get their components working with Chromium OS. One of the
best ways to achieve this goal is open-source, high-quality drivers for your
components.

Also note that this list is updated by request; it can get stale, and the
presence of a device here is not a guarantee that Chromium OS will run on it.
(The fact that someone got it to run at one point makes it more likely that one
could get it to run again with a bit of work, though.) If you'd just like to
give Chromium OS a try, your best bet is to run it on a virtual machine, as
described on the [Running a Chromium OS image under
KVM](/chromium-os/how-tos-and-troubleshooting/running-chromeos-image-under-virtual-machines)
page.

If you have questions about getting Chromium OS to run or would like to help get
functionality working for a particular system, please join the [chromium-os-dev
discussion group](http://groups.google.com/group/chromium-os-dev).

### Conventions

The list has the following columns:

<table>
<tr>
Brand & Model Number
<td>A brand and model number that are specific enough to indicate major components, such as which CPU is used</td>
</tr>
<tr>
<td>If "OK", basic wifi functionality works: link comes up and connects to a network (open/WPA). "No" indicates it's known to be broken. "N/A" means not applicable: there is no wifi card. If buying a new machine, try to choose one with working wifi. If you have a machine with non-working wifi, we recommend replacing the mini-PCIe card with an Atheros 9285 or Intel part (most Intel cards work).</td> Wifi
</tr>
<tr>
Trackpad <td>If <a href="/chromium-os/getting-dev-hardware/dev-hardware-list/icon-checkmark.gif"><img alt="image" src="/chromium-os/getting-dev-hardware/dev-hardware-list/icon-checkmark.gif"></a>, the onscreen cursor can be controlled by the system's trackpad</td>
</tr>
<tr>
<td><b>Suspend/Resume</b></td>
<td>If <a href="/chromium-os/getting-dev-hardware/dev-hardware-list/icon-checkmark.gif"><img alt="image" src="/chromium-os/getting-dev-hardware/dev-hardware-list/icon-checkmark.gif"></a>, the system will suspend/resume when the lid is closed/opened or power button is pressed</td>
</tr>
<tr>
Comments & Caveats <td>Other notes about the system that are important for other developers to know</td>
</tr>
<tr>
Contact <td>Manufacturer contact email for hardware related questions about the system and Chromium OS</td>
</tr>
<tr>
<td>Help other developers find this system!</td> Buy Link
</tr>
</table>

If a feature doesn't work (no [<img alt="image"
src="/chromium-os/getting-dev-hardware/dev-hardware-list/icon-checkmark.gif">](/chromium-os/getting-dev-hardware/dev-hardware-list/icon-checkmark.gif)),
add a comment with details on how the feature doesn't work or how to fix it.

### How to edit the list

Anyone signed into an **@chromium.org** address can modify this list. (If you
don't see an **Add item** button below, [sign
in](https://www.google.com/a/UniversalLogin?md=chromium.org&continue=https%3A%2F%2Fsites.google.com%2Fa%2Fchromium.org%2Fdev%2Fchromium-os%2Fgetting-dev-hardware%2Fdev-hardware-list&service=jotspot).
One way to get an @chromium.org address is to be a
[committer](/getting-involved/become-a-committer).) In the spirit of wiki, the
community is encouraged to remove inaccurate or unreproducible results. Repeat
offenders will suffer the wrath of the wiki gods.

If you do not have an @chromium.org address, feel free to e-mail
[vapier@chromium.org](mailto:vapier@chromium.org) with the details you'd like to
have added below.

Note that all Chrome OS branded devices should also run Chromium OS. We won't
bother enumerating those.

## Developer hardware list

<table>
  <tr>
    <th>Brand & Model Number</th>
    <th>Wifi</th>
    <th>Ethernet</th>
    <th>Trackpad</th>
    <th>Suspend/Resume</th>
    <th>Comments & Caveats</th>
    <th>Contact</th>
    <th>Buy Link</th>
  </tr>
  <tr>
    <td>Fujitsu Lifebook T5010</td>
    <td>OK</td>
    <td>on</td>
    <td>on</td>
    <td>on</td>
    <td></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>Compaq 6720</td>
    <td>No</td>
    <td></td>
    <td></td>
    <td></td>
    <td></td>
    <td>juanmabaiu@gmail.com</td>
    <td></td>
  </tr>
  <tr>
    <td>Samsung NC10</td>
    <td>OK</td>
    <td>on</td>
    <td>on</td>
    <td>on</td>
    <td>Wifi: Atheros AR50007EG, Ethernet: Marvell Yukon 88E8040</td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>Toshiba Satellite A205-S5000</td>
    <td>OK</td>
    <td>on</td>
    <td>on</td>
    <td>on</td>
    <td>memory card reader doesn't work; acted laggy until sleep/resume was done</td>
    <td></td>
    <td>http://www.amazon.com/Toshiba-Satellite-A205-S5000-Celeron-1-86GHz/dp/B001NJOIC8/</td>
  </tr>
  <tr>
    <td>Asus Eee PC 1008HA Seashell 10.1-Inch Pearl Black Netbook</td>
    <td>OK</td>
    <td>on</td>
    <td>on</td>
    <td>on</td>
    <td></td>
    <td></td>
    <td>http://www.amazon.com/1008HA-Seashell-10-1-Inch-Pearl-Netbook/dp/B0029QMDZI</td>
  </tr>
  <tr>
    <td>Toshiba mini NB200/5 10.1 inch Netbook w/ fullsize keyboard</td>
    <td>OK</td>
    <td>on</td>
    <td>on</td>
    <td>on</td>
    <td>Intel GMA 950 graphics and 802.11b/g (Atheros LAN)</td>
    <td>http://laptops.toshiba.com/mini</td>
    <td>http://www.toshibadirect.com/td/b2c/laptops.to?catagory=Netbook</td>
  </tr>
  <tr>
    <td>Acer AOP531h</td>
    <td>?</td>
    <td>on</td>
    <td>on</td>
    <td>on</td>
    <td>This unit may ship with Atheros (works) or Broadcom (does not work) wifi module, you can check Network Adapters under Device Manager in Windows, or FCC stickers.</td>
    <td></td>
    <td>http://www.acer.com/worldwide/  select location and go to "Where to Buy" page for a list of vendors </td>
  </tr>
  <tr>
    <td>Asus Eee PC 901</td>
    <td>OK</td>
    <td>on</td>
    <td>on</td>
    <td>on</td>
    <td></td>
    <td></td>
    <td>http://computers.shop.ebay.com/PC-Laptops-Netbooks-/177/i.html?_nkw=EEEPC+901&_catref=1&_fln=1&_trksid=p3286.c0.m282 </td>
  </tr>
  <tr>
    <td>Samsung N150+ (Samsung NP-N150-KP01CZ)</td>
    <td>OK</td>
    <td></td>
    <td></td>
    <td>on</td>
    <td>Brightness is not configurable: high when powered by external source, low when running on battery. Suspend takes a while to finish.</td>
    <td>noxx2cz@gmail.com</td>
    <td></td>
  </tr>
  <tr>
    <td>Dell Mini 10V</td>
    <td>No</td>
    <td>on</td>
    <td>on</td>
    <td>on</td>
    <td>Need to replace wifi card - both Atheros and Intel verified to work</td>
    <td></td>
    <td>https://ecomm2.dell.com/dellstore/basket.aspx?c=us&cs=19&l=en&s=dhs&itemtype=CFG</td>
  </tr>
  <tr>
    <td>Acer AOD250</td>
    <td>?</td>
    <td>on</td>
    <td>on</td>
    <td>on</td>
    <td>This unit may ship with Atheros (works) or Broadcom (does not work) wifi module, you can check Network Adapters under Device Manager in Windows, or FCC stickers.</td>
    <td></td>
    <td>http://www.acer.com/worldwide/    select location and go to "Where to Buy" page for a list of vendors</td>
  </tr>
  <tr>
    <td>Asus Eee PC 900HA</td>
    <td>OK</td>
    <td>on</td>
    <td>on</td>
    <td>on</td>
    <td>fans may not work?</td>
    <td></td>
    <td>http://www.amazon.com/Eee-PC-900-8-9-Inch-Processor/dp/B00191PKJK</td>
  </tr>
  <tr>
    <td>Asus Eee PC 1000/w SSD</td>
    <td></td>
    <td>on</td>
    <td>on</td>
    <td>on</td>
    <td></td>
    <td></td>
    <td>http://www.amazon.com/10-Inch-Netbook-Processor-Storage-Battery/dp/B001BY97IU</td>
  </tr>
  <tr>
    <td>Packard Bell  DOTSR</td>
    <td>?</td>
    <td>on</td>
    <td>on</td>
    <td>on</td>
    <td>This unit may ship with Atheros (works) or Broadcom (does not work) wifi module, you can check Network Adapters under Device Manager in Windows, or FCC stickers.</td>
    <td></td>
    <td>Check attached file :ChromeOS Compatibility list-20091221-A.xlsx" tab Packard Bell Store by region: France: http://packardbell.fr/shopping/store_locator.html   </td>
  </tr>
  <tr>
    <td>Asus Eee PC 700</td>
    <td>OK</td>
    <td>on</td>
    <td>on</td>
    <td>on</td>
    <td></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>Diskless Workstations LTSP Term 1520</td>
    <td>N/A</td>
    <td>on</td>
    <td>on</td>
    <td>on</td>
    <td></td>
    <td>info@DisklessWorkstations.com </td>
    <td>http://www.DisklessWorkstations.com/cgi-bin/prod/200118.html</td>
  </tr>
  <tr>
    <td>Acer AO531h</td>
    <td>?</td>
    <td>on</td>
    <td>on</td>
    <td>on</td>
    <td>This unit may ship with Atheros (works) or Broadcom (does not work) wifi module, you can check Network Adapters under Device Manager in Windows, or FCC stickers.</td>
    <td></td>
    <td>http://www.acer.com/worldwide/  select location and go to "Where to Buy" page for a list of vendors</td>
  </tr>
  <tr>
    <td>Lenovo Ideapad S10</td>
    <td>?</td>
    <td>on</td>
    <td>on</td>
    <td>on</td>
    <td>Need to replace wifi card</td>
    <td></td>
    <td>http://shop.lenovo.com/SEUILibrary/controller/e/web/LenovoPortal/en_US/catalog.workflow:category.details?current-catalog-id=12F0696583E04D86B9B79B0FEC01C087&current-category-id=02695ADDF94544E5A11D24AEBC064493&menu-id=products</td>
  </tr>
  <tr>
    <td>Toshiba Satellite M200</td>
    <td>OK</td>
    <td></td>
    <td>on</td>
    <td></td>
    <td>Intel Core 2 Duo + inbuilt Intel Graphics Media Accelerator.  Sound & WebCam works too.</td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>Asus Eee PC 1005HA</td>
    <td>OK</td>
    <td>on</td>
    <td>on</td>
    <td>on</td>
    <td></td>
    <td></td>
    <td>http://www.amazon.com/s/ref=nb_ss?url=search-alias%3Delectronics&field-keywords=1005HA&x=0&y=0</td>
  </tr>
  <tr>
    <td>Lenovo IBM ThinkPad R60e</td>
    <td>OK</td>
    <td>on</td>
    <td>on</td>
    <td>on</td>
    <td></td>
    <td>petrmenzel@gmail.com</td>
    <td>http://www.notebookreview.com/notebookreview/lenovo-thinkpad-r60-review-pics-specs/</td>
  </tr>
  <tr>
    <td>Asus Eee PC 1005HG</td>
    <td>OK</td>
    <td></td>
    <td></td>
    <td>on</td>
    <td></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>Sony VAIO W Series VPCW121AX/W</td>
    <td>OK</td>
    <td>on</td>
    <td>on</td>
    <td>on</td>
    <td></td>
    <td></td>
    <td>http://www.sonystyle.com/webapp/wcs/stores/servlet/ProductDisplay?catalogId=10551&storeId=10151&langId=-1&productId=8198552921665974276</td>
  </tr>
  <tr>
    <td>HP S372OY</td>
    <td>Built-in doesn't work</td>
    <td>on</td>
    <td>on</td>
    <td>on</td>
    <td>Bought and used netgear usb network adapter wg111us which worked great</td>
    <td></td>
    <td>http://www.amazon.com/Netgear-WG111US-Wireless-Usb-Adapter/dp/B0009N544A</td>
  </tr>
  <tr>
    <td>Bangho B-x0x1</td>
    <td>No</td>
    <td></td>
    <td>on</td>
    <td></td>
    <td></td>
    <td>juanmabaiu@gmail.com</td>
    <td></td>
  </tr>
  <tr>
    <td>Packard Bell  DOTS</td>
    <td>?</td>
    <td>on</td>
    <td>on</td>
    <td>on</td>
    <td>This unit may ship with Atheros (works) or Broadcom (does not work) wifi module, you can check Network Adapters under Device Manager in Windows, or FCC stickers.</td>
    <td></td>
    <td>Check attached file :ChromeOS Compatibility list-20091221-A.xlsx" tab Packard Bell Store by region: France: http://packardbell.fr/shopping/store_locator.html     </td>
  </tr>
  <tr>
    <td>MSI Megabook VR201 ("Model Nr. MS-1217")</td>
    <td>OK</td>
    <td>on</td>
    <td>on</td>
    <td></td>
    <td>Suspends but does not wake up anymore. It shuts down after trying to wake it up by pressing the power button. Sound is fine.</td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>HP Mini 5101</td>
    <td>No</td>
    <td>on</td>
    <td>on</td>
    <td>on</td>
    <td>Make sure to specify Intel wireless module when customizing.</td>
    <td></td>
    <td>http://h71016.www7.hp.com/MiddleFrame.asp?page=config&ProductLineId=539&FamilyId=3066&BaseId=31126&oi=E9CED&BEID=19701&SBLID=</td>
  </tr>
  <tr>
    <td>Fujitsu Lifebook A6030</td>
    <td>OK</td>
    <td>on</td>
    <td>on</td>
    <td>on</td>
    <td></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>Gatteway LT20</td>
    <td>?</td>
    <td>on</td>
    <td>on</td>
    <td>on</td>
    <td>This unit may ship with Atheros (works) or Broadcom (does not work) wifi module, you can check Network Adapters under Device Manager in Windows, or FCC stickers.</td>
    <td></td>
    <td>Check attached file :ChromeOS Compatibility list-20091221-A.xlsx" tab Gateway Store by region:  USA/Canada: http://www.gateway.com/retail/consumer.php?cmpid=topnav_shop    Mexico: http://mx.gateway.com/products/stores.html    </td>
  </tr>
  <tr>
    <td>Asus K42JK</td>
    <td>OK</td>
    <td>on</td>
    <td>on</td>
    <td></td>
    <td></td>
    <td>pratomoasta@gmail.com</td>
    <td>-</td>
  </tr>
  <tr>
    <td>HP Compaq 6820s</td>
    <td>No</td>
    <td>on</td>
    <td>on</td>
    <td></td>
    <td></td>
    <td>Petr Menzel</td>
    <td></td>
  </tr>
  <tr>
    <td>Acer Aspire One AOD250-1165 10.1-Inch Blue Netbook</td>
    <td>?</td>
    <td>on</td>
    <td>on</td>
    <td>on</td>
    <td>This unit may ship with Atheros (works) or Broadcom (does not work) wifi module, you can check Network Adapters under Device Manager in Windows, or FCC stickers.</td>
    <td></td>
    <td>http://www.amazon.com/Acer-Aspire-AOD250-1165-10-1-Inch-Netbook/dp/B0023B138W</td>
  </tr>
  <tr>
    <td>Lenovo IdeaPad S12</td>
    <td>?</td>
    <td>on</td>
    <td>on</td>
    <td>on</td>
    <td>Need to replace WiFi card, need to select Intel Integrated graphics models only (NVidia and VIA graphics not supported)</td>
    <td></td>
    <td>http://shop.lenovo.com/SEUILibrary/controller/e/web/LenovoPortal/en_US/catalog.workflow:category.details?current-catalog-id=12F0696583E04D86B9B79B0FEC01C087&current-category-id=8EE56652C33D4452A778393130C14F42</td>
  </tr>
  <tr>
    <td>HP Compaq 6720s</td>
    <td>?</td>
    <td></td>
    <td></td>
    <td></td>
    <td>touchpad does not work</td>
    <td>joeboy95619@gmail.com</td>
    <td></td>
  </tr>
  <tr>
    <td>Packard Bell EasyNote TS11-HR-868</td>
    <td>OK</td>
    <td>on</td>
    <td></td>
    <td>on</td>
    <td>function keys are working</td>
    <td></td>
    <td>laptop is not being sold anymore (bought it Jul 2011)</td>
  </tr>
</table>
