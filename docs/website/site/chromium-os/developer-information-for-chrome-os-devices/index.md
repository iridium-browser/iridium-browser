---
breadcrumbs:
- - /chromium-os
  - Chromium OS
page_name: developer-information-for-chrome-os-devices
title: Developer Information for Chrome OS Devices
---

## Introduction

This page provides developer-related information for Chrome devices. These are
systems that *shipped from the factory* with Google Chrome OS on them. For
details about Google Chrome OS and how it differs from Chromium OS, see the
[note in the FAQ about Google Chrome
OS](http://www.chromium.org/chromium-os/chromium-os-faq#TOC-What-s-the-difference-between-Chrom).
Chrome OS devices typically require special setup in order to be used for
Chromium OS development.

**Caution: Modifications you make to the system are not supported by Google, may
cause hardware, software or security issues and may void warranty.**

> Remember: Chrome OS devices are **not** general-purpose PCs. We believe you
> should be able to hack on your own property, but if you do it's not our fault
> if something breaks.

Googlers not finding what they're looking for may want to look at
[go/cros-names](http://goto.google.com/cros-names).

### End of Life (EOL) / Auto Update Expiration (AUE)

The [official Google policy](https://support.google.com/chrome/a/answer/6220366)
includes projected dates. Here we focus on the corresponding release versions as
that is often more useful to developers.

## Routers

These WiFi routers are marketed as [OnHub](https://on.google.com/hub/), [Google
Wifi](https://store.google.com/us/product/google_wifi), and [Nest
Wifi](https://store.google.com/us/product/nest_wifi).

<table>
<tr>
<td> <b>Release date</b></td>
<td><b> Manufacturer</b></td>
<td><b> Model</b></td>
<td><b> Project code name</b></td>
<td><b> Board name(s)</b></td>
<td> <b>Base board</b></td>
</tr>
<tr>
<td> August 2015</td>
<td> TP-LINK</td>
<td> <a href="/chromium-os/developer-information-for-chrome-os-devices/tp-link-onhub-tgr1900">OnHub Router TGR1900</a></td>
<td> Whirlwind</td>
<td> whirlwind</td>
<td> storm</td>
</tr>
<tr>
<td> November 2015</td>
<td> ASUS</td>
<td> OnHub SRT-AC1900</td>
<td> Arkham</td>
<td> arkham</td>
<td> storm</td>
</tr>
<tr>
<td> November 2016</td>
<td> Google</td>
<td> <a href="https://madeby.google.com/wifi/">Google WiFi</a></td>
<td> Gale</td>
<td> gale</td>
<td> gale</td>
</tr>
<tr>
<td> November 2019</td>
<td> Google</td>
<td> <a href="https://store.google.com/us/product/nest_wifi">Nest Wifi router</a> (not "point")</td>
<td> Mistral</td>
<td> mistral</td>
<td> mistral</td>
</tr>
</table>

## USB Type-C

<table>
<tr>
<td><b>Release date</b></td>
<td><b> Model</b></td>
<td><b> Board name(s)</b></td>
</tr>
<tr>
<td> January 2015</td>
<td> <a href="/chromium-os/dingdong">USB Type-C to DP Adapter</a></td>
<td> dingdong</td>
</tr>
<tr>
<td> January 2015</td>
<td> <a href="/chromium-os/hoho">USB Type-C to HDMI Adapter</a></td>
<td> hoho</td>
</tr>
<tr>
<td> January 2015</td>
<td> USB Type-C to VGA Adapter</td>
<td> hoho, but substitute a DP to VGA chip</td>
</tr>
<tr>
<td> January 2015</td>
<td> <a href="/chromium-os/twinkie">USB-PD Sniffer</a></td>
<td> twinkie</td>
</tr>
</table>

## Notebooks and Desktops

These are your standard Chromebook/Chromebox/etc devices.

<table>
  <tr>
    <th>Release</th>
    <th>OEM</th>
    <th>Model</th>
    <th>Code name</th>
    <th>Board name(s)</th>
    <th>Base board</th>
    <th>User ABI</th>
    <th>Kernel</th>
    <th>Kernel ABI</th>
    <th>Platform</th>
    <th>Form Factor</th>
    <th>First Release</th>
    <th>EOL/AUE</th>
    <th>USB Gadget</th>
    <th>Closed Case Debugging</th>
  </tr>
   <tr>
    <td></td>
    <td>HP</td>
    <td><a href="https://www.hp.com/us-en/shop/pdp/hp-elite-dragonfly-135-inch-chromebook">HP Elite Dragonfly 13.5 Chromebook</a></td>
    <td>redrix</td>
    <td>Brya</td>
    <td>Brya </td>
    <td>x86_64</td>
    <td>5.10</td>
    <td>x86_64</td>
    <td>AlderLake-U</td>
    <td>Convertible</td>
    <td>R100</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td></td>
    <td>Lenovo</td>
    <td><a href="https://www.lenovo.com/us/en/coming-soon/IdeaPad-F5-CB-13CML-05/p/88IPFC51448">IdeaPad Flex 5i Chromebook (13", 5) </a></td>
    <td>Akemi</td>
    <td>Hatch</td>
    <td>Hatch </td>
    <td>x86_64</td>
    <td>4.19</td>
    <td>x86_64</td>
    <td>Cometlake-U</td>
    <td>Convertible</td>
    <td>R81</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td></td>
    <td>Lenovo</td>
    <td><a href="https://www.lenovo.com/gb/en/laptops/lenovo/student-chromebooks/100e-AMD-G3/p/22ED01E1EA3">Lenovo 100e Chromebook Gen 3 AMD</a></td>
    <td>vilboz</td>
    <td>zork</td>
    <td>dalboz</td>
    <td>x86_64</td>
    <td>5.4</td>
    <td>x86_64</td>
    <td>Pollock</td>
    <td>Chromebook</td>
    <td>M91</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td></td>
    <td>HP</td>
    <td><a href="https://support.hp.com/sg-en/document/c06719265">HP Elite c1030 Chromebook</a></td>
    <td>jinlon</td>
    <td>Hatch</td>
    <td>Hatch</td>
    <td>x86_64</td>
    <td>4.19</td>
    <td>x86_64</td>
    <td>Comet Lake-U</td>
    <td>Convertible</td>
    <td>R84</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td></td>
    <td>Google</td>
    <td><a href="https://store.google.com/us/product/pixelbook_go">Pixelbook Go</a></td>
    <td>atlas</td>
    <td>atlas</td>
    <td>poppy</td>
    <td>x86_64</td>
    <td>4.4</td>
    <td>x86_64</td>
    <td>Amberlake-Y</td>
    <td>Chromebook</td>
    <td>M75</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td>Yes</td>
    <td>Yes</td>
  </tr>
  <tr>
    <td></td>
    <td>Asus</td>
    <td><a href="https://www.asus.com/Laptops/For-Home/Chromebook/ASUS-Chromebook-Flip-CM3-CM3200/">ASUS Chromebook Flip CM3 (CM3200)</a></td>
    <td>Damu</td>
    <td>jacuzzi</td>
    <td>jacuzzi</td>
    <td>arm</td>
    <td>4.19</td>
    <td>aarch64</td>
    <td>MT8183</td>
    <td>Convertible</td>
    <td>R87</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td></td>
    <td>Lenovo</td>
    <td><a href="https://www.aboutchromebooks.com/news/mwc-2021-lenovo-chromebook-5i-and-flex-5i-debut-look-great-on-paper/">IdeaPad Flex 5i Chromebook</a></td>
    <td>lillipup</td>
    <td>volteer</td>
    <td>volteer</td>
    <td>x86_64</td>
    <td>5.4</td>
    <td>x86_64</td>
    <td>TigerLake-UP3</td>
    <td>Chromebook</td>
    <td>R90</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td></td>
    <td>HP</td>
    <td><a href="">HP Chromebook x360 14b / HP Chromebook x360 14a</a></td>
    <td>Blooguard</td>
    <td>octopus</td>
    <td>octopus</td>
    <td>x86_64</td>
    <td>4.14</td>
    <td>x86_64</td>
    <td>Gemini Lake</td>
    <td>Convertible</td>
    <td></td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td></td>
    <td>Asus</td>
    <td><a href="https://www.asus.com/Laptops/For-Home/Chromebook/ASUS-Chromebook-Detachable-CM3-CM3000/"> ASUS Chromebook Detachable CM3 (CM3000)</a></td>
    <td>Kakadu</td>
    <td>kukui</td>
    <td>kukui</td>
    <td>arm</td>
    <td>4.19</td>
    <td>aarch64</td>
    <td>MT8183</td>
    <td>Chromeblet</td>
    <td>R88</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td></td>
    <td>Acer</td>
    <td><a href="https://www.acer.com/ac/en/US/content/series/acerchromebook712">Acer Chromebook 712</a></td>
    <td>Kindred</td>
    <td>Hatch </td>
    <td>Hatch</td>
    <td>x86_64</td>
    <td>4.19</td>
    <td>x86_64</td>
    <td>Cometlake-U</td>
    <td>Chromebook</td>
    <td>R79</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td></td>
    <td>Lenovo</td>
    <td><a href="https://www.lenovo.com/us/en/coming-soon/300e-AMD-G3/p/22ED03E3EA3">Lenovo 300e Chromebook Gen 3 AMD</a></td>
    <td>vilboz360</td>
    <td>zork</td>
    <td>dalboz</td>
    <td>x86_64</td>
    <td>5.4</td>
    <td>x86_64</td>
    <td>Pollock</td>
    <td>Chromebook</td>
    <td>R91</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td></td>
    <td>Asus</td>
    <td><a href="https://www.asus.com/Laptops/For-Home/Chromebook/ASUS-Chromebook-Flip-CM3-CM3200/">ASUS Chromebook Flip CM3 (CM3200)</a></td>
    <td>Hayato</td>
    <td>asurada</td>
    <td>asurada</td>
    <td>arm</td>
    <td>5.4</td>
    <td>aarch64</td>
    <td>MT8192</td>
    <td>Chromebook</td>
    <td>M91</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2010-12-05</td>
    <td>Google</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/cr-48-chrome-notebook-developer-information">Cr-48</a></td>
    <td>Mario</td>
    <td>x86-mario</td>
    <td></td>
    <td>x86</td>
    <td>3.8</td>
    <td>x86</td>
    <td>PineTrail</td>
    <td>Chromebook</td>
    <td>~R8</td>
    <td>R56</td>
    <td></td>
    <td>No</td>
  </tr>
  <tr>
    <td>2011-05-30</td>
    <td>Samsung</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/samsung-series-5-chromebook">Samsung Series 5 Chromebook</a></td>
    <td>Alex</td>
    <td>x86-alex & x86-alex_he</td>
    <td></td>
    <td>x86</td>
    <td>3.8</td>
    <td>x86</td>
    <td>PineTrail</td>
    <td>Chromebook</td>
    <td>R11</td>
    <td>R58</td>
    <td></td>
    <td>No</td>
  </tr>
  <tr>
    <td>2011-06-30</td>
    <td>Acer</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/acer-ac700-chromebook">Acer AC700 Chromebook</a></td>
    <td>ZGB</td>
    <td>x86-zgb & x86-zgb_he</td>
    <td></td>
    <td>x86</td>
    <td>3.8</td>
    <td>x86</td>
    <td>PineTrail</td>
    <td>Chromebook</td>
    <td>R12</td>
    <td>R58</td>
    <td></td>
    <td>No</td>
  </tr>
  <tr>
    <td>2012-04-29</td>
    <td>Samsung</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/samsung-sandy-bridge">Samsung Chromebook Series 5 550</a></td>
    <td>Lumpy</td>
    <td>lumpy</td>
    <td></td>
    <td>x86_64</td>
    <td>3.8</td>
    <td>x86_64</td>
    <td>SandyBridge</td>
    <td>Chromebook</td>
    <td>R17</td>
    <td>R65</td>
    <td></td>
    <td>No </td>
  </tr>
  <tr>
    <td>2012-07-01</td>
    <td>Samsung</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/samsung-sandy-bridge">Samsung Chromebox Series 3</a></td>
    <td>Stumpy</td>
    <td>stumpy</td>
    <td></td>
    <td>x86_64</td>
    <td>3.8</td>
    <td>x86_64</td>
    <td>SandyBridge</td>
    <td>Chromebox</td>
    <td>R17</td>
    <td>R65</td>
    <td></td>
    <td>No</td>
  </tr>
  <tr>
    <td>2012-10-14</td>
    <td>Samsung</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/samsung-arm-chromebook">Samsung Chromebook</a></td>
    <td>Snow</td>
    <td>daisy</td>
    <td>daisy</td>
    <td>arm</td>
    <td>3.8</td>
    <td>arm</td>
    <td>Exynos 5250</td>
    <td>Chromebook</td>
    <td>R22</td>
    <td>R75</td>
    <td></td>
    <td>No </td>
  </tr>
  <tr>
    <td>2012-10-30</td>
    <td>Acer</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/acer-c7-chromebook">Acer C7 Chromebook</a></td>
    <td>Parrot</td>
    <td>parrot_ivb</td>
    <td>parrot</td>
    <td>x86_64</td>
    <td>3.8</td>
    <td>x86_64</td>
    <td>IvyBridge</td>
    <td>Chromebook</td>
    <td>R27</td>
    <td>R69</td>
    <td></td>
    <td>No</td>
  </tr>
  <tr>
    <td>2012-10-30</td>
    <td>Acer</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/acer-c7-chromebook">Acer C7 Chromebook</a></td>
    <td>Parrot</td>
    <td>parrot</td>
    <td></td>
    <td>x86_64</td>
    <td>3.8</td>
    <td>x86_64</td>
    <td>SandyBridge</td>
    <td>Chromebook</td>
    <td>R21</td>
    <td>R65</td>
    <td></td>
    <td>No</td>
  </tr>
  <tr>
    <td>2012-12-30</td>
    <td>Lenovo</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/lenovo-thinkpad-x131e-chromebook">Lenovo Thinkpad X131e Chromebook</a></td>
    <td>Stout</td>
    <td>stout</td>
    <td></td>
    <td>x86_64</td>
    <td>3.8</td>
    <td>x86_64</td>
    <td>IvyBridge</td>
    <td>Chromebook</td>
    <td>R22</td>
    <td>R69</td>
    <td></td>
    <td>No</td>
  </tr>
  <tr>
    <td>2013-01-30</td>
    <td>Google</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/chromebook-pixel">Google Chromebook Pixel</a></td>
    <td>Link</td>
    <td>link</td>
    <td></td>
    <td>x86_64</td>
    <td>3.8</td>
    <td>x86_64</td>
    <td>IvyBridge</td>
    <td>Chromebook</td>
    <td>R22</td>
    <td>R69</td>
    <td></td>
    <td>No</td>
  </tr>
  <tr>
    <td>2013-01-30</td>
    <td>HP</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/hp-pavilion-14-chromebook">HP Pavilion Chromebook 14</a></td>
    <td>Butterfly</td>
    <td>butterfly</td>
    <td></td>
    <td>x86_64</td>
    <td>3.8</td>
    <td>x86_64</td>
    <td>SandyBridge</td>
    <td>Chromebook</td>
    <td>R22</td>
    <td>R65</td>
    <td></td>
    <td>No</td>
  </tr>
  <tr>
    <td>2013-09-12</td>
    <td>Acer</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/acer-c720-chromebook">Acer C720/C70P/C740 Chromebook</a></td>
    <td>Peppy & Pepto</td>
    <td>peppy</td>
    <td>slippy</td>
    <td>x86_64</td>
    <td>3.8</td>
    <td>x86_64</td>
    <td>Haswell</td>
    <td>Chromebook</td>
    <td>R30</td>
    <td>R76</td>
    <td></td>
    <td>No</td>
  </tr>
  <tr>
    <td>2013-09-29</td>
    <td>HP</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/hp-chromebook-11">HP Chromebook 11 G1</a></td>
    <td>Spring</td>
    <td>daisy_spring</td>
    <td>snow</td>
    <td>arm</td>
    <td>3.8</td>
    <td>arm</td>
    <td>Exynos 5250</td>
    <td>Chromebook</td>
    <td>R27</td>
    <td>R72</td>
    <td></td>
    <td>No</td>
  </tr>
  <tr>
    <td>2013-09-29</td>
    <td>HP</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/hp-chromebook-14">HP Chromebook 14</a></td>
    <td>Falco</td>
    <td>falco & falco_li</td>
    <td>slippy</td>
    <td>x86_64</td>
    <td>3.8</td>
    <td>x86_64</td>
    <td>Haswell</td>
    <td>Chromebook</td>
    <td>R30</td>
    <td>R76</td>
    <td></td>
    <td>No</td>
  </tr>
  <tr>
    <td>2014-01-15</td>
    <td>Dell</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/dell-chromebook-11">Dell Chromebook 11</a></td>
    <td>Wolf</td>
    <td>wolf</td>
    <td>slippy</td>
    <td>x86_64</td>
    <td>3.8</td>
    <td>x86_64</td>
    <td>Haswell</td>
    <td>Chromebook</td>
    <td>R31</td>
    <td>R76</td>
    <td></td>
    <td>No</td>
  </tr>
  <tr>
    <td>2014-01-30</td>
    <td>Toshiba</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/toshiba-cb30-chromebook">Toshiba Chromebook</a></td>
    <td>Leon</td>
    <td>leon</td>
    <td>slippy</td>
    <td>x86_64</td>
    <td>3.8</td>
    <td>x86_64</td>
    <td>Haswell</td>
    <td>Chromebook</td>
    <td>R31</td>
    <td>R76</td>
    <td></td>
    <td>No</td>
  </tr>
  <tr>
    <td>2014-03-13</td>
    <td>ASUS</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/asus-chromebox">ASUS Chromebox CN60</a></td>
    <td>Panther</td>
    <td>panther</td>
    <td>beltino</td>
    <td>x86_64</td>
    <td>3.8</td>
    <td>x86_64</td>
    <td>Haswell</td>
    <td>Chromebox</td>
    <td>R32</td>
    <td>R76</td>
    <td></td>
    <td>No</td>
  </tr>
  <tr>
    <td>2014-05-04</td>
    <td>Samsung</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/samsung-chromebook-2">Samsung Chromebook 2 11"</a></td>
    <td>Pit</td>
    <td>peach_pit</td>
    <td>peach</td>
    <td>arm</td>
    <td>3.8</td>
    <td>arm</td>
    <td>Exynos 5450</td>
    <td>Chromebook</td>
    <td>R30</td>
    <td>R75</td>
    <td></td>
    <td>No</td>
  </tr>
  <tr>
    <td>2014-05-18</td>
    <td>LG</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/lg-chromebase-22cv241-w">LG Chromebase 22CV241 &amp; 22CB25S</a></td>
    <td>Monroe</td>
    <td>monroe</td>
    <td>beltino</td>
    <td>x86_64</td>
    <td>3.8</td>
    <td>x86_64</td>
    <td>Haswell</td>
    <td>Chromebase</td>
    <td>R33</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>No</td>
  </tr>
  <tr>
    <td>2014-05-18</td>
    <td>Samsung</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/samsung-chromebook-2">Samsung Chromebook 2 13"</a></td>
    <td>Pi</td>
    <td>peach_pi</td>
    <td>peach</td>
    <td>arm</td>
    <td>3.8</td>
    <td>arm</td>
    <td>Exynos 5450</td>
    <td>Chromebook</td>
    <td>R34</td>
    <td>R75</td>
    <td></td>
    <td>No</td>
  </tr>
  <tr>
    <td>2014-05-25</td>
    <td>HP</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/hp-chromebook-11">HP Chromebook 11 G2</a></td>
    <td>Skate</td>
    <td>daisy_skate</td>
    <td>snow</td>
    <td>arm</td>
    <td>3.8</td>
    <td>arm</td>
    <td>Exynos 5250</td>
    <td>Chromebook</td>
    <td>R34</td>
    <td>R75</td>
    <td></td>
    <td>No</td>
  </tr>
  <tr>
    <td>2014-05-29</td>
    <td>Lenovo</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/lenovo-chromebook-n20">Lenovo N20 Chromebook</a></td>
    <td>Clapper</td>
    <td>clapper</td>
    <td>rambi</td>
    <td>x86_64</td>
    <td>4.4</td>
    <td>x86_64</td>
    <td>BayTrail</td>
    <td>Convertible</td>
    <td>R34</td>
    <td>R76</td>
    <td></td>
    <td>No</td>
  </tr>
  <tr>
    <td>2014-05-31</td>
    <td>ASUS</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">ASUS Chromebook C200</a></td>
    <td>Squawks</td>
    <td>squawks</td>
    <td>rambi</td>
    <td>x86_64</td>
    <td>4.4</td>
    <td>x86_64</td>
    <td>BayTrail</td>
    <td>Chromebook</td>
    <td>R34</td>
    <td>R76</td>
    <td></td>
    <td>No</td>
  </tr>
  <tr>
    <td>2014-06-05</td>
    <td>Lenovo</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">Lenovo ThinkPad 11e Chromebook</a></td>
    <td>Glimmer</td>
    <td>glimmer</td>
    <td>rambi</td>
    <td>x86_64</td>
    <td>4.4</td>
    <td>x86_64</td>
    <td>BayTrail</td>
    <td>Convertible</td>
    <td>R34</td>
    <td>R76</td>
    <td></td>
    <td>No</td>
  </tr>
  <tr>
    <td>2014-06-14</td>
    <td><white label></td>
    <td><a href=""></a></td>
    <td>Enguarde</td>
    <td>enguarde</td>
    <td>rambi</td>
    <td>x86_64</td>
    <td>4.4</td>
    <td>x86_64</td>
    <td>BayTrail</td>
    <td>Chromebook</td>
    <td>R34</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>No</td>
  </tr>
  <tr>
    <td>2014-06-14</td>
    <td><white label></td>
    <td><a href=""></a></td>
    <td>Expresso</td>
    <td>expresso</td>
    <td>rambi</td>
    <td>x86_64</td>
    <td>4.4</td>
    <td>x86_64</td>
    <td>BayTrail</td>
    <td>Chromebook</td>
    <td>R34</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>No</td>
  </tr>
  <tr>
    <td>2014-06-21</td>
    <td>HP</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/hp-chromebox">HP Chromebox G1</a></td>
    <td>Zako</td>
    <td>zako</td>
    <td>beltino</td>
    <td>x86_64</td>
    <td>3.8</td>
    <td>x86_64</td>
    <td>Haswell</td>
    <td>Chromebox</td>
    <td>R34</td>
    <td>R76</td>
    <td></td>
    <td>No</td>
  </tr>
  <tr>
    <td>2014-08-20</td>
    <td>Acer</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">Acer Chromebox</a></td>
    <td>McCloud</td>
    <td>mccloud</td>
    <td>beltino</td>
    <td>x86_64</td>
    <td>3.8</td>
    <td>x86_64</td>
    <td>Haswell</td>
    <td>Chromebox</td>
    <td>R36</td>
    <td>R76</td>
    <td></td>
    <td>No</td>
  </tr>
  <tr>
    <td>2014-08-30</td>
    <td>ASUS</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">ASUS Chromebook C300</a></td>
    <td>Quawks</td>
    <td>quawks</td>
    <td>rambi</td>
    <td>x86_64</td>
    <td>4.4</td>
    <td>x86_64</td>
    <td>BayTrail</td>
    <td>Chromebook</td>
    <td>R34</td>
    <td>R76</td>
    <td></td>
    <td>No</td>
  </tr>
  <tr>
    <td>2014-08-31</td>
    <td>Acer</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">Acer Chromebook 11 (C730 / C730E / C735)</a></td>
    <td>Gnawty</td>
    <td>gnawty</td>
    <td>rambi</td>
    <td>x86_64</td>
    <td>4.4</td>
    <td>x86_64</td>
    <td>BayTrail</td>
    <td>Chromebook</td>
    <td>R34</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>No</td>
  </tr>
  <tr>
    <td>2014-09-01</td>
    <td>HP</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">HP Chromebook 11 G3 / G4 / G4 EE</a></td>
    <td>Kip</td>
    <td>kip</td>
    <td>rambi</td>
    <td>x86_64</td>
    <td>4.4</td>
    <td>x86_64</td>
    <td>BayTrail</td>
    <td>Chromebook</td>
    <td>R34</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>No</td>
  </tr>
  <tr>
    <td>2014-09-02</td>
    <td>Toshiba</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">Toshiba Chromebook 2</a></td>
    <td>Swanky</td>
    <td>swanky</td>
    <td>rambi</td>
    <td>x86_64</td>
    <td>4.4</td>
    <td>x86_64</td>
    <td>BayTrail</td>
    <td>Chromebook</td>
    <td>R36</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>No</td>
  </tr>
  <tr>
    <td>2014-09-07</td>
    <td>Acer</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/acer-cb5-311-chromebook-13">Acer Chromebook 13 (CB5-311)</a></td>
    <td>Big</td>
    <td>nyan_big</td>
    <td>nyan</td>
    <td>arm</td>
    <td>3.10</td>
    <td>arm</td>
    <td>Tegra K1 T124</td>
    <td>Chromebook</td>
    <td>R35</td>
    <td>R77</td>
    <td></td>
    <td>No</td>
  </tr>
  <tr>
    <td>2014-09-13</td>
    <td>Dell</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">Dell Chromebox</a></td>
    <td>Tricky</td>
    <td>tricky</td>
    <td>beltino</td>
    <td>x86_64</td>
    <td>3.8</td>
    <td>x86_64</td>
    <td>Haswell</td>
    <td>Chromebox</td>
    <td>R36</td>
    <td>R76</td>
    <td></td>
    <td>No</td>
  </tr>
  <tr>
    <td>2014-10-13</td>
    <td>Samsung</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">Samsung Chromebook 2 11 - XE500C12</a></td>
    <td>Winky</td>
    <td>winky</td>
    <td>rambi</td>
    <td>x86_64</td>
    <td>4.4</td>
    <td>x86_64</td>
    <td>BayTrail</td>
    <td>Chromebook</td>
    <td>R36</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>No</td>
  </tr>
  <tr>
    <td>2014-10-18</td>
    <td>HP</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">HP Chromebook 14 G3</a></td>
    <td>Blaze</td>
    <td>nyan_blaze</td>
    <td>nyan</td>
    <td>arm</td>
    <td>3.10</td>
    <td>arm</td>
    <td>Tegra K1 T124</td>
    <td>Chromebook</td>
    <td>R36</td>
    <td>R77</td>
    <td></td>
    <td>No</td>
  </tr>
  <tr>
    <td>2015-02-28</td>
    <td>Acer</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/acer-c670-chromebook">Acer C670 Chromebook 11</a></td>
    <td>Paine</td>
    <td>auron_paine</td>
    <td>auron</td>
    <td>x86_64</td>
    <td>3.14</td>
    <td>x86_64</td>
    <td>Broadwell</td>
    <td>Chromebook</td>
    <td>R40</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2015-02-28</td>
    <td>Dell</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">Dell Chromebook 11 (3120)</a></td>
    <td>Candy</td>
    <td>candy</td>
    <td>rambi</td>
    <td>x86_64</td>
    <td>4.4</td>
    <td>x86_64</td>
    <td>BayTrail</td>
    <td>Chromebook</td>
    <td>R39</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>No</td>
  </tr>
  <tr>
    <td>2015-03-11</td>
    <td>Google</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/chromebook-pixel-2015">Google Chromebook Pixel (2015)</a></td>
    <td>Samus</td>
    <td>samus</td>
    <td></td>
    <td>x86_64</td>
    <td>4.14</td>
    <td>x86_64</td>
    <td>Broadwell</td>
    <td>Chromebook</td>
    <td>R39</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2015-04-23</td>
    <td>Acer</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">Acer Chromebook 15.6" (CB3-531)</a></td>
    <td>Banjo</td>
    <td>banjo</td>
    <td>rambi</td>
    <td>x86_64</td>
    <td>4.4</td>
    <td>x86_64</td>
    <td>BayTrail</td>
    <td>Chromebook</td>
    <td>R42</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>No</td>
  </tr>
  <tr>
    <td>2015-04-30</td>
    <td><white label></td>
    <td><a href=""></a></td>
    <td>Jerry</td>
    <td>veyron_jerry</td>
    <td>veyron_pinky</td>
    <td>arm</td>
    <td>4.19</td>
    <td>arm</td>
    <td>RK3288</td>
    <td>Chromebook</td>
    <td>R41</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2015-04-30</td>
    <td><white label></td>
    <td><a href=""></a></td>
    <td>Mighty</td>
    <td>veyron_mighty</td>
    <td>veyron_pinky</td>
    <td>arm</td>
    <td>4.19</td>
    <td>arm</td>
    <td>RK3288</td>
    <td>Chromebook</td>
    <td>R41</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2015-04-30</td>
    <td>Acer</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/acer-c910-chromebook">Acer Chromebook 15</a></td>
    <td>Yuna</td>
    <td>auron_yuna</td>
    <td>auron</td>
    <td>x86_64</td>
    <td>3.14</td>
    <td>x86_64</td>
    <td>Broadwell</td>
    <td>Chromebook</td>
    <td>R40</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2015-04-30</td>
    <td><white label></td>
    <td><a href=""></a></td>
    <td>Jaq</td>
    <td>veyron_jaq</td>
    <td>veyron_pinky</td>
    <td>arm</td>
    <td>4.19</td>
    <td>arm</td>
    <td>RK3288</td>
    <td>Chromebook</td>
    <td>R41</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2015-05-01</td>
    <td>Acer</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">Acer Chromebox CXI2</a></td>
    <td>Rikku</td>
    <td>rikku</td>
    <td>jecht</td>
    <td>x86_64</td>
    <td>3.14</td>
    <td>x86_64</td>
    <td>Broadwell</td>
    <td>Chromebox</td>
    <td>R42</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2015-05-01</td>
    <td>ASUS</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">ASUS Chromebook C201</a></td>
    <td>Speedy</td>
    <td>veyron_speedy</td>
    <td>veyron_pinky</td>
    <td>arm</td>
    <td>4.19</td>
    <td>arm</td>
    <td>RK3288</td>
    <td>Chromebook</td>
    <td>R41</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2015-06-02</td>
    <td>Lenovo</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">Lenovo ThinkCentre Chromebox</a></td>
    <td>Tidus</td>
    <td>tidus</td>
    <td>jecht</td>
    <td>x86_64</td>
    <td>3.14</td>
    <td>x86_64</td>
    <td>Broadwell</td>
    <td>Chromebox</td>
    <td>R42</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2015-07-01</td>
    <td>ASUS</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">ASUS Chromebook Flip C100PA</a></td>
    <td>Minnie</td>
    <td>veyron_minnie</td>
    <td>veyron_pinky</td>
    <td>arm</td>
    <td>4.19</td>
    <td>arm</td>
    <td>RK3288</td>
    <td>Convertible</td>
    <td>R42</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2015-08-01</td>
    <td>Acer</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">Acer Chromebase</a></td>
    <td>Kitty</td>
    <td>nyan_kitty</td>
    <td>nyan</td>
    <td>arm</td>
    <td>3.10</td>
    <td>arm</td>
    <td>Tegra K1 T124</td>
    <td>Chromebase</td>
    <td>R40</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>No</td>
  </tr>
  <tr>
    <td>2015-08-03</td>
    <td>ASUS</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">ASUS Chromebox CN62</a></td>
    <td>Guado</td>
    <td>guado</td>
    <td>jecht</td>
    <td>x86_64</td>
    <td>3.14</td>
    <td>x86_64</td>
    <td>Broadwell</td>
    <td>Chromebox</td>
    <td>R41</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2015-08-13</td>
    <td>Dell</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">Dell Chromebook 13 7310</a></td>
    <td>Lulu</td>
    <td>lulu</td>
    <td>auron</td>
    <td>x86_64</td>
    <td>3.14</td>
    <td>x86_64</td>
    <td>Broadwell</td>
    <td>Chromebook</td>
    <td>R43</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2015-09-01</td>
    <td>AOpen</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">AOpen Chromebox Commercial</a></td>
    <td>Ninja</td>
    <td>ninja</td>
    <td>rambi</td>
    <td>x86_64</td>
    <td>4.4</td>
    <td>x86_64</td>
    <td>BayTrail</td>
    <td>Chromebox</td>
    <td>R43</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>No</td>
  </tr>
  <tr>
    <td>2015-09-02</td>
    <td>AOpen</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">AOpen Chromebase Commercial</a></td>
    <td>Sumo</td>
    <td>sumo</td>
    <td>rambi</td>
    <td>x86_64</td>
    <td>4.4</td>
    <td>x86_64</td>
    <td>BayTrail</td>
    <td>Chromebase</td>
    <td>R43</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>No</td>
  </tr>
  <tr>
    <td>2015-09-04</td>
    <td>Lenovo</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">Lenovo 100S Chromebook</a></td>
    <td>Orco</td>
    <td>orco</td>
    <td>rambi</td>
    <td>x86_64</td>
    <td>4.4</td>
    <td>x86_64</td>
    <td>BayTrail</td>
    <td>Chromebook</td>
    <td>R44</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>No</td>
  </tr>
  <tr>
    <td>2015-09-22</td>
    <td>Toshiba</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">Toshiba Chromebook 2 (2015 Edition)</a></td>
    <td>Gandof</td>
    <td>gandof</td>
    <td>auron</td>
    <td>x86_64</td>
    <td>3.14</td>
    <td>x86_64</td>
    <td>Broadwell</td>
    <td>Chromebook</td>
    <td>R44</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2015-11-02</td>
    <td>ASUS</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">Asus Chromebit CS10</a></td>
    <td>Mickey</td>
    <td>veyron_mickey</td>
    <td>veyron_pinky</td>
    <td>arm</td>
    <td>4.19</td>
    <td>arm</td>
    <td>RK3288</td>
    <td>Chromebit</td>
    <td>R45</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2015-11-26</td>
    <td><white label></td>
    <td><a href=""></a></td>
    <td>Heli</td>
    <td>heli</td>
    <td>rambi</td>
    <td>x86_64</td>
    <td>4.4</td>
    <td>x86_64</td>
    <td>BayTrail</td>
    <td>Chromebook</td>
    <td>R45</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>No</td>
  </tr>
  <tr>
    <td>2015-12-01</td>
    <td>Acer</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">Acer Chromebook R11</a></td>
    <td>Cyan</td>
    <td>cyan</td>
    <td>strago</td>
    <td>x86_64</td>
    <td>3.18</td>
    <td>x86_64</td>
    <td>Braswell</td>
    <td>Chromebook</td>
    <td>R44</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2015-12-22</td>
    <td>Samsung</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">Samsung Chromebook 3</a></td>
    <td>Celes</td>
    <td>celes</td>
    <td>strago</td>
    <td>x86_64</td>
    <td>3.18</td>
    <td>x86_64</td>
    <td>Braswell</td>
    <td>Chromebook</td>
    <td>R46</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2016-02-29</td>
    <td>ASUS</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">ASUS Chromebook C202SA/C300SA</a></td>
    <td>Terra</td>
    <td>terra</td>
    <td>strago</td>
    <td>x86_64</td>
    <td>3.18</td>
    <td>x86_64</td>
    <td>Braswell</td>
    <td>Chromebook</td>
    <td>R48</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2016-03-07</td>
    <td>Lenovo</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">ThinkPad 11e Chromebook 3rd</a></td>
    <td>Ultima</td>
    <td>ultima</td>
    <td>strago</td>
    <td>x86_64</td>
    <td>3.18</td>
    <td>x86_64</td>
    <td>Braswell</td>
    <td>Chromebook</td>
    <td>R48</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2016-03-29</td>
    <td>Acer</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">Chromebook 14 (CB3-431)</a></td>
    <td>Edgar</td>
    <td>edgar</td>
    <td>strago</td>
    <td>x86_64</td>
    <td>3.18</td>
    <td>x86_64</td>
    <td>Braswell</td>
    <td>Chromebook</td>
    <td>R49</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2016-04-01</td>
    <td>Acer</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">Acer Chromebase 24</a></td>
    <td>Buddy</td>
    <td>buddy</td>
    <td>auron</td>
    <td>x86_64</td>
    <td>3.14</td>
    <td>x86_64</td>
    <td>Broadwell</td>
    <td>Chromebase</td>
    <td>R48</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2016-05-05</td>
    <td>HP</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">HP Chromebook 13 G1</a></td>
    <td>Chell</td>
    <td>chell</td>
    <td>glados</td>
    <td>x86_64</td>
    <td>3.18</td>
    <td>x86_64</td>
    <td>Skylake</td>
    <td>Chromebook</td>
    <td>R49</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2016-05-27</td>
    <td>Lenovo</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">Thinkpad 13 Chromebook</a></td>
    <td>Sentry</td>
    <td>sentry</td>
    <td>kunimitsu</td>
    <td>x86_64</td>
    <td>3.18</td>
    <td>x86_64</td>
    <td>Skylake</td>
    <td>Chromebook</td>
    <td>R50</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2016-05-31</td>
    <td>Acer</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">Chromebook 14 for work (CP5-471)</a></td>
    <td>Lars</td>
    <td>lars</td>
    <td>kunimitsu</td>
    <td>x86_64</td>
    <td>3.18</td>
    <td>x86_64</td>
    <td>Skylake</td>
    <td>Chromebook</td>
    <td>R49</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2016-07-08</td>
    <td>HP</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">HP Chromebook 11 G5</a></td>
    <td>Setzer</td>
    <td>setzer</td>
    <td>strago</td>
    <td>x86_64</td>
    <td>3.18</td>
    <td>x86_64</td>
    <td>Braswell</td>
    <td>Chromebook</td>
    <td>R51</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2016-08-01</td>
    <td>Lenovo</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/lenovo-chromebook-11">Lenovo Chromebook 11</a></td>
    <td>Reks</td>
    <td>reks</td>
    <td>strago</td>
    <td>x86_64</td>
    <td>3.18</td>
    <td>x86_64</td>
    <td>Braswell</td>
    <td>Chromebook</td>
    <td>R48</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2016-08-05</td>
    <td><white label></td>
    <td><a href=""></a></td>
    <td>Relm</td>
    <td>relm</td>
    <td>strago</td>
    <td>x86_64</td>
    <td>3.18</td>
    <td>x86_64</td>
    <td>Braswell</td>
    <td>Chromebook</td>
    <td>R52</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2016-08-08</td>
    <td><white label></td>
    <td><a href=""></a></td>
    <td>Wizpig</td>
    <td>wizpig</td>
    <td>strago</td>
    <td>x86_64</td>
    <td>3.18</td>
    <td>x86_64</td>
    <td>Braswell</td>
    <td>Convertible</td>
    <td>R50</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2016-08-12</td>
    <td>Acer</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">Acer 15.6" Chromebook</a></td>
    <td>Banon</td>
    <td>banon</td>
    <td>strago</td>
    <td>x86_64</td>
    <td>3.18</td>
    <td>x86_64</td>
    <td>Braswell</td>
    <td>Chromebook</td>
    <td>R51</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2016-09-06</td>
    <td>Acer</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">Acer Chromebook R13</a></td>
    <td>Elm</td>
    <td>elm</td>
    <td>oak</td>
    <td>arm</td>
    <td>3.18</td>
    <td>aarch64</td>
    <td>MT8173</td>
    <td>Convertible</td>
    <td>R52</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2017-01-05</td>
    <td>ASUS</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">ASUS Chromebook Flip C302</a></td>
    <td>Cave</td>
    <td>cave</td>
    <td>glados</td>
    <td>x86_64</td>
    <td>3.18</td>
    <td>x86_64</td>
    <td>Skylake</td>
    <td>Convertible</td>
    <td>R53</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2017-02-07</td>
    <td>Dell</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">Dell Chromebook 13 3380</a></td>
    <td>Asuka</td>
    <td>asuka</td>
    <td>kunimitsu</td>
    <td>x86_64</td>
    <td>3.18</td>
    <td>x86_64</td>
    <td>Skylake</td>
    <td>Chromebook</td>
    <td>R55</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2017-02-07</td>
    <td>Dell</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">Chromebook 11 Model 3180</a></td>
    <td>Kefka</td>
    <td>kefka</td>
    <td>strago</td>
    <td>x86_64</td>
    <td>3.18</td>
    <td>x86_64</td>
    <td>Braswell</td>
    <td>Chromebook</td>
    <td>R54</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2017-02-12</td>
    <td>Samsung</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">Samsung Chromebook Plus</a></td>
    <td>Kevin</td>
    <td>kevin</td>
    <td>gru</td>
    <td>arm</td>
    <td>4.4</td>
    <td>aarch64</td>
    <td>RK3399</td>
    <td>Convertible</td>
    <td>R53</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2017-02-28</td>
    <td>AOpen</td>
    <td><a href="http://www.aopen.com/us/chrome-mini-products">AOpen Chromebox Mini</a></td>
    <td>fievel</td>
    <td>veyron_fievel</td>
    <td>veyron_pinky</td>
    <td>arm</td>
    <td>4.19</td>
    <td>arm</td>
    <td>RK3288</td>
    <td>Chromebox</td>
    <td>R54</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2017-02-28</td>
    <td>AOpen</td>
    <td><a href="http://www.aopen.com/us/chrome-mini-products">AOpen Chromebase Mini</a></td>
    <td>tiger</td>
    <td>veyron_tiger</td>
    <td>veyron_pinky</td>
    <td>arm</td>
    <td>4.19</td>
    <td>arm</td>
    <td>RK3288</td>
    <td>Chromebase</td>
    <td>R54</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2017-03-17</td>
    <td>Lenovo</td>
    <td><a href="http://shop.lenovo.com/us/en/laptops/lenovo/n-series/n23-yoga-chromebook/">Lenovo N23 Yoga Chromebook</a></td>
    <td>hana</td>
    <td>hana</td>
    <td>oak</td>
    <td>arm</td>
    <td>3.18</td>
    <td>aarch64</td>
    <td>MT8173</td>
    <td>Convertible</td>
    <td>R56</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2017-04-24</td>
    <td>Lenovo</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">Lenovo Thinkpad 11e Chromebook / Lenovo Thinkpad Yoga 11e Chromebook</a></td>
    <td>Pyro</td>
    <td>pyro</td>
    <td>reef</td>
    <td>x86_64</td>
    <td>4.4</td>
    <td>x86_64</td>
    <td>ApolloLake</td>
    <td>Chromebook</td>
    <td>R56</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2017-04-24</td>
    <td>HP</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">HP Chromebook x360 11 G1 EE</a></td>
    <td>Snappy</td>
    <td>snappy</td>
    <td>reef</td>
    <td>x86_64</td>
    <td>4.4</td>
    <td>x86_64</td>
    <td>ApolloLake</td>
    <td>Convertible</td>
    <td>R56</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2017-05-25</td>
    <td>Samsung</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">Samsung Chromebook Pro</a></td>
    <td>Caroline</td>
    <td>caroline</td>
    <td>glados</td>
    <td>x86_64</td>
    <td>3.18</td>
    <td>x86_64</td>
    <td>Skylake</td>
    <td>Convertible</td>
    <td>R56</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2017-06-30</td>
    <td>Acer</td>
    <td><a href="http://www.chromium.org/chromium-os/developer-information-for-chrome-os-devices/generic">Acer Chromebook  Spin 11 R751T</a></td>
    <td>Electro</td>
    <td>reef</td>
    <td>reef</td>
    <td>x86_64</td>
    <td>4.4</td>
    <td>x86_64</td>
    <td>ApolloLake</td>
    <td>Chromebook</td>
    <td>R53</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2017-08-10</td>
    <td>Poin2</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">Poin2 Chromebook 14</a></td>
    <td>Birch</td>
    <td>hana</td>
    <td>oak</td>
    <td>arm</td>
    <td>3.18</td>
    <td>aarch64</td>
    <td>MT8173</td>
    <td>Chromebook</td>
    <td>R56</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2017-09-01</td>
    <td>ASUS</td>
    <td><a href="https://www.asus.com/Laptops/ASUS-Chromebook-Flip-C101PA/">Asus Chromebook Flip C101PA</a></td>
    <td>Bob</td>
    <td>Bob</td>
    <td>gru</td>
    <td>arm</td>
    <td>4.4</td>
    <td>aarch64</td>
    <td>RK3399</td>
    <td>Convertible</td>
    <td>R58</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Hinge Port</td>
  </tr>
  <tr>
    <td>2017-09-08</td>
    <td>Acer</td>
    <td><a href="http://www.chromium.org/chromium-os/developer-information-for-chrome-os-devices/generic">Chromebook 15 CB515-1HT/1H</a></td>
    <td>Sand</td>
    <td>sand</td>
    <td>reef</td>
    <td>x86_64</td>
    <td>4.4</td>
    <td>x86_64</td>
    <td>ApolloLake</td>
    <td>Chromebook</td>
    <td>R59</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2017-10-31</td>
    <td>Google</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">Pixelbook</a></td>
    <td>Eve</td>
    <td>eve</td>
    <td>eve</td>
    <td>x86_64</td>
    <td>4.4</td>
    <td>x86_64</td>
    <td>Kabylake-Y</td>
    <td>Convertible</td>
    <td>R61</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td>Left Port</td>
    <td>Left Port</td>
  </tr>
  <tr>
    <td>2017-11-15</td>
    <td>Poin2</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">Poin2 Chromebook 11C</a></td>
    <td>Hanawl</td>
    <td>hana</td>
    <td>oak</td>
    <td>arm</td>
    <td>3.18</td>
    <td>aarch64</td>
    <td>MT8173</td>
    <td>Chromebook</td>
    <td>R56</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2018-02-14</td>
    <td>Acer</td>
    <td><a href="https://www.acer.com/ac/en/US/press/2018/333129">Acer Chromebook 11 (C732, C732T, C732L &amp; C732LT )</a></td>
    <td>Astronaut</td>
    <td>coral</td>
    <td>coral</td>
    <td>x86_64</td>
    <td>4.4</td>
    <td>x86_64</td>
    <td>ApolloLake</td>
    <td>Chromebook</td>
    <td>R64</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2018-03-01</td>
    <td>Lenovo</td>
    <td><a href="https://www.lenovo.com/us/en/laptops/lenovo/lenovo-n-series/300e-Chromebook/p/88ELC1S9997">Lenovo 300e Chromebook</a></td>
    <td>hana</td>
    <td>hana</td>
    <td>oak</td>
    <td>arm</td>
    <td>3.18</td>
    <td>aarch64</td>
    <td>MT8173</td>
    <td>Convertible</td>
    <td>R64</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2018-03-01</td>
    <td>Lenovo</td>
    <td><a href="https://www.lenovo.com/us/en/laptops/lenovo/lenovo-n-series/500e-Chromebook/p/88ELC1S9998">Lenovo 500e Chromebook</a></td>
    <td>robo360</td>
    <td>coral</td>
    <td>coral</td>
    <td>x86_64</td>
    <td>4.4</td>
    <td>x86_64</td>
    <td>ApolloLake</td>
    <td>Convertible</td>
    <td>R64</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2018-03-01</td>
    <td>Lenovo</td>
    <td><a href="https://www.lenovo.com/us/en/laptops/lenovo/lenovo-n-series/100e-Chromebook/p/88ELC1S9999">Lenovo 100e Chromebook</a></td>
    <td>robo</td>
    <td>coral</td>
    <td>coral</td>
    <td>x86_64</td>
    <td>4.4</td>
    <td>x86_64</td>
    <td>ApolloLake</td>
    <td>Chromebook</td>
    <td>R64</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2018-03-05</td>
    <td>Acer</td>
    <td><a href="https://www.acer.com/ac/en/US/press/2018/324876">AcerChromebook 11 (CB311-8H &amp; CB311-8HT)</a></td>
    <td>Santa</td>
    <td>coral</td>
    <td>coral</td>
    <td>x86_64</td>
    <td>4.4</td>
    <td>x86_64</td>
    <td>ApolloLake</td>
    <td>Chromebook</td>
    <td>R64</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2018-03-16</td>
    <td>Acer</td>
    <td><a href="https://www.acer.com/ac/en/AU/press/2018/333192">Acer Chromebook Spin 11 (CP311-1H &amp; CP311-1HN)</a></td>
    <td>Lava</td>
    <td>coral</td>
    <td>coral</td>
    <td>x86_64</td>
    <td>4.4</td>
    <td>x86_64</td>
    <td>ApolloLake</td>
    <td>Convertible</td>
    <td>R64</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2018-05-18</td>
    <td>HP</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">HP Chromebox G2</a></td>
    <td>Kench</td>
    <td>fizz</td>
    <td>fizz</td>
    <td>x86_64</td>
    <td>4.4</td>
    <td>x86_64</td>
    <td>Kabylake-U/R</td>
    <td>Chromebox</td>
    <td></td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2018-06-01</td>
    <td>Acer</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">Acer Chromebox CXI3</a></td>
    <td>Sion</td>
    <td>fizz</td>
    <td>fizz</td>
    <td>x86_64</td>
    <td>4.4</td>
    <td>x86_64</td>
    <td>Kabylake-U/R</td>
    <td>Chromebox</td>
    <td></td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2018-06-01</td>
    <td>HP</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">HP Chromebook x2</a></td>
    <td>Soraka</td>
    <td>soraka</td>
    <td>poppy</td>
    <td>x86_64</td>
    <td>4.4</td>
    <td>x86_64</td>
    <td>Kabylake-Y</td>
    <td>Chromeblet</td>
    <td></td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td>Left Port</td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2018-06-01</td>
    <td>ASUS</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">ASUS Chromebox 3</a></td>
    <td>Teemo</td>
    <td>fizz</td>
    <td>fizz</td>
    <td>x86_64</td>
    <td>4.4</td>
    <td>x86_64</td>
    <td>Kabylake-U/R</td>
    <td>Chromebox</td>
    <td></td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2018-06-01</td>
    <td>CTL</td>
    <td><a href="https://ctl.net/collections/chromebox-1/products/ctl-chromebox">CTL Chromebox CBx1</a></td>
    <td>Wukong</td>
    <td>fizz</td>
    <td>fizz</td>
    <td>x86_64</td>
    <td>4.4</td>
    <td>x86_64</td>
    <td>Kabylake-U/R</td>
    <td>Chromebox</td>
    <td>R67</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2018-06-01</td>
    <td>Viewsonic</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">ViewSonic NMP660 Chromebox</a></td>
    <td>Wukong</td>
    <td>fizz</td>
    <td>fizz</td>
    <td>x86_64</td>
    <td>4.4</td>
    <td>x86_64</td>
    <td>Kabylake-U/R</td>
    <td>Chromebox</td>
    <td></td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2018-06-14</td>
    <td>Samsung</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">Samsung Chromebook Plus (V2)</a></td>
    <td>Nautilus</td>
    <td>nautilus</td>
    <td>poppy</td>
    <td>x86_64</td>
    <td>4.4</td>
    <td>x86_64</td>
    <td>Kabylake-Y</td>
    <td>Convertible</td>
    <td>R67</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2018-06-15</td>
    <td>CTL</td>
    <td><a href="https://ctl.net/products/ctl-chromebook-j41">CTL Chromebook J41</a></td>
    <td>Whitetip</td>
    <td>coral</td>
    <td>coral</td>
    <td>x86_64</td>
    <td>4.4</td>
    <td>x86_64</td>
    <td>ApolloLake</td>
    <td>Chromebook</td>
    <td>R67</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2018-06-30</td>
    <td><white label></td>
    <td><a href=""></a></td>
    <td>blacktip</td>
    <td>coral</td>
    <td>coral</td>
    <td>x86_64</td>
    <td>4.4</td>
    <td>x86_64</td>
    <td>ApolloLake</td>
    <td>Chromebook</td>
    <td>R67</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2018-06-30</td>
    <td>CTL</td>
    <td><a href="https://ctl.net/products/ctl-nl7t-chromebook-for-education-1">CTL Chromebook NL7T-360</a></td>
    <td>Blacktip360</td>
    <td>coral</td>
    <td>coral</td>
    <td>x86_64</td>
    <td>4.4</td>
    <td>x86_64</td>
    <td>ApolloLake</td>
    <td>Convertible</td>
    <td>R67</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2018-06-30</td>
    <td>CTL</td>
    <td><a href="https://ctl.net/products/ctl-chromebook-nl7-for-education">CTL Chromebook NL7</a></td>
    <td>Blacktip</td>
    <td>coral</td>
    <td>coral</td>
    <td>x86_64</td>
    <td>4.4</td>
    <td>x86_64</td>
    <td>ApolloLake</td>
    <td>Chromebook</td>
    <td>R67</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2018-08-08</td>
    <td>ASUS</td>
    <td><a href="https://www.asus.com/Laptops/ASUS-Chromebook-C223NA/">ASUS Chromebook C223</a></td>
    <td>babymega</td>
    <td>coral</td>
    <td>coral</td>
    <td>x86_64</td>
    <td>4.4</td>
    <td>x86_64</td>
    <td>ApolloLake</td>
    <td>Chromebook</td>
    <td>R67</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>yes</td>
  </tr>
  <tr>
    <td>2018-08-31</td>
    <td>Lenovo</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">Lenovo Chromebook C330</a></td>
    <td>maple</td>
    <td>hana</td>
    <td>oak</td>
    <td>arm</td>
    <td>3.18</td>
    <td>aarch64</td>
    <td>MT8173</td>
    <td>Convertible</td>
    <td>R67</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2018-09-14</td>
    <td>ASUS</td>
    <td><a href="https://www.asus.com/us/Laptops/ASUS-Chromebook-C423NA/">ASUS Chromebook C423</a></td>
    <td>rabbid</td>
    <td>coral</td>
    <td>coral</td>
    <td>x86_64</td>
    <td>4.4</td>
    <td>x86_64</td>
    <td>ApolloLake</td>
    <td>Chromebook</td>
    <td>R68</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>yes</td>
  </tr>
  <tr>
    <td>2018-09-17</td>
    <td>Lenovo</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">Lenovo Chromebook S330</a></td>
    <td>maple14</td>
    <td>hana</td>
    <td>oak</td>
    <td>arm</td>
    <td>3.18</td>
    <td>aarch64</td>
    <td>MT8173</td>
    <td>Chromebook</td>
    <td>M67</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2018-10-08</td>
    <td>ASUS</td>
    <td><a href="https://www.asus.com/us/Laptops/ASUS-Chromebook-C523NA/">ASUS Chromebook C523</a></td>
    <td>babytiger</td>
    <td>coral</td>
    <td>coral</td>
    <td>x86_64</td>
    <td>4.4</td>
    <td>x86_64</td>
    <td>ApolloLake</td>
    <td>Chromebook</td>
    <td>R69</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>yes</td>
  </tr>
  <tr>
    <td>2018-10-15</td>
    <td>Acer</td>
    <td><a href="">Acer Chromebook 514</a></td>
    <td>Epaulette</td>
    <td>coral</td>
    <td>coral</td>
    <td>x86_64</td>
    <td>4.4</td>
    <td>x86_64</td>
    <td>ApolloLake</td>
    <td>Chromebook</td>
    <td>R69</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2018-10-26</td>
    <td>Acer</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">Acer Chromebook 13 / Spin 13</a></td>
    <td>Akali / Akali360</td>
    <td>nami</td>
    <td>nami</td>
    <td>x86_64</td>
    <td>4.4</td>
    <td>x86_64</td>
    <td>Kabylake-U/R</td>
    <td>Convertible</td>
    <td>R69</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2018-10-26</td>
    <td>Dell</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">Dell Inspiron 14 2-in-1 Model 7486</a></td>
    <td>vayne</td>
    <td>vayne</td>
    <td>nami</td>
    <td>x86_64</td>
    <td>4.4</td>
    <td>x86_64</td>
    <td>Kabylake-U/R</td>
    <td>Chromebook</td>
    <td>R69</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2018-10-26</td>
    <td>Lenovo</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">Yoga Chromebook C630</a></td>
    <td>pantheon</td>
    <td>pantheon</td>
    <td>nami</td>
    <td>x86_64</td>
    <td>4.4</td>
    <td>x86_64</td>
    <td>Kabylake-U/R</td>
    <td>Convertible</td>
    <td>R69</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2018-11-02</td>
    <td>Samsung</td>
    <td><a href="https://www.samsung.com/us/computing/chromebooks/12-14/samsung-chromebook-plus-lte-xe525qbb-k01us/">Samsung Chromebook Plus LTE</a></td>
    <td>Nautilus LTE</td>
    <td>nautilus</td>
    <td>poppy</td>
    <td>x86_64</td>
    <td>4.4</td>
    <td>x86_64</td>
    <td>Kabylake-Y</td>
    <td>Convertible</td>
    <td>R69</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>yes</td>
  </tr>
  <tr>
    <td>2018-11-26</td>
    <td>Google</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">Google Pixel Slate</a></td>
    <td>Nocturne</td>
    <td>nocturne</td>
    <td>poppy</td>
    <td>x86_64</td>
    <td>4.4</td>
    <td>x86_64</td>
    <td>Kabylake-Y</td>
    <td>Chromeblet</td>
    <td>R71</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td>Left Port</td>
    <td>Left Port</td>
  </tr>
  <tr>
    <td>2019-01-06</td>
    <td>HP</td>
    <td><a href="https://store.hp.com/us/en/pdp/hp-chromebook-14-db0020nr">HP Chromebook 14 (db0000-db0999)</a></td>
    <td>careena</td>
    <td>grunt</td>
    <td>grunt</td>
    <td>x86_64</td>
    <td>4.14</td>
    <td>x86_64</td>
    <td>Stoney Ridge</td>
    <td>Chromebook</td>
    <td>R70</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Right Port</td>
  </tr>
  <tr>
    <td>2019-01-07</td>
    <td>Acer</td>
    <td><a href="https://www.acer.com/ac/en/US/press/2019/479116">Acer Chromebook 315</a></td>
    <td>Aleena</td>
    <td>grunt</td>
    <td>grunt</td>
    <td>x86_64</td>
    <td>4.14</td>
    <td>x86_64</td>
    <td>Stoney Ridge</td>
    <td>Chromebook</td>
    <td>R71</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Left Port</td>
  </tr>
  <tr>
    <td>2019-01-16</td>
    <td>PCmerge</td>
    <td><a href="/chromium-os/developer-information-for-chrome-os-devices/generic">PCmerge Chromebook AL116</a></td>
    <td>Whitetip</td>
    <td>coral</td>
    <td>coral</td>
    <td>x86_64</td>
    <td>4.4</td>
    <td>x86_64</td>
    <td>ApolloLake</td>
    <td>Chromebook</td>
    <td>R71</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>yes</td>
  </tr>
  <tr>
    <td>2019-01-18</td>
    <td>ASUS</td>
    <td><a href="https://www.asus.com/us/Laptops/ASUS-Chromebook-C403NA/">ASUS Chromebook C403</a></td>
    <td>babymako</td>
    <td>coral</td>
    <td>coral</td>
    <td>x86_64</td>
    <td>4.4</td>
    <td>x86_64</td>
    <td>ApolloLake</td>
    <td>Chromebook</td>
    <td>R71</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>yes</td>
  </tr>
  <tr>
    <td>2019-01-21</td>
    <td>HP</td>
    <td><a href="https://store.hp.com/us/en/pdp/hp-chromebook-11-g6-ee">HP Chromebook 11A G6 EE</a></td>
    <td>Barla</td>
    <td>grunt</td>
    <td>grunt</td>
    <td>x86_64</td>
    <td>4.14</td>
    <td>x86_64</td>
    <td>Stoney Ridge</td>
    <td>Chromebook</td>
    <td>R71</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Right Port</td>
  </tr>
  <tr>
    <td>2019-02-27</td>
    <td>Lenovo</td>
    <td><a href="https://www.lenovo.com/us/en/laptops/lenovo/student-chromebooks/Lenovo-100e-Chromebook-2nd-Gen-MTK/p/88ELC1S9989">Lenovo 100e Chromebook 2nd Gen (MTK)</a></td>
    <td>Sycamore</td>
    <td>hana</td>
    <td>oak</td>
    <td>arm</td>
    <td>3.18</td>
    <td>aarch64</td>
    <td>MT8173</td>
    <td>Chromebook</td>
    <td>R72</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2019-02-27</td>
    <td>Lenovo</td>
    <td><a href="https://www.lenovo.com/us/en/laptops/lenovo/student-chromebooks/Lenovo-300e-Chromebook-2nd-Gen-MTK/p/88ELC1S9988">Lenovo 300e Chromebook 2nd Gen (MTK)</a></td>
    <td>Sycamore360</td>
    <td>hana</td>
    <td>oak</td>
    <td>arm</td>
    <td>3.18</td>
    <td>aarch64</td>
    <td>MT8173</td>
    <td>Chromebook</td>
    <td>R72</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2019-02-27</td>
    <td>Acer</td>
    <td><a href="http://eu-acerforeducation.acer.com/resources/bett-show-2019-acer-unveils-new-suite-of-11-6-inch-chromebooks/">Acer Chromebook 311</a></td>
    <td>bobba</td>
    <td>bobba</td>
    <td>octopus</td>
    <td>x86_64</td>
    <td>4.14</td>
    <td>x86_64</td>
    <td>Gemini Lake</td>
    <td>Chromebook</td>
    <td>R72</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2019-02-27</td>
    <td>Acer</td>
    <td><a href="https://eu-acerforeducation.acer.com/resources/bett-show-2019-acer-unveils-new-suite-of-11-6-inch-chromebooks/">Acer Chromebook Spin 511</a></td>
    <td>bobba360</td>
    <td>bobba360</td>
    <td>octopus</td>
    <td>x86_64</td>
    <td>4.14</td>
    <td>x86_64</td>
    <td>Gemini Lake</td>
    <td>Chromebook</td>
    <td>R72</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2019-03-01</td>
    <td>Lenovo</td>
    <td><a href="Lenovo%20500e%20Chromebook%202nd%20Gen">Lenovo 500e Chromebook 2nd Gen</a></td>
    <td>Phaser360S</td>
    <td>Octopus</td>
    <td></td>
    <td>x86_64</td>
    <td>4.14</td>
    <td>x86_64</td>
    <td>Gemini Lake</td>
    <td>Convertible</td>
    <td>R72</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2019-03-01</td>
    <td>Lenovo</td>
    <td><a href="Lenovo%20100e%20Chromebook%202nd%20Gen">Lenovo 100e Chromebook 2nd Gen (Intel)</a></td>
    <td>Phaser</td>
    <td>Octopus</td>
    <td>octopus</td>
    <td>x86_64</td>
    <td>4.14</td>
    <td>x86_64</td>
    <td>Gemini Lake</td>
    <td>Chromebook</td>
    <td>R72</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2019-03-01</td>
    <td>Lenovo</td>
    <td><a href="Lenovo%20300e%20Chromebook%202nd%20Gen%20(Intel)">Lenovo 300e Chromebook 2nd Gen (Intel)</a></td>
    <td>Phaser360</td>
    <td>Octopus</td>
    <td>octopus</td>
    <td>x86_64</td>
    <td>4.14</td>
    <td>x86_64</td>
    <td>Gemini Lake</td>
    <td>Convertible</td>
    <td>R72</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2019-03-06</td>
    <td>Acer</td>
    <td><a href="https://www.acer.com/ac/en/US/press/2019/484286">Acer Chromebook Spin 512(R851TN)</a></td>
    <td>sparky360</td>
    <td>octopus</td>
    <td>octopus</td>
    <td>x86_64</td>
    <td>4.14</td>
    <td>x86_64</td>
    <td>Gemini Lake</td>
    <td>Convertible</td>
    <td>R72</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>yes</td>
  </tr>
  <tr>
    <td>2019-03-06</td>
    <td>Lenovo</td>
    <td><a href="https://www.lenovo.com/us/en/laptops/lenovo/student-chromebooks/Lenovo-14e-Chromebook/p/88ELC1S9991">Lenovo 14e Chromebook</a></td>
    <td>liara</td>
    <td>liara</td>
    <td>grunt</td>
    <td>x86_64</td>
    <td>4.14</td>
    <td>x86_64</td>
    <td>Stoney Ridge</td>
    <td>Chromebook</td>
    <td>R71</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Left Port</td>
  </tr>
  <tr>
    <td>2019-03-08</td>
    <td>Acer</td>
    <td><a href="https://www.acer.com/ac/en/US/press/2019/484286">Acer Chromebook 512(C851/C851T)</a></td>
    <td>sparky</td>
    <td>octopus</td>
    <td>octopus</td>
    <td>x86_64</td>
    <td>4.14</td>
    <td>x86_64</td>
    <td>Gemini Lake</td>
    <td>Chromebook</td>
    <td>R72</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>yes</td>
  </tr>
  <tr>
    <td>2019-03-15</td>
    <td>ASUS</td>
    <td><a href="https://www.asus.com/us/Commercial-Laptops/ASUS-Chromebook-Flip-C434TA/">ASUS Chromebook Flip C434</a></td>
    <td>Shyvana</td>
    <td>rammus</td>
    <td>rammus</td>
    <td>x86_64</td>
    <td>4.4</td>
    <td>x86_64</td>
    <td>Amberlake-Y</td>
    <td>Convertible</td>
    <td>R72</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>yes</td>
  </tr>
  <tr>
    <td>2019-03-22</td>
    <td>Acer</td>
    <td><a href="https://9to5google.com/2019/01/23/acer-amd-chromebooks-education/">The Acer Chromebook 311 (C721) </a></td>
    <td>Kasumi</td>
    <td>grunt</td>
    <td>grunt</td>
    <td>x86_64</td>
    <td>4.14</td>
    <td>x86_64</td>
    <td>Stoney Ridge</td>
    <td>Chromebook</td>
    <td>R72</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Left Port</td>
  </tr>
  <tr>
    <td>2019-03-22</td>
    <td>Acer</td>
    <td><a href="https://9to5google.com/2019/01/23/acer-amd-chromebooks-education/">Acer Chromebook Spin 311 (R721T)</a></td>
    <td>Kasumi360</td>
    <td>grunt</td>
    <td>grunt</td>
    <td>x86_64</td>
    <td>4.14</td>
    <td>x86_64</td>
    <td>Stoney Ridge</td>
    <td>Chromebook</td>
    <td>R72</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Left Port</td>
  </tr>
  <tr>
    <td>2019-03-30</td>
    <td>ASUS</td>
    <td><a href="https://www.asus.com/Tablets/ASUS-Chromebook-Tablet-CT100PA/">ASUS Chromebook Tablet CT100</a></td>
    <td>Dumo</td>
    <td>scarlet</td>
    <td>scarlet</td>
    <td>arm</td>
    <td>4.4</td>
    <td>aarch64</td>
    <td>RK3399</td>
    <td>Chromeblet</td>
    <td>R73</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2019-04-01</td>
    <td>CTL</td>
    <td><a href="https://ctl.net/collections/tablets/products/ctl-chrome-tablet-tx1">CTL Chromebook Tab Tx1</a></td>
    <td>Druwl</td>
    <td>scarlet</td>
    <td>scarlet</td>
    <td>arm</td>
    <td>4.4</td>
    <td>aarch64</td>
    <td>RK3399</td>
    <td>Chromeblet</td>
    <td>R72</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2019-04-05</td>
    <td>ASUS</td>
    <td><a href="https://www.asus.com/us/2-in-1-PCs/ASUS-Chromebook-Flip-C214MA/">ASUS-Chromebook-Flip-C214</a></td>
    <td>Ampton</td>
    <td>octopus</td>
    <td>octopus</td>
    <td>x86_64</td>
    <td>4.14</td>
    <td>x86_64</td>
    <td>Gemini Lake</td>
    <td>Convertible</td>
    <td>R73</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>yes</td>
  </tr>
  <tr>
    <td>2019-04-05</td>
    <td>ASUS</td>
    <td><a href="https://www.asus.com/us/Laptops/ASUS-Chromebook-C204MA/">ASUS Chromebook Flip C204</a></td>
    <td>Apel</td>
    <td>octopus</td>
    <td>octopus</td>
    <td>x86_64</td>
    <td>4.14</td>
    <td>x86_64</td>
    <td>Gemini Lake</td>
    <td>Chromebook</td>
    <td>R73</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>yes</td>
  </tr>
  <tr>
    <td>2019-04-22</td>
    <td>HP</td>
    <td><a href="https://www8.hp.com/us/en/laptops/product-details/27924352">HP Chromebook 11 G7 EE</a></td>
    <td>Mimrock</td>
    <td>octopus</td>
    <td>octopus</td>
    <td>x86_64</td>
    <td>4.14</td>
    <td>x86_64</td>
    <td>Gemini Lake</td>
    <td>Chromebook</td>
    <td>R73</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2019-04-22</td>
    <td>HP</td>
    <td><a href="http://www8.hp.com/us/en/laptops/product-details/27924571">HP Chromebook x360 11 G2 EE</a></td>
    <td>Meep</td>
    <td>octopus</td>
    <td>octopus</td>
    <td>x86_64</td>
    <td>4.14</td>
    <td>x86_64</td>
    <td>Gemini Lake</td>
    <td>Chromebook</td>
    <td>R73</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2019-06-17</td>
    <td>Lenovo </td>
    <td><a href="https://www.lenovo.com/gb/en/laptops/lenovo/student-chromebooks/Lenovo-Chromebook-C340-11/p/88LGCC31288">Lenovo Chromebook C340-11</a></td>
    <td>Laser</td>
    <td>octopus</td>
    <td>octopus</td>
    <td>x86_64</td>
    <td>4.14</td>
    <td>x86_64</td>
    <td>Gemini Lake</td>
    <td>Convertible</td>
    <td>R74</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>yes</td>
  </tr>
  <tr>
    <td>2019-06-17</td>
    <td>Lenovo</td>
    <td><a href="https://psref.lenovo.com/Product/Lenovo_Laptops/Lenovo_Chromebook_S340">Lenovo Chromebook S340-14</a></td>
    <td>Laser14</td>
    <td>octopus</td>
    <td>octopus</td>
    <td>x86_64</td>
    <td>4.14</td>
    <td>x86_64</td>
    <td>Gemini Lake</td>
    <td>Chromebook</td>
    <td>R74</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>yes</td>
  </tr>
  <tr>
    <td>2019-07-31</td>
    <td>Lenovo</td>
    <td><a href="https://www.lenovo.com/us/en/laptops/lenovo/student-chromebooks/Lenovo-Chromebook-C340-15/p/88LGCC31290">Lenovo Chromebook C340-15</a></td>
    <td>pyke</td>
    <td>nami</td>
    <td>nami</td>
    <td>x86_64</td>
    <td>4.4</td>
    <td>x86_64</td>
    <td>Kabylake-U/R</td>
    <td>Convertible</td>
    <td>M75</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2019-08-20</td>
    <td>ASUS</td>
    <td><a href="https://www.asus.com/Laptops/ASUS-Chromebook-14-C425TA/">ASUS Chromebook C425</a></td>
    <td>leona</td>
    <td>leona</td>
    <td>rammus</td>
    <td>x86_64</td>
    <td>4.4</td>
    <td>x86_64</td>
    <td>Amberlake-Y</td>
    <td>Chromebook</td>
    <td>M75</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2019-08-26</td>
    <td>Dell</td>
    <td><a href="https://www.dell.com/en-us/work/shop/cty/pdp/spd/latitude-13-5300-2-in-1-chrome-laptop">Latitude 5300 2-in-1 Chromebook Enterprise</a></td>
    <td>arcada</td>
    <td>sarien</td>
    <td>sarien</td>
    <td>x86_64</td>
    <td>4.19</td>
    <td>x86_64</td>
    <td>Whiskey Lake</td>
    <td>Chromebook</td>
    <td>R75</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>No</td>
  </tr>
  <tr>
    <td>2019-08-26</td>
    <td>Dell</td>
    <td><a href="https://www.dell.com/en-us/work/shop/dell-laptops-and-notebooks/dell-latitude-5400-chromebook-enterprise/spd/latitude-14-5400-chrome-laptop/xctolc540014us1">Dell Latitude 5400 Chromebook Enterprise</a></td>
    <td>sarien</td>
    <td>sarien</td>
    <td>sarien</td>
    <td>x86_64</td>
    <td>4.19</td>
    <td>x86_64</td>
    <td>Whiskey Lake</td>
    <td>Chromebook</td>
    <td>R75</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>No</td>
  </tr>
  <tr>
    <td>2019-09-02</td>
    <td>ASUS</td>
    <td><a href="https://www.asus.com/2-in-1-PCs/ASUS-Chromebook-Flip-C433TA/">ASUS Chromebook Flip C433TA</a></td>
    <td>shyvana-m</td>
    <td>shyvana</td>
    <td>rammus</td>
    <td>x86_64</td>
    <td>4.4</td>
    <td>x86_64</td>
    <td>Amberlake-Y</td>
    <td>Convertible</td>
    <td>M75</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2019-09-04</td>
    <td>Acer</td>
    <td><a href="https://news.acer.com/acer-delivers-full-lineup-of-chromebooks-for-family-fun-entertainment-productivity">Acer Chromebook Spin 311 (CP311-2H)</a></td>
    <td>gik360</td>
    <td>octopus</td>
    <td>octopus</td>
    <td>x86_64</td>
    <td>4.14</td>
    <td>x86_64</td>
    <td>Gemini Lake</td>
    <td>Convertible</td>
    <td>R76</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2019-09-04</td>
    <td>Acer</td>
    <td><a href="https://news.acer.com/acer-delivers-full-lineup-of-chromebooks-for-family-fun-entertainment-productivity">Acer Chromebook 314 (CB314-1H/1HT)</a></td>
    <td>Droid</td>
    <td>octopus</td>
    <td>octopus</td>
    <td>x86_64</td>
    <td>4.14</td>
    <td>x86_64</td>
    <td>Gemini Lake</td>
    <td>Chromebook</td>
    <td>R76</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2019-09-04</td>
    <td>Acer</td>
    <td><a href="https://news.acer.com/acer-delivers-full-lineup-of-chromebooks-for-family-fun-entertainment-productivity">Acer Chromebook 315 (CB315-3H/3HT)</a></td>
    <td>Blorb</td>
    <td>octopus</td>
    <td>octopus</td>
    <td>x86_64</td>
    <td>4.14</td>
    <td>x86_64</td>
    <td>Gemini Lake</td>
    <td>Chromebook</td>
    <td>R76</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2019-09-04</td>
    <td>Acer</td>
    <td><a href="https://news.acer.com/acer-delivers-full-lineup-of-chromebooks-for-family-fun-entertainment-productivity">Acer Chromebook 311 (CB311-9HT/9H) )</a></td>
    <td>gik</td>
    <td>octopus</td>
    <td>octopus</td>
    <td>x86_64</td>
    <td>4.14</td>
    <td>x86_64</td>
    <td>Gemini Lake</td>
    <td>Chromebook</td>
    <td>R76</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2019-10-07</td>
    <td>Samsung</td>
    <td><a href="https://www.samsung.com/us/business/computing/chromebooks/under-12/chromebook-4-11-6-32gb-storage-4gb-ram-xe310xba-k01us/">Samsung Chromebook 4</a></td>
    <td>Bluebird</td>
    <td>octopus</td>
    <td>octopus</td>
    <td>x86_64</td>
    <td>4.14</td>
    <td>x86_64</td>
    <td>Gemini Lake</td>
    <td>Chromebook</td>
    <td>R76</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>yes</td>
  </tr>
  <tr>
    <td>2019-10-07</td>
    <td>Samsung</td>
    <td><a href="https://www.samsung.com/us/computing/chromebooks/15-18/chromebook-4-15-6-32gb-storage-4gb-ram-xe350xba-k01us/">Samsung Chromebook+</a></td>
    <td>Casta</td>
    <td>octopus</td>
    <td>octopus</td>
    <td>x86_64</td>
    <td>4.14</td>
    <td>x86_64</td>
    <td>Gemini Lake</td>
    <td>Chromebook</td>
    <td>M75</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>yes</td>
  </tr>
  <tr>
    <td>2019-10-15</td>
    <td>Google</td>
    <td><a href="https://store.google.com/us/product/pixelbook_go">Pixelbook Go</a></td>
    <td>atlas</td>
    <td>atlas</td>
    <td>poppy</td>
    <td>x86_64</td>
    <td>4.4</td>
    <td>x86_64</td>
    <td>Amberlake-Y</td>
    <td>Chromebook</td>
    <td>October 2019</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td>Yes</td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2019-12-27</td>
    <td>ASUS</td>
    <td><a href="https://www.asus.com/Laptops/ASUS-Chromebook-C202XA/">ASUS Chromebook C202XA</a></td>
    <td>telesu</td>
    <td>hana</td>
    <td>hana</td>
    <td>arm</td>
    <td>3.18</td>
    <td>aarch64</td>
    <td>MT8173</td>
    <td>Chromebook</td>
    <td>M78</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2020-01-06</td>
    <td>Samsung</td>
    <td><a href="https://www.samsung.com/us/computing/chromebooks/">Samsung Galaxy Chromebook</a></td>
    <td>kohaku</td>
    <td>hatch</td>
    <td>hatch</td>
    <td>x86_64</td>
    <td>4.19</td>
    <td>x86_64</td>
    <td>Cometlake-U</td>
    <td>Convertible</td>
    <td>M79</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2020-01-07</td>
    <td>Lenovo</td>
    <td><a href="https://www.google.com/chromebook/device/lenovo-ideapad-duet-chromebook/">Lenovo Chromebook Duet/Lenovo IdeaPad Duet Chromebook</a></td>
    <td>krane</td>
    <td>kukui</td>
    <td>kukui</td>
    <td>arm</td>
    <td>4.19</td>
    <td>aarch64</td>
    <td>MT8183</td>
    <td>Chromeblet</td>
    <td>R79</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2020-01-13</td>
    <td>ASUS</td>
    <td><a href="https://www.asus.com/2-in-1-PCs/ASUS-Chromebook-Flip-C436FA/">ASUS Chromebook Flip C436FA</a></td>
    <td>helios</td>
    <td>hatch</td>
    <td>hatch</td>
    <td>x86_64</td>
    <td>4.19</td>
    <td>x86_64</td>
    <td>Cometlake-U</td>
    <td>Convertible</td>
    <td>M79</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2020-01-14</td>
    <td>Lenovo</td>
    <td><a href="https://www.lenovo.com/us/en/coming-soon/Lenovo-10e-Chromebook-Tablet/p/22ED10E10E1">Lenovo 10e Chromebook Tablet</a></td>
    <td>kodama</td>
    <td>kukui</td>
    <td>kukui</td>
    <td>arm</td>
    <td>4.19</td>
    <td>aarch64</td>
    <td>MT8183</td>
    <td>Chromeblet</td>
    <td>M80</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2020-01-20</td>
    <td>HP</td>
    <td><a href="https://store.hp.com/us/en/pdp/hp-chromebook-11-g8-ee-notebook-pc-customizable-8mq33av-mb">HP Chromebook 11 G8 EE</a></td>
    <td>Vorticon</td>
    <td>octopus</td>
    <td>octopus</td>
    <td>x86_64</td>
    <td>4.14</td>
    <td>x86_64</td>
    <td>Gemini Lake</td>
    <td>Chromebook</td>
    <td>R78</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2020-01-20</td>
    <td>HP</td>
    <td><a href="https://store.hp.com/us/en/pdp/hp-chromebook-x360-11-g3-ee-notebook-pc-customizable-8mm99av-mb">HP Chromebook x360 11 G3 EE</a></td>
    <td>Vortininja</td>
    <td>octopus</td>
    <td>octopus</td>
    <td>x86_64</td>
    <td>4.14</td>
    <td>x86_64</td>
    <td>Gemini Lake</td>
    <td>Chromebook</td>
    <td>R78</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2020-01-20</td>
    <td>HP</td>
    <td><a href="https://store.hp.com/us/en/pdp/hp-chromebook-14-g5-notebook-pc-customizable-8mq26av-mb">HP Chromebook 14 G6</a></td>
    <td>Dorp</td>
    <td>octopus</td>
    <td>octopus</td>
    <td>x86_64</td>
    <td>4.14</td>
    <td>x86_64</td>
    <td>Gemini Lake</td>
    <td>Chromebook</td>
    <td>R78</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2020-02-28</td>
    <td>Lenovo</td>
    <td><a href="https://www.lenovo.com/us/en/laptops/lenovo/student-chromebooks/Lenovo-300e-Chromebook-2nd-Gen-AST/p/88ELC1S8036">Lenovo 300e 2nd Gen AMD</a></td>
    <td>Treeya360</td>
    <td>Grunt</td>
    <td></td>
    <td>x86_64</td>
    <td>4.14</td>
    <td>x86_64</td>
    <td>Stoney Ridge</td>
    <td>Convertible</td>
    <td>R80</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2020-02-28</td>
    <td>HP</td>
    <td><a href="https://www.google.com/chromebook/device/hp-chromebook-14a/">HP Chromebook 14a</a></td>
    <td>blooglet</td>
    <td>octopus</td>
    <td>octopus</td>
    <td>x86_64</td>
    <td>4.14</td>
    <td>x86_64</td>
    <td>Gemini Lake</td>
    <td>Chromebook</td>
    <td>R79</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2020-02-28</td>
    <td>Lenovo</td>
    <td><a href="https://www.lenovo.com/us/en/laptops/lenovo/student-chromebooks/Lenovo-100e-Chromebook-2nd-Gen-AST/p/88ELC1S8035">Lenovo 100e 2nd Gen AMD</a></td>
    <td>Treeya</td>
    <td>Grunt</td>
    <td></td>
    <td>x86_64</td>
    <td>4.14</td>
    <td>x86_64</td>
    <td>Stoney Ridge</td>
    <td>Chromebook</td>
    <td>R80</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2020-05-01</td>
    <td>Acer</td>
    <td><a href="https://www.acer.com/ac/en/US/content/professional-models/laptops/acerchromebook314">Acer Chromebook 314 (C933L/LT)</a></td>
    <td>droid</td>
    <td>octopus</td>
    <td>octopus</td>
    <td>x86_64</td>
    <td>4.14</td>
    <td>x86_64</td>
    <td>Gemini Lake</td>
    <td>Chromebook</td>
    <td>R80</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2020-06-23</td>
    <td>Acer</td>
    <td><a href="Acer%20Chromebook%20Spin%20311%20(CP311-3H)">Acer Chromebook Spin 311 (CP311-3H)</a></td>
    <td>juniper</td>
    <td>jacuzzi</td>
    <td>jacuzzi</td>
    <td>arm</td>
    <td>4.19</td>
    <td>aarch64</td>
    <td>MT8183</td>
    <td>Chromebook</td>
    <td>M81</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2020-07-31</td>
    <td>Lenovo</td>
    <td><a href="https://www.google.com/chromebook/device/lenovo-ideapad-3-chromebook-11-05/">Ideapad 3 Chromebook</a></td>
    <td>Lick</td>
    <td>Octopus</td>
    <td></td>
    <td>x86_64</td>
    <td>4.14</td>
    <td>x86_64</td>
    <td>Gemini Lake</td>
    <td>Chromebook</td>
    <td>R80</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2020-08-11</td>
    <td>Dell</td>
    <td><a href="https://www.dell.com/en-us/work/shop/laptops/new-14-7410/spd/latitude-14-7410-2-in-1-chrome-laptop">Dell Latitude 7410 Chromebook Enterprise</a></td>
    <td>Drallion</td>
    <td>Drallion</td>
    <td>Drallion</td>
    <td>x86_64</td>
    <td>4.19</td>
    <td>x86_64</td>
    <td>Cometlake-U</td>
    <td>Chromebook</td>
    <td>R83</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2020-10-12</td>
    <td>Lenovo</td>
    <td><a href="https://www.lenovo.com/us/en/laptops/thinkpad/thinkpad-c-series/ThinkPad-C13-Yoga-Chromebook-Enterprise/p/22TPC13C3Y1">Lenovo ThinkPad C13 Yoga Chromebook</a></td>
    <td>morphius</td>
    <td>zork</td>
    <td>zork</td>
    <td>x86_64</td>
    <td>5.4</td>
    <td>x86_64</td>
    <td>Picasso/Dali</td>
    <td>Convertible</td>
    <td>M86</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2020-10-23</td>
    <td>HP</td>
    <td><a href="http://www.chromium.org/chromium-os/developer-information-for-chrome-os-devices/generic">HP Chromebox G3</a></td>
    <td>Noibat</td>
    <td>Puff</td>
    <td>Puff</td>
    <td>x86_64</td>
    <td>4.19</td>
    <td>x86_64</td>
    <td>Cometlake-U</td>
    <td>Chromebox</td>
    <td>R85</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2020-10-29</td>
    <td>ASUS</td>
    <td><a href="http://www.chromium.org/chromium-os/developer-information-for-chrome-os-devices/generic">ASUS Chromebox 4</a></td>
    <td>Duffy</td>
    <td>Puff</td>
    <td>Puff</td>
    <td>x86_64</td>
    <td>4.19</td>
    <td>x86_64</td>
    <td>Cometlake-U</td>
    <td>Chromebox</td>
    <td>R85</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2020-10-29</td>
    <td>Acer</td>
    <td><a href="http://www.chromium.org/chromium-os/developer-information-for-chrome-os-devices/generic">Acer Chromebox CXI4</a></td>
    <td>Kaisa</td>
    <td>Puff</td>
    <td>Puff</td>
    <td>x86_64</td>
    <td>4.19</td>
    <td>x86_64</td>
    <td>Cometlake-U</td>
    <td>Chromebox</td>
    <td>R85</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2020-11-13</td>
    <td>ASUS</td>
    <td><a href="http://www.chromium.org/chromium-os/developer-information-for-chrome-os-devices/generic">ASUS Fanless Chromebox</a></td>
    <td>Faffy</td>
    <td>Puff</td>
    <td>Puff</td>
    <td>x86_64</td>
    <td>4.19</td>
    <td>x86_64</td>
    <td>Cometlake-U</td>
    <td>Chromebox</td>
    <td>R85</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2020-12-29</td>
    <td>HP</td>
    <td><a href="https://www8.hp.com/h20195/v2/GetDocument.aspx?docname=4AA7-9044ENUC">HP Pro c645 Chromebook Enterprise</a></td>
    <td>berknip</td>
    <td>zork</td>
    <td>zork</td>
    <td>x86_64</td>
    <td>5.4</td>
    <td>x86_64</td>
    <td>Picasso/Dali</td>
    <td>Chromebook</td>
    <td>M86</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
   <tr>
    <td>2022-06-10</td>
    <td>HP</td>
    <td><a href="https://press.hp.com/us/en/blogs/2022/new-hp-chromebooks-cloud-first-computing.html">HP Elite c645 G2 Chromebook Enterprise</a></td>
    <td>nipperkin</td>
    <td>guybrush</td>
    <td>guybrush</td>
    <td>x86_64</td>
    <td>5.10</td>
    <td>x86_64</td>
    <td>Cezanne/Barcelo</td>
    <td>Chromebook</td>
    <td>M100</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2021-01-07</td>
    <td>Samsung</td>
    <td><a href="https://www.samsung.com/us/computing/chromebooks/galaxy-chromebook2/">Samsung Galaxy Chromebook 2</a></td>
    <td>nightfury</td>
    <td>hatch</td>
    <td>hatch</td>
    <td>x86_64</td>
    <td>4.19</td>
    <td>x86_64</td>
    <td>Cometlake-U</td>
    <td>Convertible</td>
    <td></td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2021-01-13</td>
    <td>Acer</td>
    <td><a href="https://www.acer.com/ac/en/US/content/series/acerchromebookspin514">Acer Chromebook Spin 514</a></td>
    <td>Ezkinil</td>
    <td>Zork</td>
    <td>Zork</td>
    <td>x86_64</td>
    <td>4.19</td>
    <td>x86_64</td>
    <td>Picasso/Dali</td>
    <td>Convertible</td>
    <td>R86</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2021-01-22</td>
    <td>Acer</td>
    <td><a href="https://www.acer.com/ac/en/US/content/series/acerchromebookspin513">Acer Chromebook Spin 513</a></td>
    <td>Lazor</td>
    <td>Trogdor</td>
    <td>Trogdor</td>
    <td>arm</td>
    <td>5.4</td>
    <td>aarch64</td>
    <td>QC-7C</td>
    <td>Convertible</td>
    <td>R87</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2021-01-29</td>
    <td>HP</td>
    <td><a href="https://support.hp.com/us-en/document/c06995206#AbT0">HP Chromebook 11MK G9 EE</a></td>
    <td>esche</td>
    <td>jacuzzi</td>
    <td>jacuzzi</td>
    <td>arm</td>
    <td>4.19</td>
    <td>aarch64</td>
    <td>MT8183</td>
    <td>Chromebook</td>
    <td>R87</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2021-01-29</td>
    <td>HP</td>
    <td><a href="https://store.hp.com/us/en/pdp/hp-chromebook-x360-11mk-g3-ee-customizable-2p2j9av-mb">HP Chromebook x360 11MK G3 EE</a></td>
    <td>burnet</td>
    <td>jacuzzi</td>
    <td>jacuzzi</td>
    <td>arm</td>
    <td>4.19</td>
    <td>aarch64</td>
    <td>MT8183</td>
    <td>Convertible</td>
    <td>R87</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2021-03-05</td>
    <td>HP</td>
    <td><a href="https://support.hp.com/us-en/document/c07056426">HP Chromebook 14 G7</a></td>
    <td>Drawman</td>
    <td>Dedede</td>
    <td>Dedede</td>
    <td>x86_64</td>
    <td>5.4</td>
    <td>x86_64</td>
    <td>Jasper Lake</td>
    <td>Chromebook</td>
    <td>R87</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2021-03-12</td>
    <td>HP</td>
    <td><a href="https://support.hp.com/us-en/document/c07030816">HP Chromebook 11 G9 EE</a></td>
    <td>Drawlat</td>
    <td>Dedede</td>
    <td>Dedede</td>
    <td>x86_64</td>
    <td>5.4</td>
    <td>x86_64</td>
    <td>Jasper Lake</td>
    <td>Chromebook</td>
    <td>R87</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2021-03-19</td>
    <td>HP</td>
    <td><a href="https://support.hp.com/us-en/product/hp-chromebook-x360-11-g4-education-edition/38457215/product-info">HP Chromebook x360 11 G4 EE</a></td>
    <td>Drawcia</td>
    <td>Dedede</td>
    <td>Dedede</td>
    <td>x86_64</td>
    <td>5.4</td>
    <td>x86_64</td>
    <td>Jasper Lake</td>
    <td>Chromebook</td>
    <td>R88</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2021-03-26</td>
    <td>HP</td>
    <td><a href="https://www.hp.com/us-en/shop/pdp/hp-chromebook-14a-nd0097nr">HP Chromebook 14a-nd0097nr</a></td>
    <td>dirinboz</td>
    <td>zork</td>
    <td>dalboz</td>
    <td>x86_64</td>
    <td>5.4</td>
    <td>x86_64</td>
    <td>Pollock</td>
    <td>Chromebook</td>
    <td>R88</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2021-04-16</td>
    <td>Asus</td>
    <td><a href="https://www.asus.com/Laptops/For-Home/Chromebook/ASUS-Chromebook-Flip-CM5-CM5500/">ASUS Chromebook Flip CM5</a></td>
    <td>woomax</td>
    <td>zork</td>
    <td>zork</td>
    <td>x86_64</td>
    <td>5.4</td>
    <td>x86_64</td>
    <td>Picasso/Dali</td>
    <td>Chromebook</td>
    <td>R88</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2021-05-25</td>
    <td>Lenovo</td>
    <td><a href="https://www.lenovo.com/us/en/coming-soon/500e-G3/p/22ED05E5EN3">Lenovo 500e Chromebook Gen 3</a></td>
    <td>Boten</td>
    <td>boten</td>
    <td>dedede</td>
    <td>x86_64</td>
    <td>5.4</td>
    <td>x86_64</td>
    <td>Jasper Lake</td>
    <td>Convertible</td>
    <td>R90</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2021-05-27</td>
    <td>Acer</td>
    <td><a href="https://news.acer.com/acer-launches-four-new-chromebooks-including-industrys-first-17-inch-model">The Acer Chromebook 514 (CB514-1H)</a></td>
    <td>Volta</td>
    <td>Volteer</td>
    <td>Volteer</td>
    <td>x86_64</td>
    <td>5.4</td>
    <td>x86_64</td>
    <td>TigerLake-UP3</td>
    <td>Chromebook</td>
    <td>R91</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2021-05-27</td>
    <td>Acer</td>
    <td><a href="https://www.acer.com/ac/en/US/content/series/acerchromebookspin713cp7133w">Acer Chromebook Spin 713 (CP713-3W)</a></td>
    <td>Voxel</td>
    <td>Volteer</td>
    <td>Volteer</td>
    <td>x86_64</td>
    <td>5.4</td>
    <td>x86_64</td>
    <td>TigerLake-UP3</td>
    <td>Convertible</td>
    <td>R89</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2021-05-30</td>
    <td>Lenovo</td>
    <td><a href="">Lenovo Flex 3i-11 Chromebook</a></td>
    <td>botenflex</td>
    <td>boten</td>
    <td>dedede</td>
    <td>x86_64</td>
    <td>5.4</td>
    <td>x86_64</td>
    <td>Jasper Lake</td>
    <td>Chromebook</td>
    <td>R90</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2021-05-31</td>
    <td>Asus</td>
    <td><a href="https://www.asus.com/Laptops/For-Home/Chromebook/ASUS-Chromebook-Flip-CX5-CX5500/">ASUS Chromebook Flip CX5 (CX5500)</a></td>
    <td>delbin</td>
    <td>Volteer</td>
    <td>Volteer</td>
    <td>x86_64</td>
    <td>5.4</td>
    <td>x86_64</td>
    <td>TigerLake-UP3</td>
    <td>Convertible</td>
    <td>R89</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2021-06-04</td>
    <td>HP</td>
    <td><a href="https://www8.hp.com/us/en/cloud-computing/chrome-enterprise/hp-pro-c640-chromebook.html"> HP Pro c640 G2 Chromebook</a></td>
    <td>Elemi</td>
    <td>Volteer</td>
    <td>Volteer</td>
    <td>x86_64</td>
    <td>5.4</td>
    <td>x86_64</td>
    <td>TigerLake-UP3</td>
    <td>Chromebook</td>
    <td>R90</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2021-06-16</td>
    <td>HP</td>
    <td><a href="https://support.hp.com/ca-en/document/c07657289">HP Chromebook 14a</a></td>
    <td>Lantis</td>
    <td>Dedede</td>
    <td>Dedede</td>
    <td>x86_64</td>
    <td>5.4</td>
    <td>x86_64</td>
    <td>Jasper Lake</td>
    <td>Chromebook</td>
    <td>R90</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2021-06-26</td>
    <td>Asus</td>
    <td><a href="">ASUS Chromebook Flip CX5 (CX5400)</a></td>
    <td>copano</td>
    <td>copano</td>
    <td>terrador</td>
    <td>x86_64</td>
    <td>5.4</td>
    <td>x86_64</td>
    <td>TigerLake-UP4</td>
    <td>Chromebook</td>
    <td>R90</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2021-06-30</td>
    <td>Acer</td>
    <td><a href="https://www.acer.com/ac/en/US/content/series/acerchromebook511c741l">Acer Chromebook 511</a></td>
    <td>Limozeen</td>
    <td>Trogdor</td>
    <td>Trogdor</td>
    <td>arm</td>
    <td>5.4</td>
    <td>aarch64</td>
    <td>QC-7C</td>
    <td>Chromebook</td>
    <td>R90</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2021-07-07</td>
    <td>HP</td>
    <td><a href="https://support.hp.com/th-en/document/c07653399">HP Chromebook x360 14a</a></td>
    <td>Gumboz</td>
    <td>zork</td>
    <td>dalboz</td>
    <td>x86_64</td>
    <td>5.4</td>
    <td>x86_64</td>
    <td>Pollock</td>
    <td>Chromebook</td>
    <td>R89</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2021-07-20</td>
    <td>Asus</td>
    <td><a href="https://www.asus.com/Laptops/For-Work/Chromebook/ASUS-Chromebook-CX9-CX9400-11th-Gen-Intel/">ASUS Chromebook CX9 (CX9400)</a></td>
    <td>drobit</td>
    <td>drobit</td>
    <td>Volteer</td>
    <td>x86_64</td>
    <td>5.4</td>
    <td>x86_64</td>
    <td>TigerLake-UP3</td>
    <td>Chromebook</td>
    <td>R90</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2021-08-02</td>
    <td>Asus</td>
    <td><a href="https://www.asus.com/Laptops/For-Home/Chromebook/ASUS-Chromebook-CX1-CX1500/">Asus Chromebook CX1500</a></td>
    <td>Galith</td>
    <td>Dedede</td>
    <td>Dedede</td>
    <td>x86_64</td>
    <td>5.4</td>
    <td>x86_64</td>
    <td>Jasper Lake</td>
    <td>Chromebook</td>
    <td>R90</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2021-08-05</td>
    <td>Samsung</td>
    <td><a href="https://www.samsungmobilepress.com/mediaresources/galaxy_chromebook_go">Galaxy Chromebook Go</a></td>
    <td>sasuke</td>
    <td>sasuke</td>
    <td>dedede</td>
    <td>x86_64</td>
    <td>5.4</td>
    <td>x86_64</td>
    <td>Jasper Lake</td>
    <td>Chromebook</td>
    <td>R90</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2021-08-20</td>
    <td>ASUS</td>
    <td><a href="https://www.asus.com/Laptops/For-Home/Chromebook/ASUS-Chromebook-CX1-CX1101/">ASUS Chromebook CX1101</a></td>
    <td>Apele</td>
    <td>octopus</td>
    <td>octopus</td>
    <td>x86_64</td>
    <td>4.14</td>
    <td>x86_64</td>
    <td>Gemini Lake</td>
    <td>Chromebook</td>
    <td>R91</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2021-08-21</td>
    <td>ASUS</td>
    <td><a href="https://www.asus.com/laptops/for-home/all-series/filter?Series=Chromebook&amp;SubSeries=Chromebook,Chromebook-Flip,Chromebook-Detachable">ASUS Chromebook C424</a></td>
    <td>Nospike</td>
    <td>Octopus</td>
    <td>Octopus</td>
    <td>x86_64</td>
    <td>4.14</td>
    <td>x86_64</td>
    <td>Gemini Lake</td>
    <td>Chromebook</td>
    <td>M75</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2021-08-23</td>
    <td>ASUS</td>
    <td><a href="http://www.chromium.org/chromium-os/developer-information-for-chrome-os-devices/generic">ASUS Chromebook Flip CM1</a></td>
    <td>Jelboz360</td>
    <td>zork</td>
    <td>dalboz</td>
    <td>x86_64</td>
    <td>5.4</td>
    <td>x86_64</td>
    <td>Pollock</td>
    <td>Chromebook</td>
    <td>R92</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2021-08-24</td>
    <td>Asus</td>
    <td><a href="https://www.asus.com/Laptops/For-Home/Chromebook/ASUS-Chromebook-CX1-CX1700/">ASUS Chromebook CX1700</a></td>
    <td>Gallop</td>
    <td>Dedede</td>
    <td>Dedede</td>
    <td>x86_64</td>
    <td>5.4</td>
    <td>x86_64</td>
    <td>Jasper Lake</td>
    <td>Chromebook</td>
    <td>R91</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2021-09-23</td>
    <td>Lenovo</td>
    <td><a href="https://www.lenovo.com/ca/en/coming-soon/IdeaPad-Duet-5-Chromebook/p/LEN101I0023">Lenovo Chromebook Duet 5 /IdeaPad Duet 5 Chromebook</a></td>
    <td>Homestar</td>
    <td>Strongbad</td>
    <td>Strongbad</td>
    <td>arm</td>
    <td>5.4</td>
    <td>aarch64</td>
    <td>QC-7C</td>
    <td>Chromeblet</td>
    <td>R92</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2021-09-27</td>
    <td>Asus</td>
    <td><a href="https://www.asus.com/Laptops/For-Students/Chromebook/ASUS-Chromebook-CR1-CR1100/">ASUS Chromebook CR1100</a></td>
    <td>Storo</td>
    <td>Dedede</td>
    <td>Dedede</td>
    <td>x86_64</td>
    <td>5.4</td>
    <td>x86_64</td>
    <td>Jasper Lake</td>
    <td>Chromebook</td>
    <td>R92</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2021-09-27</td>
    <td>Asus</td>
    <td><a href="https://www.asus.com/Laptops/For-Students/Chromebook/ASUS-Chromebook-Flip-CR1-CR1100/">ASUS Chromebook Flip CR1100</a></td>
    <td>Storo360</td>
    <td>Dedede</td>
    <td>Dedede</td>
    <td>x86_64</td>
    <td>5.4</td>
    <td>x86_64</td>
    <td>Jasper Lake</td>
    <td>Convertible</td>
    <td>R92</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2021-10-14</td>
    <td>Acer</td>
    <td><a href="https://www.acer.com/ac/en/US/content/series/acerchromebook514cb5142h">Acer Chromebook 514</a></td>
    <td>Spherion</td>
    <td>asurada</td>
    <td>asurada</td>
    <td>arm</td>
    <td>5.4</td>
    <td>aarch64</td>
    <td>MT8183</td>
    <td>Chromebook</td>
    <td>M93</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2021-10-29</td>
    <td>Acer</td>
    <td><a href="https://www.acer.com/ac/en/US/content/series/acerchromebook515cb5151w">Acer Chromebook 515</a></td>
    <td>Volet</td>
    <td>volteer</td>
    <td>volteer</td>
    <td>x86_64</td>
    <td>5.4</td>
    <td>x86_64</td>
    <td>TigerLake-UP3</td>
    <td>Chromebook</td>
    <td>M93</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2021-12-31</td>
    <td>Acer</td>
    <td><a href="https://www.acer.com/ac/en/US/content/series/acerchromebook311">Acer Chromebook Spin 311</a></td>
    <td>Pico</td>
    <td>Jacuzzi</td>
    <td>Jacuzzi</td>
    <td>arm</td>
    <td>4.19</td>
    <td>arm</td>
    <td>MT8183</td>
    <td>Chromebook</td>
    <td>M96</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>2022-03-11</td>
    <td>Lenovo</td>
    <td><a href="https://www.lenovo.com/us/en/p/laptops/lenovo/lenovo-edu-chromebooks/ideapad-duet-3-chromebook-(11-inch-qcom)/len101i0034">Lenovo Chromebook Duet 3 /IdeaPad Duet 3 Chromebook</a></td>
    <td>Wormdingler</td>
    <td>Strongbad</td>
    <td>Strongbad</td>
    <td>arm</td>
    <td>5.4</td>
    <td>aarch64</td>
    <td>QC-7C</td>
    <td>Chromeblet</td>
    <td>R97</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2022-04-08</td>
    <td>Asus</td>
    <td><a href="https://www.asus.com/Laptops/For-Home/Chromebook/ASUS-Chromebook-Flip-CX1-CX1400/">ASUS Chromebook Flip CX1400</a></td>
    <td>Galtic360</td>
    <td>Dedede</td>
    <td>Dedede</td>
    <td>x86_64</td>
    <td>5.4</td>
    <td>x86_64</td>
    <td>Jasper Lake</td>
    <td>Convertible</td>
    <td>R97</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2022-04-14</td>
    <td>Lenovo</td>
    <td><a href="https://pcsupport.lenovo.com/us/en/products/laptops-and-netbooks/lenovo-chromebooks-series/100e-gen3-jpl/82uy">Lenovo 100e Gen3</a></td>
    <td>Bookem</td>
    <td>Dedede</td>
    <td>Dedede</td>
    <td>x86_64</td>
    <td>5.4</td>
    <td>x86_64</td>
    <td>Jasper Lake</td>
    <td>Clamshell</td>
    <td>M99</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2022-04-29</td>
    <td>Asus</td>
    <td><a href="https://www.asus.com/Laptops/For-Home/Chromebook/ASUS-Chromebook-Flip-CX1-CX1500FKA/">ASUS Chromebook Flip CX1500</a></td>
    <td>Galith360</td>
    <td>Dedede</td>
    <td>Dedede</td>
    <td>x86_64</td>
    <td>5.4</td>
    <td>x86_64</td>
    <td>Jasper Lake</td>
    <td>Convertible</td>
    <td>R97</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2022-07-29</td>
    <td>Asus</td>
    <td><a href="https://www.asus.com/Laptops/For-Home/Chromebook/ASUS-Chromebook-CX1-CX1400/">ASUS Chromebook CX1 CX1400</a></td>
    <td>Galtic</td>
    <td>Dedede</td>
    <td>Dedede</td>
    <td>x86_64</td>
    <td>5.4</td>
    <td>x86_64</td>
    <td>Jasper Lake</td>
    <td>Clamshell</td>
    <td>R97</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2022-08-12</td>
    <td>Asus</td>
    <td>ASUS Chromebook CX1 CX1102</td>
    <td>Galnat</td>
    <td>Dedede</td>
    <td>Dedede</td>
    <td>x86_64</td>
    <td>5.4</td>
    <td>x86_64</td>
    <td>Jasper Lake</td>
    <td>Clamshell</td>
    <td>R103</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2022-08-12</td>
    <td>Asus</td>
    <td>ASUS Chromebook Flip CX1 CX1102</td>
    <td>Galnat360</td>
    <td>Dedede</td>
    <td>Dedede</td>
    <td>x86_64</td>
    <td>5.4</td>
    <td>x86_64</td>
    <td>Jasper Lake</td>
    <td>Convertible</td>
    <td>R103</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2022-08-31</td>
    <td>Acer</td>
    <td>Acer Chromebook Spin 514 </td>
    <td>DeWatt</td>
    <td>guybrush</td>
    <td>guybrush</td>
    <td>x86_64</td>
    <td>5.10</td>
    <td>x86_64</td>
    <td>Barcelo</td>
    <td>Convertible</td>
    <td>R103</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2022-08-31</td>
    <td>Acer</td>
    <td><a href="https://www.acer.com/ac/en/US/content/series/acerchromebookvero514/">Acer Chromebook Vero 514</a></td>
    <td>Volmar</td>
    <td>Brya</td>
    <td>Brya</td>
    <td>x86_64</td>
    <td>5.10</td>
    <td>x86_64</td>
    <td>Alder Lake</td>
    <td>Clamshell</td>
    <td>R104</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2022-09-27</td>
    <td>HP</td>
    <td><a href="https://www.hp.com/us-en/shop/pdp/hp-chromebook-x360-133-laptop-13b-ca000-73x68av-1">HP Chromebook x360 13.3" Laptop - 13b-ca000</a></td>
    <td>Dojo</td>
    <td>Cherry</td>
    <td>Cherry</td>
    <td>arm</td>
    <td>5.10</td>
    <td>arm</td>
    <td>MT8195/MT8195T</td>
    <td>Convertible</td>
    <td>R104</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2022-08-31</td>
    <td>Acer</td>
    <td><a href="https://www.acer.com/us-en/chromebooks/acer-chromebook-516-ge-cbg516-1h">Acer Chromebook 516 GE (CBG516-1H)</a></td>
    <td>Osiris</td>
    <td>Brya</td>
    <td>Brya</td>
    <td>x86_64</td>
    <td>5.10</td>
    <td>x86_64</td>
    <td>Alder Lake</td>
    <td>Clamshell</td>
    <td>R105</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2022-11-28</td>
    <td>Framework</td>
    <td><a href="https://frame.work/laptop-chromebook-12-gen-intel/">Framework Laptop Chromebook Edition</a></td>
    <td>Banshee</td>
    <td>Brya</td>
    <td>Brya</td>
    <td>x86_64</td>
    <td>5.10</td>
    <td>x86_64</td>
    <td>Alder Lake</td>
    <td>Clamshell</td>
    <td>R106</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2022-12-07</td>
    <td>Lenovo</td>
    <td>Lenovo ThinkCentre M60q Chromebox</td>
    <td>Kinox</td>
    <td>Brask</td>
    <td>Brask</td>
    <td>x86_64</td>
    <td>5.10</td>
    <td>x86_64</td>
    <td>Alder Lake</td>
    <td>Chromebox</td>
    <td>R106</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2023-01-18</td>
    <td>Lenovo</td>
    <td>Lenovo 100e Chromebook Gen 4</td>
    <td>Rusty</td>
    <td>Corsola</td>
    <td>Corsola</td>
    <td>arm</td>
    <td>5.15</td>
    <td>arm</td>
    <td>MT8186</td>
    <td>Clamshell</td>
    <td>R108</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2023-01-18</td>
    <td>Lenovo</td>
    <td>Lenovo 300e Yoga Chromebook Gen 4</td>
    <td>Steelix</td>
    <td>Corsola</td>
    <td>Corsola</td>
    <td>arm</td>
    <td>5.15</td>
    <td>arm</td>
    <td>MT8186</td>
    <td>Convertible</td>
    <td>R108</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2023-01-19</td>
    <td>Acer</td>
    <td><a href="https://www.acer.com/us-en/chromebooks/acer-chromebook-vero-712-cv872-cv872t">Acer Chromebook Vero 712 (CV872/CV872T)</a></td>
    <td>Zavala</td>
    <td>Brya</td>
    <td>Brya</td>
    <td>x86_64</td>
    <td>5.10</td>
    <td>x86_64</td>
    <td>Alder Lake</td>
    <td>Clamshell</td>
    <td>R108</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2023-01-18</td>
    <td>Lenovo</td>
    <td>Lenovo IdeaPad Slim 3 Chromebook (14M868)</td>
    <td>Magneton</td>
    <td>Corsola</td>
    <td>Corsola</td>
    <td>arm</td>
    <td>5.15</td>
    <td>arm</td>
    <td>MT8186</td>
    <td>Convertible</td>
    <td>R108</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2023-03-01</td>
    <td>Asus</td>
    <td>ASUS Chromebook CM14 Flip (CM1402F)</td>
    <td>Tentacruel</td>
    <td>Corsola</td>
    <td>Corsola</td>
    <td>arm</td>
    <td>5.15</td>
    <td>arm</td>
    <td>MT8186</td>
    <td>Convertible</td>
    <td>R109</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2023-03-01</td>
    <td>Asus</td>
    <td>ASUS Chromebook CM14 (CM1402C)</td>
    <td>Tentacool</td>
    <td>Corsola</td>
    <td>Corsola</td>
    <td>arm</td>
    <td>5.15</td>
    <td>arm</td>
    <td>MT8186</td>
    <td>Clamshell</td>
    <td>R109</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td></td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2023-03-31</td>
    <td>Lenovo</td>
    <td>Lenovo 500e Yoga Chromebook Gen 4</td>
    <td>Pujjo</td>
    <td>Nissa</td>
    <td>Nissa</td>
    <td>x86_64</td>
    <td>5.15</td>
    <td>x86_64</td>
    <td>AlderLake-N</td>
    <td>Convertible</td>
    <td>R110</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td>Yes</td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2023-03-31</td>
    <td>Lenovo</td>
    <td>Lenovo IdeaPad Flex 3i Chromebook</td>
    <td>Pujjoflex</td>
    <td>Nissa</td>
    <td>Nissa</td>
    <td>x86_64</td>
    <td>5.15</td>
    <td>x86_64</td>
    <td>AlderLake-N</td>
    <td>Convertible</td>
    <td>R110</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td>Yes</td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2023-03-31</td>
    <td>Lenovo</td>
    <td>Lenovo 14e Chromebook Gen 3</td>
    <td>Pujjoteen</td>
    <td>Nissa</td>
    <td>Nissa</td>
    <td>x86_64</td>
    <td>5.15</td>
    <td>x86_64</td>
    <td>AlderLake-N</td>
    <td>Chromebook</td>
    <td>R110</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td>Yes</td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2023-03-31</td>
    <td>Lenovo</td>
    <td>Lenovo IdeaPad Slim 3i Chromebook</td>
    <td>Pujjoteen15W</td>
    <td>Nissa</td>
    <td>Nissa</td>
    <td>x86_64</td>
    <td>5.15</td>
    <td>x86_64</td>
    <td>AlderLake-N</td>
    <td>Chromebook</td>
    <td>R110</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td>Yes</td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>2023-05-12</td>
    <td>Asus</td>
    <td><a href="https://www.asus.com/laptops/for-home/chromebook/asus-chromebook-cm34-flip-cm3401/">ASUS Chromebook CM34 Flip</a></td>
    <td>Frostflow</td>
    <td>Skyrim</td>
    <td>Skyrim</td>
    <td>x86_64</td>
    <td>5.15</td>
    <td>x86_64</td>
    <td>Mendocino</td>
    <td>Convertible</td>
    <td>R112</td>
    <td><a href="https://support.google.com/chrome/a/answer/6220366">AUE Schedule</a></td>
    <td>Yes</td>
    <td>Yes</td>
  </tr>
</table>
