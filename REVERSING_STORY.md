# Introduction

First, a disclaimer, this is the first time I'm doing this kind of exercise, so the process described here is probably not the best approach.

## Motivation

I've always wanted to play around World of Warships game content for various reasons, from extracting things like armor layouts or in game parameters to the 3D models themselves.

There is already a tool that does that: [wows-unpack](https://forum.worldofwarships.eu/topic/113847-all-wows-unpack-tool-unpack-game-client-resources/)

But being an pro-OSS and working under linux, this tool doesn't suite me well.

It also doesn't fit my needs regarding having something that could be integrated into other programs easily

I also wanted to do as an intellectual exercise of reverse engineering.

## Goals

My goals are:

* Reverse the format, even partially (but enough to extract the data)* Document the format specification so other people could leverage it, and improve upon my implementation
* Create a CLI tool with Linux/Unix as its main target
* Create a library which can be reused (maybe through bindings) in other pieces of software.

# Reverse process

## First steps

I started this reverse process a while back, around 2 years ago, but lost interest. Consequently I only have very fuzzy memories of the initial steps I took, and the original findings.

Anyway, the first step was simply to look at the game files and see were the bulk of the data was:

```shell
kakwa@linux Games/World of Warships » ls
[...] api-ms-win-crt-runtime-l1-1-0.dll  concrt140.dll             msvcp140_codecvt_ids.dll  Reports               vcruntime140_1.dll
[...] api-ms-win-crt-stdio-l1-1-0.dll    crashes                   msvcp140.dll              res_packages          vcruntime140.dll
[...] api-ms-win-crt-string-l1-1-0.dll   currentrealm.txt          patche                    res_unpack            Wallpapers
[...] api-ms-win-crt-time-l1-1-0.dll     GameCheck                 placeholder.txt           screenshot            WorldOfWarships.exe
[...] api-ms-win-crt-utility-l1-1-0.dll  lib                       platform64.dll            steam_api64.dll       wowsunpack.exe
[...] app_type.xml                       mods_preferences.xml      preferences.xml           steam_autocloud.vdf   WOWSUnpackTool.cfg
[...] Aslain_Modpack                     msvcp140_1.dll            profile                   stuff                 WOWSUnpackTool.exe
[...] Aslains_WoWs_Logs_Archiver.exe     msvcp140_2.dll            readme.rtf                ucrtbase.dll
[...] bin                                msvcp140_atomic_wait.dll  replays                   user_preferences.xml

kakwa@linux Games/World of Warships » du -hd 1 | sort -h
4.0K	./Reports
12K	./patche
2.8M	./Wallpapers
4.2M	./GameCheck
4.4M	./screenshot
49M	./replays
55M	./lib
464M	./profile
593M	./crashes
1.2G	./bin
62G	./res_packages
73G	.
```

So here, the bulk of the data is in the `res_packages/` directory. Lets take a look:

```shell
kakwa@linux Games/World of Warships » ls res_packages                                                                                                                                                                                                                                                                 130 ↵
basecontent_0001.pkg           spaces_dock_dry_0001.pkg           spaces_greece_0001.pkg               spaces_sea_hope_0001.pkg              vehicles_level10_spain_0001.pkg      vehicles_level4_0001.pkg             vehicles_level6_panasia_0001.pkg     vehicles_level8_panamerica_0001.pkg
camouflage_0001.pkg            spaces_dock_dunkirk_0001.pkg       spaces_honey_0001.pkg                spaces_shards_0001.pkg                vehicles_level10_uk_0001.pkg         vehicles_level4_ger_0001.pkg         vehicles_level6_premium_0001.pkg     vehicles_level8_panasia_0001.pkg
[...]
spaces_dock_1_april_0001.pkg   spaces_faroe_0001.pkg              spaces_ridge_0001.pkg                vehicles_level10_panamerica_0001.pkg  vehicles_level3_panasia_0001.pkg     vehicles_level6_jap_0001.pkg         vehicles_level8_it_0001.pkg          z_vehicles_events_0001.pkg
spaces_dock_azurlane_0001.pkg  spaces_fault_line_0001.pkg         spaces_ring_0001.pkg                 vehicles_level10_panasia_0001.pkg     vehicles_level3_uk_0001.pkg          vehicles_level6_ned_0001.pkg         vehicles_level8_jap_0001.pkg
spaces_dock_dragon_0001.pkg    spaces_gold_harbor_0001.pkg        spaces_rotterdam_0001.pkg            vehicles_level10_ru_0001.pkg          vehicles_level3_usa_0001.pkg         vehicles_level6_panamerica_0001.pkg  vehicles_level8_ned_0001.pkg
```

Then use `file` to see if what type of files we are dealing with:
```shell
kakwa@linux Games/World of Warships » cd res_packages 
kakwa@linux World of Warships/res_packages » file *
[...]
spaces_dock_ny_0001.pkg:              data
spaces_dock_ocean_0001.pkg:           data
spaces_dock_prem_0001.pkg:            Microsoft DirectDraw Surface (DDS): 4 x 4, compressed using DX10
spaces_dock_rio_0001.pkg:             data
spaces_dock_spb_0001.pkg:             data
spaces_dock_table_0001.pkg:           data
spaces_dock_twitch_0001.pkg:          data
spaces_estuary_0001.pkg:              data
spaces_exterior_0001.pkg:             data
spaces_faroe_0001.pkg:                OpenPGP Secret Key
spaces_fault_line_0001.pkg:           data
spaces_gold_harbor_0001.pkg:          data
spaces_greece_0001.pkg:               data
spaces_honey_0001.pkg:                data
spaces_ice_islands_0001.pkg:          data
spaces_islands_0001.pkg:              data
spaces_labyrinth_0001.pkg:            data
spaces_lepve_0001.pkg:                data
spaces_military_navigation_0001.pkg:  data
spaces_naval_base_0001.pkg:           DOS executable (COM), maybe with interrupt 22h, start instruction 0x8cbc075c 534dd337
spaces_naval_defense_0001.pkg:        data
[...]
```

So mostly `data` i.e. unknown format, and looking at the files which are not `data`, they are in fact most likely false positives. So we are dealing with a custom format.

Next, lets try to see if we have some clear text strings in the files:

```shell
kakwa@linux World of Warships/res_packages » strings *
YyHIKzR
+!?<
m:C-
h4.1
~s3o
]2bm
]$ }
O=z$
<27P
=C=k]
dQz{4
$Zm|
$ZOc
%gR:-
kIw.
gd4$
`?Z+
Q}>Q
%+&C7
cQO{
;uNo
_ab1?#
UUvP&
7`==
E;wo`
[}Z6T
;mZu
?iGvM
>+#{
GimR\	|
2(?O(
Iq#G
x~8_S
flO~f
'nV&
<4n5
r>Zs%
6?Iw
KqM&u
^C
```

Nada. So we are dealing with a completely binary format.

Next, lets try to compress a file:

```shell
kakwa@linux World of Warships/res_packages » ls -l vehicles_level4_usa_0001.pkg                                                                                                                                                                                                                                       130 ↵
-rwxr-xr-x 1 kakwa kakwa 15356139 Jan 17 19:01 vehicles_level4_usa_0001.pkg

gzip vehicles_level4_usa_0001.pkg

ls -l vehicles_level4_usa_0001.pkg.gz 
-rwxr-xr-x 1 kakwa kakwa 15332196 Jan 17 19:01 vehicles_level4_usa_0001.pkg.gz
```

Ok, barely any change in size, which means the data is probably compressed (not a big surprise there since a lot of formats such as images are compressed).

Then, the process is a little fuzzy in my memory. But if I recall correctly, I did the following:

```shell
kakwa@linux World of Warships/res_packages » hexdump -C vehicles_level4_usa_0001.pkg | less
00000000  95 58 7f 50 9b e7 7d 7f  f4 2a 24 e2 55 64 7c bb  |.X.P..}..*$.Ud|.|
00000010  a4 be 5b 77 8d e6 45 0e  08 83 ba 5d b1 b7 b8 35  |..[w..E....]...5|
00000020  4a 2f e9 71 d9 3f c4 e5  45 2a 05 a1 92 eb 9d 2a  |J/.q.?..E*.....*|
00000030  77 41 73 43 ab c3 31 bc  c8 97 3b 41 c2 d0 f5 82  |wAsC..1...;A....|
00000040  fd 2e 69 e3 77 22 84 97  57 01 d1 b4 04 82 8d 90  |..i.w"..W.......|
00000050  f1 fe 68 bc 76 f3 ed ac  c0 75 ae 51 c9 b9 21 72  |..h.v....u.Q..!r|
00000060  1d 58 36 05 59 46 7a f7  fd 3c 90 6d d7 6d 7f 4c  |.X6.YFz..<.m.m.L|
00000070  77 f8 e3 e7 f7 f7 c7 e7  f9 7e bf cf fb e4 93 5f  |w........~....._|
[...]
```

I hexdumped one of the file, looking for some pattern that would help me determine the type of compression used. I was looking for things like padding patterns. I'm not sure how, but I finally determined the compression used was `DEFLATE` (RFC 1951) (I vaguely remember `7f f0` being a marker, but I might be very well mistaken, I might have just brute forced trying to uncompress using various tools). 

I ended-up creating [this tool](https://github.com/kakwa/brute-force-deflate) which tries to brute force deflate all the sections of the file, and sure enough, I was able to extract some interesting files:

```shell
kakwa@linux World of Warships/res_packages » bf-deflate -i system_data_0001.pkg -o systemout                                                                                                                                                                                                                          127 ↵
kakwa@linux World of Warships/res_packages » file systemout/* | tail
systemout/000A15D8ED-000A15D8F3: ISO-8859 text, with no line terminators
systemout/000A15D8F7-000A15EF9A: XML 1.0 document, ASCII text
systemout/000A15EF9E-000A15EFA7: ISO-8859 text, with CR line terminators
systemout/000A15EFAA-000A15F919: XML 1.0 document, ASCII text
systemout/000A15F929-000A162634: exported SGML document, Unicode text, UTF-8 text, with CRLF line terminators
systemout/000A162644-000A165D7C: ASCII text, with CRLF line terminators
systemout/000A165D8C-000A16C774: ASCII text, with CRLF line terminators
systemout/000A16C784-000A16D41A: exported SGML document, ASCII text, with CRLF line terminators
systemout/000A16D41E-000A16D426: data
systemout/bf-Xe4fzss:            empty

kakwa@linux World of Warships/res_packages » head systemout/000A15EFAA-000A15F919 
<?xml version="1.0" encoding="UTF-8" standalone="no"?>

<root>
    <Implements>
        <Interface>VisionOwner</Interface>
        <Interface>AtbaOwner</Interface>
        <Interface>AirDefenceOwner</Interface>
        <Interface>BattleLogicEntityOwner</Interface>
        <Interface>DamageDealerOwner</Interface>
        <Interface>DebugDrawEntity</Interface>
```

Okay, we actually are able to extract actual files! But without the names, it's not that interesting.

Note that I chose to name the files I managed to extract with the (approximate) corresponding start and end offsets of what my tool managed to uncompress (ex: `000A165D8C-000A16C774`, start offset is `000A165D8C`, end is `000A16C774`). This makes it simpler to correlate each extracted files to section in the original file.

But I then lost interest, and didn't follow-up for two years. If I recall correctly I remember being discouraged by not being able to exploit the few 3D models/textures I managed to extract, so I left it as is for the time being.

## Step 2

TODO
