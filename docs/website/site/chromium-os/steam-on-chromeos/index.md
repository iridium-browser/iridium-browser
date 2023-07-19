# Steam on ChromeOS Beta

Beginning with **Chrome OS 108.0.5359.24**, the Beta channel will include an early, 
beta-quality version of Steam on ChromeOS for a set of [recent Chromebooks](#supported-devices). 
If you have one of these Chromebooks and decide to give it a try, please send your feedback, 
we are in active development and want to incorporate as much user input as possible.

## What do we mean by beta?

Major issues affecting the whole system should be uncommon, 
and hundreds of Steam games are now playable on a wider variety of Chromebooks. 
However, many other games do not run or run poorly, 
parts of the user experience are not release-quality, 
and some OS-level features are still in development (see [Known Issues](#known-issues)).

## Supported Devices

Because many games have high performance demands, we’ve focused our efforts thus
far on a set of devices where more games can run well. 
Currently, Steam can be enabled in the beta channel on configurations of these Chromebooks 
with Core i3/Ryzen 3 or higher CPUs and at least 8GB of RAM:

*   [Acer Chromebook 514 (CB514-1W)](https://www.acer.com/ac/en/US/content/series/acerchromebook514cb5141w)
*   [Acer Chromebook 515 (CB515-1W)](https://www.acer.com/ac/en/US/content/series/acerchromebook515cb5151w)
*   [Acer Chromebook 516 GE](https://www.acer.com/us-en/chromebooks/acer-chromebook-516-ge-cbg516-1h)
*   [Acer Chromebook Spin 514 (CP514-3H, CP514-3HH, CP514-3WH)](https://www.acer.com/us-en/chromebooks/acer-chromebook-spin-514-cp514-3h-cp514-3hh-cp514-3wh)
*   [Acer Chromebook Spin 713 (CP713-3W)](https://www.acer.com/ac/en/US/content/series/acerchromebookspin713cp7133w)
*   [Acer Chromebook Spin 714 (CP714-1WN)](https://www.acer.com/us-en/chromebooks/acer-chromebook-enterprise-spin-714-cp714-1wn)
*   [Acer Chromebook Vero 514](https://www.acer.com/us-en/chromebooks/acer_chromebook_vero_514_cbv514-1h_cbv514-1ht/index.html)
*   [ASUS Chromebook CX9 (CX9400)](https://www.asus.com/us/Laptops/For-Work/Chromebook/ASUS-Chromebook-CX9-CX9400-11th-Gen-Intel/)
*   [ASUS Chromebook Flip CX5 (CX5500)](https://www.asus.com/us/Laptops/For-Home/Chromebook/ASUS-Chromebook-Flip-CX5-CX5500/)
*   [ASUS Chromebook Flip CX5 (CX5601)](https://www.asus.com/us/Laptops/For-Home/Chromebook/ASUS-Chromebook-Flip-CX5-CX5601-12th-Gen-Intel/)
*   [ASUS Chromebook Vibe CX55 Flip](https://www.asus.com/Laptops/For-Home/Chromebook/ASUS-Chromebook-Vibe-CX55-Flip-CX5501-11th-Gen-Intel/)
*   [Framework Laptop Chromebook Edition](https://frame.work/products/laptop-chromebook-12-gen-intel)
*   [HP Elite c640 14 inch G3 Chromebook](https://www.hp.com/us-en/shop/pdp/hp-elite-c640-14-g3-chromebook-customizable-5z8l4av-mb)
*   [HP Elite c645 G2 Chromebook](https://www.hp.com/us-en/shop/pdp/hp-elite-c645-14-g2-chromebook-customizable-5z9h1av-mb)
*   [HP Elite Dragonfly Chromebook](https://www.hp.com/us-en/shop/pdp/hp-elite-dragonfly-135-chromebook-enterprise-pc-customizable-5b955av-mb)
*   [HP Pro c640 G2 Chromebook](https://www.hp.com/us-en/cloud-computing/chrome-enterprise/hp-pro-c640-chromebook.html)
*   [IdeaPad Gaming Chromebook 16](https://www.google.com/chromebook/discover/pdp-ideapad-gaming-chromebook-16/sku-ideapad-gaming-chromebook-16-8gb-128gb/)
*   [Lenovo 5i-14 Chromebook](https://www.lenovo.com/us/en/p/laptops/lenovo/lenovo-edu-chromebooks/5i-chromebook-gen-6-%2814-intel%29/wmd00000481)
*   [Lenovo Flex 5i Chromebook 14](https://www.lenovo.com/us/en/p/laptops/ideapad/ideapad-flex-series/ideapad-flex-5i-chromebook-gen-7-%2814-inch-intel%29/len101i0035)
*   [Lenovo ThinkPad C14](https://www.lenovo.com/us/en/p/laptops/thinkpad/thinkpadc/thinkpad-c14-chromebook-enterprise-%2814-inch-intel%29/len101t0022)

This list will be updated as new models and configurations are enabled.

## Instructions

1.  On your supported Chromebook,
    [switch to Beta channel.](https://www.google.com/support/chromeos/bin/answer.py?answer=1086915)
2.  After updating, navigate to chrome://flags. Set #borealis-enabled to Enabled.
3.  After restarting, open the ChromeOS launcher, search for Steam, and select the top result.
4.  Follow the setup flow to install Steam.
5.  Log in with your Steam account and start playing!

If for whatever reason searching for Steam in the launcher doesn't work, 
you can open a crosh terminal with ctrl+alt+t and enter `insert_coin`.

We recommend trying games from the [list below](#game-list), as not all games currently work
well. Rest assured we are actively working to support as many titles as
possible.

If you encounter issues, first check the known issues list below, then
[file feedback](https://support.google.com/chromebook/answer/2982029) with a
description of your issue and “#steam” so we can triage it quickly.

## Known Issues

These are some issues you may encounter when using Steam on Chrome OS:

|Category      | Known Issue| Workaround |
|------------- | -----------|  -----------|
|Compatibility | | |
| | [Easy Anti-Cheat and BattlEye do not yet work with Proton on Chrome OS](https://issuetracker.google.com/issues/225082536)| |
| | [Some Proton games have incorrect window placement, including offscreen](https://issuetracker.google.com/issues/224834525)| Try pressing fullscreen key |
|Performance   | | |
| | [First few minutes of gameplay have poor performance for some titles](https://issuetracker.google.com/issues/224858407)| |
| | ["Processing Vulkan shaders" occurs frequently and can take a long time](https://issuetracker.google.com/issues/224848059)| Enable “Allow background processing of Vulkan shaders” in Steam settings. Will impact battery life. |
| | [Some games work with 16GB RAM but not 8GB RAM](https://issuetracker.google.com/issues/225082848)| |
| Display     | | |
| | [External monitors are not supported and have unexpected behavior](https://issuetracker.google.com/issues/224859456)| |
| | [Client UI scaling is not ideal on high DPI displays](https://issuetracker.google.com/issues/257077318) | Enable #borealis-force-double-scale in chrome://flags |
| Input       | | |
| | [Gamepads that are not designated WWCB may not work correctly](https://issuetracker.google.com/issues/225083178)| |
| Audio      | | |
| | [Audio quality in Steam voice chat is poor](https://issuetracker.google.com/issues/224851318)| |
| Storage     | | |
| | [External storage is not yet supported](https://issuetracker.google.com/issues/225083207)| |
| Power       | | |
| | [Device will not sleep when Steam or a game is focused](https://issuetracker.google.com/issues/225083268)|
| | [Steam and games sometimes freeze when device is asleep](https://issuetracker.google.com/issues/224848735)|

Please report any other issues you see via the Send Feedback dialog with tag
**“#steam”**.

## Game List

These are some games that we’ve tried ourselves and think you might enjoy. Some
will require enabling Steam Play in order to install ([see below](#steam-play)), others may
work better on models with higher specs (like 16GB RAM).

| Game                                                                                      | Tips and Known Issues                                       |
| ----------------------------------------------------------------------------------------- | ----------------------------------------------------------- |
| [A Short Hike](https://store.steampowered.com/app/1055540)                                |                                                             |
| [Age of Empires II: Definitive Edition](https://store.steampowered.com/app/813780)        |                                                             |
| [Age of Mythology: Extended Edition](https://store.steampowered.com/app/266840)           |                                                             |
| [ASTRONEER](https://store.steampowered.com/app/361420)                                    |                                                             |
| [Baba Is You](https://store.steampowered.com/app/736260)                                  |                                                             |
| [Besiege](https://store.steampowered.com/app/346010)                                      |                                                             |
| [Bloons TD 6](https://store.steampowered.com/app/960090)                                  |                                                             |
| [Bloons TD Battles 2](https://store.steampowered.com/app/1276390)                         |                                                             |
| [CARRION](https://store.steampowered.com/app/953490)                                      |                                                             |
| [Celeste](https://store.steampowered.com/app/504230)                                      |                                                             |
| [Core Keeper](https://store.steampowered.com/app/1621690)                                 |                                                             |
| [Cult of the Lamb](https://store.steampowered.com/app/1313140)                            |                                                             |
| [Cultist Simulator](https://store.steampowered.com/app/718670)                            |                                                             |
| [Cuphead](https://store.steampowered.com/app/268910)                                      |                                                             |
| [DARK SOULS™: REMASTERED](https://store.steampowered.com/app/570940)                      |                                                             |
| [Darkest Dungeon®](https://store.steampowered.com/app/262060)                             |                                                             |
| [Dead Cells](https://store.steampowered.com/app/588650)                                   |                                                             |
| [Deus Ex: Human Revolution - Director's Cut](https://store.steampowered.com/app/238010)   |                                                             |
| [Dicey Dungeons](https://store.steampowered.com/app/861540)                               |                                                             |
| [Disco Elysium](https://store.steampowered.com/app/632470)                                | Long load time on initial launch                            |
| [Dishonored](https://store.steampowered.com/app/205100)                                   |                                                             |
| [Disney Dreamlight Valley](https://store.steampowered.com/app/1401590)                    |                                                             |
| [Divinity: Original Sin 2](https://store.steampowered.com/app/435150)                     | Set Graphics Quality Preset to Medium or lower              |
| [Dome Keeper](https://store.steampowered.com/app/1637320)                                 |                                                             |
| [Don't Starve Together](https://store.steampowered.com/app/322330)                        |                                                             |
| [DOOM](https://store.steampowered.com/app/379720)                                         |                                                             |
| [Dorfromantik](https://store.steampowered.com/app/1455840)                                |                                                             |
| [Enter the Gungeon](https://store.steampowered.com/app/311690)                            |                                                             |
| [Escape Simulator](https://store.steampowered.com/app/1435790)                            |                                                             |
| [Euro Truck Simulator 2](https://store.steampowered.com/app/227300)                       |                                                             |
| [Factorio](https://store.steampowered.com/app/427520)                                     |                                                             |
| [Fallout 4](https://store.steampowered.com/app/377160)                                    | Set graphics quality to medium or lower. 16GB recommended   |
| [Farm Together](https://store.steampowered.com/app/673950)                                |                                                             |
| [Fishing Planet](https://store.steampowered.com/app/380600)                               |                                                             |
| [Football Manager 2022](https://store.steampowered.com/app/1569040)                       |                                                             |
| [For The King](https://store.steampowered.com/app/527230)                                 |                                                             |
| [Gang Beasts](https://store.steampowered.com/app/285900)                                  |                                                             |
| [Geometry Dash](https://store.steampowered.com/app/322170)                                |                                                             |
| [Grim Dawn](https://store.steampowered.com/app/219990)                                    |                                                             |
| [Gunfire Reborn](https://store.steampowered.com/app/1217060)                              |                                                             |
| [Hades](https://store.steampowered.com/app/1145360)                                       | Select default version at launch                            |
| [Half-Life 2](https://store.steampowered.com/app/220)                                     |                                                             |
| [Hearts of Iron IV](https://store.steampowered.com/app/394360)                            |                                                             |
| [Hollow Knight](https://store.steampowered.com/app/367520)                                |                                                             |
| [Human: Fall Flat](https://store.steampowered.com/app/477160)                             | Set Advanced Video to Medium or lower                       |
| [Inscryption](https://store.steampowered.com/app/1092790)                                 |                                                             |
| [Into the Breach](https://store.steampowered.com/app/590380)                              |                                                             |
| [Katamari Damacy REROLL](https://store.steampowered.com/app/848350)                       |                                                             |
| [Kerbal Space Program](https://store.steampowered.com/app/220200)                         |                                                             |
| [Killer Queen Black](https://store.steampowered.com/app/663670)                           |                                                             |
| [Left 4 Dead 2](https://store.steampowered.com/app/550)                                   |                                                             |
| [Loop Hero](https://store.steampowered.com/app/1282730)                                   |                                                             |
| [Mini Metro](https://store.steampowered.com/app/287980)                                   |                                                             |
| [Mirror's Edge](https://store.steampowered.com/app/17410)                                 |                                                             |
| [Monster Train](https://store.steampowered.com/app/1102190)                               |                                                             |
| [Muck](https://store.steampowered.com/app/1625450)                                        |                                                             |
| [Northgard](https://store.steampowered.com/app/466560)                                    |                                                             |
| [OCTOPATH TRAVELER](https://store.steampowered.com/app/921570)                            |                                                             |
| [Ori and the Blind Forest: Definitive Edition](https://store.steampowered.com/app/387290) |                                                             |
| [Overcooked! 2](https://store.steampowered.com/app/728880)                                |                                                             |
| [Oxygen Not Included](https://store.steampowered.com/app/457140)                          |                                                             |
| [Papers, Please](https://store.steampowered.com/app/239030)                               |                                                             |
| [PAYDAY 2](https://store.steampowered.com/app/218620)                                     |                                                             |
| [Portal 2](https://store.steampowered.com/app/620)                                        |                                                             |
| [Prey](https://store.steampowered.com/app/480490)                                         |                                                             |
| [Project Zomboid](https://store.steampowered.com/app/108600)                              |                                                             |
| [Return of the Obra Dinn](https://store.steampowered.com/app/653530)                      |                                                             |
| [RimWorld](https://store.steampowered.com/app/294100)                                     |                                                             |
| [RISK: Global Domination](https://store.steampowered.com/app/1128810)                     |                                                             |
| [Shatter Remastered Deluxe](https://store.steampowered.com/app/1937230)                   |                                                             |
| [Shop Titans](https://store.steampowered.com/app/1258080)                                 |                                                             |
| [Sid Meier's Civilization V](https://store.steampowered.com/app/8930)                     |                                                             |
| [Slay the Spire](https://store.steampowered.com/app/646570)                               |                                                             |
| [Slime Rancher](https://store.steampowered.com/app/433340)                                |                                                             |
| [STAR WARS™: The Old Republic™](https://store.steampowered.com/app/1286830)               |                                                             |
| [Stardew Valley](https://store.steampowered.com/app/413150)                               |                                                             |
| [Stellaris](https://store.steampowered.com/app/281990)                                    |                                                             |
| [Stormworks: Build and Rescue](https://store.steampowered.com/app/573090)                 |                                                             |
| [Stumble Guys](https://store.steampowered.com/app/1677740)                                |                                                             |
| [Subnautica](https://store.steampowered.com/app/264710)                                   |                                                             |
| [SUPERHOT](https://store.steampowered.com/app/322500)                                     |                                                             |
| [Tabletop Simulator](https://store.steampowered.com/app/286160)                           |                                                             |
| [Team Fortress 2](https://store.steampowered.com/app/440)                                 | Disable multicore rendering (Options > Graphics > Advanced) |
| [Terraria](https://store.steampowered.com/app/105600)                                     |                                                             |
| [Tetris® Effect: Connected](https://store.steampowered.com/app/1003590)                   |                                                             |
| [The Battle of Polytopia](https://store.steampowered.com/app/874390)                      |                                                             |
| [The Elder Scrolls V: Skyrim Special Edition](https://store.steampowered.com/app/489830)  | Set graphics quality to medium or lower. 16GB recommended.  |
| [The Jackbox Party Pack 8](https://store.steampowered.com/app/1552350)                    | Other party packs work well too!                            |
| [The Witcher 3: Wild Hunt](https://store.steampowered.com/app/292030)                     | Set graphics and postprocessing to low. 16GB required.      |
| [Tomb Raider](https://store.steampowered.com/app/203160)                                  | Use Proton 7.0-4                                            |
| [Totally Accurate Battle Simulator](https://store.steampowered.com/app/508440)            |                                                             |
| [TUNIC](https://store.steampowered.com/app/553420)                                        |                                                             |
| [Two Point Hospital](https://store.steampowered.com/app/535930)                           |                                                             |
| [Untitled Goose Game](https://store.steampowered.com/app/837470)                          |                                                             |
| [Unturned](https://store.steampowered.com/app/304930)                                     |                                                             |
| [Vampire Survivors](https://store.steampowered.com/app/1794680)                           | May need to use public beta.                                |
| [Wingspan](https://store.steampowered.com/app/1054490)                                    |                                                             |
| [Wolfenstein: The New Order](https://store.steampowered.com/app/201810)                   |                                                             |
| [World of Tanks Blitz](https://store.steampowered.com/app/444200)                         |                                                             |
| [Yu-Gi-Oh! Master Duel](https://store.steampowered.com/app/1449850)                       |                                                             |

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

## Optimizing Performance

Many games will attempt to automatically adjust their performance-related settings based on your device’s specs. 
However, you may want to manually adjust in-game settings to optimize performance further. 
Here are some common steps we recommend trying:

*   Reduce game resolution. This is particularly true on devices with very high resolution displays, like cloud gaming Chromebooks.
*   Close other windows. Other apps and websites may be using resources in the background. 
Try closing them to free up those resources for the game.
*   Disable v-sync. Users have reported that some games benefit significantly from disabling v-sync when running on Chromebooks. 
We’re looking into this, but in the meantime try disabling it if you’re having performance issues. 
Re-enable if graphics artifacts occur.
*   Reduce graphics presets. “Medium” or “Low” is generally a good setting to try.

In all cases, please note in your gameplay report what you tried and what the impact is to help us investigate.

## Export data

If there are files that you want to export from your Steam installation, you
can follow these steps:
1. Start Steam
2. Open a Crosh terminal (ctrl+alt+t)

In the Crosh terminal:

3. Run `vmc share borealis Downloads`
4. Run `vsh borealis`
5. Navigate to the files you want to export and copy them to
`/mnt/shared/MyFiles/Downloads`
    * e.g
`cp ~/.local/share/Terraria/Players/ /mnt/shared/MyFiles/Downloads -r`
6. You should now be able to see your exported files in your Downloads folder

note: if you're looking to copy game installations (not saves) you may want to
make use of [Steam's Backup feature.](https://help.steampowered.com/en/faqs/view/4593-5cb7-dc3c-64f0)
Make sure to select `/mnt/shared/MyFiles/Downloads` as the folder to use and
be careful managing the available disk space, as it may not be reported
accurately.

note: if you'd like to import files, you can follow a similar process, but,
instead of copying files to `/mnt/shared/MyFiles/Downloads`, you can instead
copy them from that directory.


## FAQ

Q: When will Steam come to Stable channel?

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
