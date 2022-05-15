# Steam on Chrome OS Alpha

Beginning with **Chrome OS 14583.0.0**, the Dev channel will include an early,
alpha-quality version of Steam on Chrome OS for a small set of recent
Chromebooks. If you have one of these Chromebooks (listed below) and decide to
give it a try, please send your feedback - we are in active development and want
to incorporate as much user input as possible.

## What do we mean by alpha?

Alpha means **anything can break**. Due to the inherent instability of the Dev
channel and the in-progress nature of this feature, we don’t recommend trying
this on a Chromebook that you rely on for work, school, or other daily
activities. You will encounter crashes, performance regressions, and
never-before-seen bugs - that’s part of the fun!

## Supported Devices

Because many games have high performance demands, we’ve focused our efforts thus
far on a set of devices where more games can run well. Currently, Steam can be
enabled in the Dev channel on configurations of these Chromebooks with Intel
Iris Xe Graphics, 11th Gen Core i5 or i7 processors, and at least 8GB of RAM:

*   [Acer Chromebook 514 (CB514-1W)](https://www.acer.com/ac/en/US/content/series/acerchromebook514cb5141w)
*   [Acer Chromebook 515 (CB515-1W)](https://www.acer.com/ac/en/US/content/series/acerchromebook515cb5151w)
*   [Acer Chromebook Spin 713 (CP713-3W)](https://www.acer.com/ac/en/US/content/series/acerchromebookspin713cp7133w)
*   [ASUS Chromebook Flip CX5 (CX5500)](https://www.asus.com/us/Laptops/For-Home/Chromebook/ASUS-Chromebook-Flip-CX5-CX5500/)
*   [ASUS Chromebook CX9 (CX9400)](https://www.asus.com/us/Laptops/For-Work/Chromebook/ASUS-Chromebook-CX9-CX9400-11th-Gen-Intel/)
*   [HP Pro c640 G2 Chromebook](https://www.hp.com/us-en/cloud-computing/chrome-enterprise/hp-pro-c640-chromebook.html)
*   [Lenovo 5i-14 Chromebook](https://www.lenovo.com/us/en/p/laptops/lenovo/lenovo-edu-chromebooks/5i-chromebook-gen-6-\(14-intel\)/wmd00000481)

Note: Configurations of these devices with an i3 CPU or 4GB of RAM are **not**
supported.

Additionally, there are two known issues that affect particular device
configurations:
*  Devices with 8GB of RAM may encounter issues in games that require 6GB of
RAM or more.
*  Devices with display resolutions greater than 1080p may encounter
performance and scaling issues.

We are actively investigating ways to improve these issues.

This list will be updated as new models and configurations are enabled.

## Instructions

Note: Dev channel is inherently less stable. We don’t recommend trying this on a
Chromebook that you rely on for work, school, or other daily activities. Before
you switch channels, backup your data.

1.  On your supported Chromebook,
    [switch to Dev channel.](https://www.google.com/support/chromeos/bin/answer.py?answer=1086915)
2.  After updating, navigate to chrome://flags. Set both #borealis-enabled and
    #exo-pointer-lock to Enabled.
3.  After restarting, open a crosh terminal with ctrl+alt+t.
4.  Type “insert_coin volteer-JOlkth573FBLGa” and hit enter.
5.  Follow the setup flow to install Steam.
6.  Log in with your Steam account and start playing!

We recommend trying games from the list below, as not all games currently work
well. Rest assured we are actively working to support as many titles as
possible.

If you encounter issues, first check the known issues list below, then
[file feedback](https://support.google.com/chromebook/answer/2982029) with a
description of your issue and “#steam” so we can triage it quickly.

## Known Issues

These are some issues you may encounter when using Steam on Chrome OS:

|Category      | Known Issue|
|------------- | -----------|
|Compatibility | |
| | [Easy Anti-Cheat and BattlEye do not yet work with Proton on Chrome OS](https://issuetracker.google.com/issues/225082536)|
| | [Proton titles using DX12 fail to start](https://issuetracker.google.com/issues/224849878)|
| | [Some Proton games have incorrect window placement, including offscreen](https://issuetracker.google.com/issues/224834525)|
| | [Some titles have occasional rendering artifacts and glitches](https://issuetracker.google.com/issues/224850235)|
|Performance   | |
| | [First few minutes of gameplay have poor performance for some titles](https://issuetracker.google.com/issues/224858407)|
| | ["Processing Vulkan shaders" occurs frequently and can take a long time](https://issuetracker.google.com/issues/224848059)|
| | [Some games work with 16GB RAM but not 8GB RAM](https://issuetracker.google.com/issues/225082848)|
| Display     | |
| | [External monitors are not supported and have unexpected behavior](https://issuetracker.google.com/issues/224859456)|
| | [4K and 2K displays have issues with performance and scaling](https://issuetracker.google.com/issues/224848902)|
| UI | |
| | [Some languages (Chinese, Japanese, Korean, and Thai) are missing fonts](https://issuetracker.google.com/issues/224859554)|
| | [Timezone for Steam and games is different from device](https://issuetracker.google.com/issues/224859460)|
| Input       | |
| | [Launcher key & keyboard shortcuts don't work when Steam/a game is focused](https://issuetracker.google.com/issues/224859546)|
| | [Plugging in a controller during gameplay may not be recognized](https://issuetracker.google.com/issues/224851350)|
| | [Gamepads that are not designated WWCB may not work correctly](https://issuetracker.google.com/issues/225083178)|
| Audio      | |
| | [Audio quality in Steam voice chat is poor](https://issuetracker.google.com/issues/224851318)|
| Storage     | |
| | [Games may fail to install citing a disk error](https://issuetracker.google.com/issues/225083197)|
| | [External storage is not yet supported](https://issuetracker.google.com/issues/225083207)|
| Power       | |
| | [Device will not sleep when Steam or a game is focused](https://issuetracker.google.com/issues/225083268)|
| | [Steam and games sometimes freeze when device is asleep](https://issuetracker.google.com/issues/224848735)|

Please report any other issues you see via the Send Feedback dialog with tag
**“#steam”**.

## Game List

These are some games that we’ve tried ourselves and think you might enjoy. Some
will require enabling Steam Play in order to install (see below), others may
work better on models with higher specs (like 16GB RAM).

| Game                           | Tips and Known Issues                       |
| ------------------------------ | ------------------------------------------- |
| [Portal 2]                     |                                             |
| [Hades]                        | Select Vulkan version at launch             |
| [Age of Empires II: Definitive Edition]| |
| [World of Tanks Blitz]         |                                             |
| [The Elder Scrolls V: SSE]  | Set quality to medium or low. i7 recommended|
| [Vampire Survivors]            |                                          |
| [Team Fortress 2] | Disable multicore rendering (Options>Graphics>Advanced)|
| [Euro Truck Simulator 2]       |                                             |
| [Dead Cells]                   |                                             |
|[The Witcher 3: Wild Hunt]|Graphics & postprocessing to med/low. i7 required|
| [Celeste]                      |                                             |
| [Unturned]                     |                                             |
| [Half-Life 2]                  |                                             |
| [Stardew Valley]               |                                             |
| [PAYDAY 2]                     |                                             |
| [Terraria]                     |                                             |
| [Sid Meier's Civilization V]   |                                             |
| [Project Zomboid]              |                                             |
| [RimWorld]                     |                                             |
| [Left 4 Dead 2]                |                                             |
| [Stellaris]                    |                                             |
| [Bloons TD 6]                  |                                             |
| [Factorio]                     |                                             |
| [Divinity: Original Sin 2] | Set Graphics Quality Preset to Medium or lower|
| [Geometry Dash]                |                                             |
| [Human: Fall Flat]             | Set Advanced Video to Medium or lower       |
| [Overcooked! 2]                |                                             |
| [Hollow Knight]                |                                             |
| [Tabletop Simulator]           |                                             |
| [Killer Queen Black]           |                                             |
| [Slay the Spire]               |                                             |
| [Cuphead]                      |                                             |
| [TEKKEN 7]                     |                                             |
| [Loop Hero]                    |                                             |
| [Kerbal Space Program]         |                                             |
| [Grim Dawn]                    |                                             |
| [RISK: Global Domination]      |                                             |
| [Northgard]                    |                                             |
| [Fishing Planet]               |                                             |
| [Don't Starve Together]        |                                             |
| [Farm Together]                |                                             |
| [Darkest Dungeon®]             |                                             |
| [The Jackbox Party Pack 8]     | Other party packs work well too!            |
| [Baba Is You]                  |                                             |
| [A Short Hike]                 |                                             |
| [Fallout 4]       | Set graphics quality to medium or lower. i7 recommended|
| [Return of the Obra Dinn]      |                                             |
| [Disco Elysium]                | Long load time on initial launch            |
| [Escape Simulator]             |                                             |
| [Untitled Goose Game]          |                                             |

Regardless of what games you play, please send feedback about how well it worked
and any issues you encountered during gameplay using the post-game survey.

## Steam Play

Chrome OS will typically run the Linux version of a game if it exists. Steam
Play allows you to run additional titles by leveraging a compatibility tool
called Proton.

To enable Steam Play for a particular title:

*   On the game’s library listing, click the settings cog
*   Select “Properties”
*   Select “Compatibility”
*   Check “Force the use of a specific Steam Play compatibility tool”
*   Select a version. We recommend Proton Experimental

To enable Steam Play for all relevant titles:

*   In the top left corner of the Steam client, select “Steam”
*   Select “Settings”
*   Select “Steam Play”
*   Check “Enable Steam Play for all other titles”
*   Select a version. We recommend Proton Experimental

## FAQ

Q: When will Steam come to Beta or Stable channel?

A: We don’t have a specific date to commit to. We’ll expand availability
when we feel the product is ready.

-------------------------------------------------------------------------------
Q: When will Steam be available on my Chromebook?

A: Some Chromebooks lack the necessary hardware to provide a quality
experience for Steam games, and thus are unlikely to be supported. As new,
compatible devices come out, we will update the supported device list.

-------------------------------------------------------------------------------
Q: Can I play my favorite game on a Chromebook now?

A: If your game is on the list above, it’s likely to run based on our
testing. Otherwise, the only way to find out is to give it a try! Please
send feedback about what you find.

-------------------------------------------------------------------------------
Q: Can I play games from other game stores?

A: Chrome OS also supports Android Games from the Play Store. Outside Steam,
other PC game stores are not supported.

-------------------------------------------------------------------------------


## Game Developers

We’d love to work with you to ensure your games run great on Chrome OS. If your
game is already on Steam and runs on Linux (with or without Proton), you can try
it using the instructions above. If you encounter Chrome OS specific bugs with
your game, have particular workflows you’d like supported, or are otherwise
interested in working with us, please reach out to borealis-game-dev@google.com.

[Portal 2]:https://store.steampowered.com/app/620
[Hades]:https://store.steampowered.com/app/1145360
[Age of Empires II: Definitive Edition]:
https://store.steampowered.com/app/813780
[World of Tanks Blitz]:https://store.steampowered.com/app/444200
[The Elder Scrolls V: SSE]:https://store.steampowered.com/app/489830
[Vampire Survivors]:https://store.steampowered.com/app/1794680
[Team Fortress 2]:https://store.steampowered.com/app/440
[Euro Truck Simulator 2]:https://store.steampowered.com/app/227300
[Dead Cells]:https://store.steampowered.com/app/588650
[The Witcher 3: Wild Hunt]:
https://store.steampowered.com/app/292030/The_Witcher_3_Wild_Hunt/
[Celeste]:https://store.steampowered.com/app/504230
[Unturned]:https://store.steampowered.com/app/304930
[Half-Life 2]:https://store.steampowered.com/app/220
[Stardew Valley]:https://store.steampowered.com/app/413150
[PAYDAY 2]:https://store.steampowered.com/app/218620
[Terraria]:https://store.steampowered.com/app/105600
[Sid Meier's Civilization V]:https://store.steampowered.com/app/8930
[Project Zomboid]:https://store.steampowered.com/app/108600
[RimWorld]:https://store.steampowered.com/app/294100
[Left 4 Dead 2]:https://store.steampowered.com/app/550
[Stellaris]:https://store.steampowered.com/app/281990
[Bloons TD 6]:https://store.steampowered.com/app/960090
[Factorio]:https://store.steampowered.com/app/427520
[Divinity: Original Sin 2]:https://store.steampowered.com/app/435150
[Geometry Dash]:https://store.steampowered.com/app/322170
[Human: Fall Flat]:https://store.steampowered.com/app/477160
[Overcooked! 2]:https://store.steampowered.com/app/728880
[Hollow Knight]:https://store.steampowered.com/app/367520
[Tabletop Simulator]:https://store.steampowered.com/app/286160
[Killer Queen Black]:
https://store.steampowered.com/app/663670/Killer_Queen_Black/
[Slay the Spire]:https://store.steampowered.com/app/646570
[Cuphead]:https://store.steampowered.com/app/268910
[TEKKEN 7]:https://store.steampowered.com/app/389730
[Loop Hero]:https://store.steampowered.com/app/1282730
[Kerbal Space Program]:https://store.steampowered.com/app/220200
[Grim Dawn]:https://store.steampowered.com/app/219990
[RISK: Global Domination]:https://store.steampowered.com/app/1128810
[Northgard]:https://store.steampowered.com/app/466560
[Fishing Planet]:https://store.steampowered.com/app/380600
[Don't Starve Together]:https://store.steampowered.com/app/322330
[Farm Together]:https://store.steampowered.com/app/673950
[Darkest Dungeon®]:https://store.steampowered.com/app/262060
[The Jackbox Party Pack 8]:https://store.steampowered.com/app/1552350
[Baba Is You]:https://store.steampowered.com/app/736260
[A Short Hike]:https://store.steampowered.com/app/1055540
[Fallout 4]:https://store.steampowered.com/app/377160/Fallout_4/
[Return of the Obra Dinn]:https://store.steampowered.com/app/653530
[Disco Elysium]:https://store.steampowered.com/app/632470
[Escape Simulator]:https://store.steampowered.com/app/1435790/Escape_Simulator/
[Untitled Goose Game]:https://store.steampowered.com/app/837470
