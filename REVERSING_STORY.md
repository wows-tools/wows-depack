# Introduction

First, a disclaimer, this is the first time I'm doing this kind of exercise, so the process described here is far from ideal, and the tools used less than adequate.

Also, I'm writting this after various findings, so the process seems quite straight forward. In reallity, it was full of dead ends and flows of ideas (good and bad) that came-up while staring at hexdumps for hours.

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

# Reverse Engineering process

## Intial effort

### Looking at the Game files

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
kakwa@linux Games/World of Warships » ls res_packages                                                                                                                                                                                                                                                                 basecontent_0001.pkg           spaces_dock_dry_0001.pkg           spaces_greece_0001.pkg               spaces_sea_hope_0001.pkg              vehicles_level10_spain_0001.pkg      vehicles_level4_0001.pkg             vehicles_level6_panasia_0001.pkg     vehicles_level8_panamerica_0001.pkg
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
spaces_exterior_0001.pkg:             data
spaces_faroe_0001.pkg:                OpenPGP Secret Key
spaces_labyrinth_0001.pkg:            data
spaces_lepve_0001.pkg:                data
spaces_military_navigation_0001.pkg:  data
spaces_naval_base_0001.pkg:           DOS executable (COM), maybe with interrupt 22h, start instruction 0x8cbc075c 534dd337
spaces_naval_defense_0001.pkg:        data
[...]
```

So mostly `data` i.e. unknown format, and looking at the files which are not `data`, they are in fact most likely false positives. So we are dealing with a custom format.

### Investing the interesting files

Next, lets try to see if we have some clear text strings in the files using the `strings` utility:

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
nV&
<4n5
r>Zs%
6?Iw
KqM&u
[...]
```

Nada, that's just garbage. So we are dealing with a completely binary format.

Next, lets try to compress a file:

```shell
# Size before
kakwa@linux World of Warships/res_packages » ls -l vehicles_level4_usa_0001.pkg                                                                                                                                                                                                                                      
-rwxr-xr-x 1 kakwa kakwa 15356139 Jan 17 19:01 vehicles_level4_usa_0001.pkg

# Compress
kakwa@linux World of Warships/res_packages » gzip vehicles_level4_usa_0001.pkg

# Size After
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

I hexdumped one of the file, looking for some pattern that would help me determine the type of compression used. I was looking for things like padding or signatures repeating within the .pkg file. I'm not sure how, but I finally determined the compression used was `DEFLATE` (RFC 1951) (I vaguely remember `7f f0` being a marker, but I might be very well mistaken). In any case, [The Wikipedia page listing file signatures](https://en.wikipedia.org/wiki/List_of_file_signatures) is really useful, as well as Googling around candidate patterns.

I ended-up creating [this tool](https://github.com/kakwa/brute-force-deflate) which tries to brute force deflate all the sections of the file, and sure enough, I was able to extract some interesting files:

```shell
# Extracting stuff
kakwa@linux World of Warships/res_packages » bf-deflate -i system_data_0001.pkg -o systemout                                                                                 
# look the file types we just extracted
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

# Look if we indeed got what "file" says it is
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

Okay, we actually are able to extract actual files!

...But without the names, it's not that interesting.

Note that in my "brute-force deflate" tool, I chose to name the files I managed to extract with the (approximate) corresponding start and end offsets of what my tool managed to uncompress (ex: `000A165D8C-000A16C774`, start offset is `000A165D8C`, end is `000A16C774`). This makes it simpler to correlate each extracted files to section in the original file.

Going back to the reverse engineering, that was progress, but I then lost I interest, and didn't follow-up for two years.

If I recall correctly I remember being discouraged by not being able to exploit the few 3D models/textures I managed to extract, so I left it as is for the time being.

## Follow-up effort

### Renewed interest

Then, a few weeks ago, I 3D printed a [ship model from Thingiverse](https://www.thingiverse.com/thing:2479868) clearly imported from WoWs, and posted the result on [Reddit](https://www.reddit.com/r/WorldOfWarships/comments/10ozv6y/i_might_have_accidentally_created_t11_petro_3d/).

In one of the comment, someone asked if it was possible to export other game models, which sparked my interest once again. FYI, a [plugin](https://github.com/ShadowyBandit/.geometry-converter) to import WoWs .geometry (Big World game engin format) into Blender, but it doesn't look functional at the moment.

During the quick search, I also tried the Windows wows_unpack tool for the first time (I should really have started from there). This gave me some insights, in particular, the resource files having individual names and paths withing the .pkg files.

So, we are dealing with what is roughly a custom archive format (a bit like a zip file). This means we can probably expect the following things:

* a bunch of data blobs concatenated together, with the compressed data in these blobs.
* an index with the file paths and some metadata containing possibly things like file types, file ids, offsets pointing to the correct data blob, checksums, etc.

So let's take a look at the files again.

### Investigating the .pkg files

First, I copied over a few of the game files into a dedicated `res_unpack` directory to avoid breaking my install by accident.

Then, let's stare at more hexdumps:

```shell
kakwa@linux World of Warships/res_unpack » hexdump -C system_data_0001.pkg | less
[...]
000021e0  b7 df f2 d7 ff e4 4f bd  52 f6 39 be f5 4a 92 2f  |......O.R.9..J./|
000021f0  d9 8a f4 ff 01 00 00 00  00 bf 00 45 5c 6c 36 00  |...........E\l6.|
00002200  00 00 00 00 00 dd 97 cf  6e e2 30 10 c6 cf 54 ea  |........n.0...T.|
00002210  3b 44 79 00 20 09 09 20  41 a5 22 ca 9f 83 b7 08  |;Dy. .. A.".....|
00002220  38 ec cd 72 93 61 6b 6d  6c 47 ce a4 85 b7 5f 3b  |8..r.akmlG...._;|
00002230  25 0b 6d 29 d5 ae ba 12  9b 63 26 33 e3 df f7 79  |%.m).....c&3...y|
00002240  ec 28 03 26 b9 60 08 09  e1 79 9c 37 b7 22 bd b9  |.(.&.`...y.7."..|
00002250  be 6a 0c 84 79 72 24 13  30 74 c9 82 de 92 7e b7  |.j..yr$.0t....~.|
00002260  43 47 85 78 48 e1 01 80  8e 55 fc 93 da 42 d7 11  |CG.xH....U...B..|
00002270  5c ce 93 14 c6 85 66 c8  95 1c ba 51 b3 6d a2 6c  |\.....f....Q.m.l|
00002280  fb 3a ea 05 26 6c 3b 37  06 2f 0b 9a e8 be 3f 6a  |.:..&l;7./....?j|
00002290  26 f3 8d d2 a6 19 ee 32  13 e0 a6 72 9f db fa 9d  |&......2...r....|
000022a0  5c 52 b5 2c d6 69 be a8  7b c4 c7 25 c5 42 6b c0  |\R.,.i..{..%.Bk.|
000022b0  8f 20 7b a7 21 43 13 b6  eb da 7c 19 b3 8c c5 1c  |. {.!C....|.....|
000022c0  ad 37 87 94 a0 2a 3c fd  da 36 70 f2 47 9e 8d 81  |.7...*<..6p.G...|
000022d0  e1 e3 dd 66 03 31 0e dd  8c 69 e4 71 0a 79 6b 0d  |...f.1...i.q.yk.|
000022e0  29 64 4a 23 5d 57 a2 41  5b cf dd cf e4 75 43 7a  |)dJ#]W.A[....uCz|
000022f0  9f 21 17 45 4e 17 9a 8b  f3 5b 70 46 dd 3f dd 82  |.!.EN....[pF.?..|
00002300  de 1b c6 75 8d f6 60 4a  fc 5e 9f 12 f8 c1 50 2b  |...u..`J.^....P+|
00002310  79 71 f6 5b bc ce 01 af  4e ce 2f 49 d4 e9 d3 65  |yq.[....N./I...e|
00002320  79 b8 2f ce 77 0b 17 56  70 75 72 7d 44 fc b6 17  |y./.w..Vpur}D...|
00002330  d2 99 c2 a5 4a fe e6 c2  f7 be c0 f6 f7 b5 36 35  |....J.........65|
00002340  78 5f d7 18 40 a9 7b 9f  75 10 bf ca 40 83 51 a1  |x_..@.{.u...@.Q.|
00002350  ad 8a b7 06 38 09 a4 6c  b7 82 78 e8 fa 4d 2f 74  |....8..l..x..M/t|
00002360  1d a9 12 f8 56 76 98 2d  e8 e4 3b 25 e6 34 f1 2d  |....Vv.-..;%.4.-|
00002370  7d f1 d4 6d fd d1 64 94  06 46 95 81 75 1a 8d 09  |}..m..d..F..u...|
00002380  f1 83 80 4e cd 15 9f da  b1 38 37 1b dd d3 c2 3a  |...N.....87....:|
00002390  5f 30 1b 67 f1 c2 03 5e  9d 9c 9f 13 3f 6a d3 95  |_0.g...^....?j..|
000023a0  2a 64 f2 cc 9e 2e ef 36  b4 7c de 11 5f 9d bc 9f  |*d.....6.|.._...|
000023b0  92 a0 6d bc 47 a6 f3 58  03 13 17 37 f7 16 d0 3b  |..m.G..X...7...;|
000023c0  06 1c 31 44 f3 55 fa 2f  dd af 44 bf fe 2f f9 05  |..1D.U./..D../..|
000023d0  00 00 00 00 6d b9 de c1  ad 0c 00 00 00 00 00 00  |....m...........|
000023e0  7d cf cd 0d 80 20 0c 80  d1 b3 26 ce c2 02 84 8b  |}.... ....&.....|
000023f0  c6 01 dc 00 b5 fe 24 85  12 5a f6 57 8c 92 70 f1  |......$..Z.W..p.|
00002400  f8 b5 ef d0 ea 48 24 a6  6b 1b 0d de ce 08 ab 19  |.....H$.k.......|
00002410  2d 32 68 f5 e5 b3 42 70  e0 85 73 94 32 5b 4c 2c  |-2h...Bp..s.2[L,|
00002420  c9 f5 78 86 9b bf c3 4a  2c e4 82 65 fe 11 7c 9c  |..x....J,..e..|.|
00002430  61 20 c4 1f b2 27 3f 91  58 a1 98 11 57 aa 44 3e  |a ...'?.X...W.D>|
00002440  4d ab e7 97 0b 00 00 00  00 f1 d2 87 5a d2 00 00  |M...........Z...|
00002450  00 00 00 00 00 ad 9c db  6e db b8 16 86 af 67 03  |........n.....g.|
00002460  fb 1d 3c b9 2f 3a 3e e4  50 20 0d c0 48 8c ad 89  |..<./:>.P ..H...|
00002470  2c 69 28 c9 4e 7a 23 b8  89 3b 35 26 89 03 c7 99  |,i(.Nz#..;5&....|
00002480  ee be fd 26 75 32 29 2e  52 4b 72 2f 0a a4 16 fd  |...&u2).RKr/....|
00002490  7f bf 28 72 69 f1 e4 cb  d7 dd fa 6d bd bf fa ef  |..(ri......m....|
[...]
```

I noticed the 128 bits pattern `00 00 00 00 | xx xx xx xx | xx xx 00 00 | 00 00 00 00` repeating within the file (`|` used to cut every 32 bits).

The starts and end of these small sections line-up pretty well with the data sections I managed to extract:

```
0000000001-00000021F6
0000002206-00000023D1
00000023E1-0000002446
0000002456-00000031C8
```

We indeed have the first `00 00 [...]` pattern staring at offset 000021f4 and ending 00002205 which nearly matches 00000021F6 (end of first extracted file) and 0000002206 (start of second extracted file).
The next pattern starts at 000023d0 and ends at 000023df, which again lines-up roughly with 00000023D1 and 00000023E1. And so on for all the sections.

Note, my brute-force tool is most likely a bit buggy and probably adds a few +1 offsets here and there, also it is likely that the uncompressions overflows a bit beyond the actual compressed data. But it is good enough for purpose.

Looking at the end of the file, we have this pattern repeating one final time at the very end:

```
0a16d3c0  79 b0 e2 a0 8e e3 a8 3c  4b d5 d4 51 a0 7b b2 a1  |y......<K..Q.{..|
0a16d3d0  32 6b 36 bf fc ce 46 b6  1e d6 2d b8 94 98 ea 74  |2k6...F...-....t|
0a16d3e0  ac 57 92 19 a0 2f 7a c5  43 23 1e 46 0e 1d a8 6f  |.W.../z.C#.F...o|
0a16d3f0  9a fe f2 85 0e 6c 2f a4  8b 87 71 d4 5e 9a 8f d4  |.....l/...q.^...|
0a16d400  41 0f eb 85 aa b4 41 f7  ab af 86 39 6a d7 db af  |A.....A....9j...|
0a16d410  2a 24 8b 8f 8d 7f 6a f6  3f 00 00 00 00 6b ba c9  |*$....j.?....k..|
0a16d420  70 eb 6c 00 00 00 00 00  00                       |p.l......|
0a16d429
(END)
```

Further more, the first uncompressed block seems to start right at the begining of the file, so there is probably no header section.

### General format of the .pkg file

Looking at a few other `.pkg`, this pattern seems to be shared accross all files.

So we can deduce the format `.pkg` is a concatenated list of sections like the following:

```
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
|                                                                                             |
|                             Compressed Data (RFC 1951/Deflate)                              |
|                                                                                             |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
+====++====++====++====++====++====++====++====+====++====++====++====++====++====++====++====+
| 00 || 00 || 00 || 00 || XX || XX || XX || XX | XX || XX || 00 || 00 || 00 || 00 || 00 || 00 |
+====++====++====++====++====++====++====++====+====++====++====++====++====++====++====++====+
|<----- 0 padding ---->||<----------- some kind of ID --------------->||<---- 0 padding ----->|
```

Note that by this point, I'm making a lot of assumptions:

* I'm assuming that `0 padding` 32 bits blocks are actually padding, but they could be fields which happened to be set to 0 most of the time
* I'm a bit puzzled by the 16 bits  `00 00` at the end of `some kind of ID`
* I'm also not completely sure if the block containing the data is always compressed using Deflate
* I'm not even sure if this general file format is actually shared across all files.

But let's go forward, this format seems common enough to still yeild good results. Plus we can always go back and revisit this interpretation.

### What the ID might be

Let's take a look at some of these:

```
00 00 00 00 | bf 00 45 5c | 6c 36 00 00 | 00 00 00 00
00 00 00 00 | 6d b9 de c1 | ad 0c 00 00 | 00 00 00 00
00 00 00 00 | f1 d2 87 5a | d2 00 00 00 | 00 00 00 00
00 00 00 00 | 83 0a 72 88 | a3 5c 00 00 | 00 00 00 00
00 00 00 00 | 92 ab 31 63 | 91 2a 00 00 | 00 00 00 00
```

So looking at these "IDs", here are the things to note:

* they are rather random and high value, meaning they probably don't represent offsets.
* they look too short (~48 bits) for any [hash algorithm](https://en.wikipedia.org/wiki/List_of_hash_functions).
* their fix size doesn't indicate it's some kind of compressed data.

So they are probably just... well... random IDs.

Also, it means that `.pkg` files only contains the raw resource files bundled together. All the meta data associated with these files, most importantly their names are contained elsewhere.

### Looking for the files metadata

So it's back to looking at the game files. Hopefully the resources metadata are not embedded directly in the WoWs executable or one of its library.

Let's look:

```shell
# List all files
# then remove uninteresting bits like replays, crash, dlls, logs, .pkg or cef stuff (embedded Chrome used for the armory, inventory dockyard and clan base))

kakwa@linux Games/World of Warships » find ./ -type f | grep -v cef | grep -v replays | grep -v crashes | grep -v '.pkg' | grep -v '.dll' | grep -v '.log'  | grep -v '\.exe' | less
[...]
./bin/6775398/res/texts/pl/LC_MESSAGES/global.mo
./bin/6775398/res/texts/zh_tw/LC_MESSAGES/global.mo
./bin/6775398/res/texts/fr/LC_MESSAGES/global.mo
./bin/6775398/res/texts/nl/LC_MESSAGES/global.mo
./bin/6775398/res/texts/th/LC_MESSAGES/global.mo
./bin/6775398/res/texts/es/LC_MESSAGES/global.mo
./bin/6775398/res/texts/ru/LC_MESSAGES/global.mo
./bin/6775398/res/texts/zh/LC_MESSAGES/global.mo
./bin/6775398/res/texts/ja/LC_MESSAGES/global.mo
./bin/6775398/res/texts/en/LC_MESSAGES/global.mo
./bin/6775398/res/texts/de/LC_MESSAGES/global.mo
./bin/6775398/res/texts/es_mx/LC_MESSAGES/global.mo
./bin/6775398/res/texts/zh_sg/LC_MESSAGES/global.mo
./bin/6775398/res/camerasConsumer.xml
./bin/6775398/bin32/paths.xml
./bin/6775398/bin32/Licenses.txt
./bin/6775398/bin32/monitor_config.json
./bin/6775398/bin64/paths.xml
./bin/6775398/bin64/Licenses.txt
./bin/6775398/bin64/monitor_config.json
./bin/6775398/idx/spaces_dock_dry.idx
./bin/6775398/idx/vehicles_pve.idx
./bin/6775398/idx/vehicles_level8_fr.idx
./bin/6775398/idx/spaces_ocean.idx
./bin/6775398/idx/vehicles_level8_panasia.idx
./bin/6775398/idx/vehicles_level9_ned.idx
./bin/6775398/idx/vehicles_level5_panasia.idx
./bin/6775398/idx/spaces_sea_hope.idx
./bin/6775398/idx/spaces_dock_hsf.idx
[...]
```


Ohhh, these `.idx` files look promissing, specially their names match quite well the pkg files:

```
spaces_dock_hsf.idx -> spaces_dock_hsf_0001.pkg
ehicles_level9_ned.idx -> vehicles_level9_ned_0001.pkg
etc
```

Let's take a look:

```shell
kakwa@linux bin/6775398/idx » strings -n 5 system_data.idx
#%B'E
#%B'E
#%B'E
E)zj'
FM'lb
%}n:b
(	?A+
c|'lY
zc78'
tKDStorage.bin
waves_heights1.dds
animatedMiscs.xml
LowerDeck.dds
[...]
maps
helpers
FoamMapLowFreq.dds
tritanopia.dds
color_correct_default.dds
Color.dds
Shimmer.dds
highlight_noise.dds
space_variation_dummy.dds
waves_heights0.dds
4F$)p
E)zjp
 |5*y
FM'lp
k4|W8
%}n:p
(	?A+
c|'lp
LrL)t
atw|$
:M+Xp
F?mep
wsystem_data_0001.pkg
```

Bingo, we have all the file names, and at the end, the name of the corresponding `.pkg` file.

These `.idx` files, as the extension indicates, are our indexes containing all the file names and metadata.

Also, note that we have a few names (like `maps` or `helpers`) without extensions, these are probably directory names.

### bin directory and Game versions

There are a few things to note about the `./bin` directory: it contains several sub-directory looking like that:

```shell
kakwa@linux World of Warships/bin » du -hd 1 | sort -h
12K	./5241351
12K	./5315992
12K	./5343985
12K	./5450485
12K	./5624555
12K	./5771708
12K	./5915585
12K	./6081105
12K	./6223574
592M	./6623042
594M	./6775398
```

This look a lot like incrementa build numbers, with WoWs keeping the latest published build 'N' and 'N-1' (the mostly empty directories are only containing left overs like logs or mods).

We will need to take this into account, using the highest numbered sub-directory to get the most up to date indexes.

### Index (.idx) general layout

So next, lets look at one of these index files.

```shell
kakwa@linux 6775398/idx » hexdump -C system_data.idx | less

00000000  49 53 46 50 00 00 00 02  91 9d 39 b4 40 00 00 00  |ISFP......9.@...|
00000010  37 01 00 00 1c 01 00 00  01 00 00 00 00 00 00 00  |7...............|
00000020  28 00 00 00 00 00 00 00  f6 3b 00 00 00 00 00 00  |(........;......|
00000030  36 71 00 00 00 00 00 00  0e 00 00 00 00 00 00 00  |6q..............|
00000040  e0 26 00 00 00 00 00 00  8f 0c 9a ba 4f 40 b6 93  |.&..........O@..|
00000050  27 b9 08 b1 d1 a1 b1 db  13 00 00 00 00 00 00 00  |'...............|
00000060  ce 26 00 00 00 00 00 00  8f ec 87 4a 28 d0 f7 c7  |.&.........J(...|
00000070  a4 eb 1b 3e 50 21 d8 74  12 00 00 00 00 00 00 00  |...>P!.t........|
00000080  c1 26 00 00 00 00 00 00  ad 70 a2 e7 ac 2c 4f 6b  |.&.......p...,Ok|
00000090  27 b9 08 b1 d1 a1 b1 db  0e 00 00 00 00 00 00 00  |'...............|
000000a0  b3 26 00 00 00 00 00 00  4e 84 a5 6a 94 dc 1f 7f  |.&......N..j....|
000000b0  62 f5 aa 4b 5e 15 7f 93  10 00 00 00 00 00 00 00  |b..K^...........|
000000c0  a1 26 00 00 00 00 00 00  4e b0 fe 23 62 40 a5 65  |.&......N..#b@.e|
000000d0  27 b9 08 b1 d1 a1 b1 db  20 00 00 00 00 00 00 00  |'....... .......|
000000e0  91 26 00 00 00 00 00 00  8e c7 6a 58 7c 86 62 33  |.&........jX|.b3|
000000f0  27 b9 08 b1 d1 a1 b1 db  0f 00 00 00 00 00 00 00  |'...............|
00000100  91 26 00 00 00 00 00 00  8e 43 3d e9 cf 49 52 a4  |.&.......C=..IR.|
00000110  62 f5 aa 4b 5e 15 7f 93  16 00 00 00 00 00 00 00  |b..K^...........|
00000120  80 26 00 00 00 00 00 00  0e 3c 9a 6d 22 de 7b da  |.&.......<.m".{.|
00000130  4e b0 fe 23 62 40 a5 65  0b 00 00 00 00 00 00 00  |N..#b@.e........|
00000140  76 26 00 00 00 00 00 00  0e 48 58 ea 50 44 1a 47  |v&.......HX.PD.G|
00000150  df 61 50 67 c7 3a dd 7a  13 00 00 00 00 00 00 00  |.aPg.:.z........|
00000160  61 26 00 00 00 00 00 00  e0 9f c3 bd d2 12 20 04  |a&............ .|
00000170  09 28 f1 df 2d 04 93 de  0b 00 00 00 00 00 00 00  |.(..-...........|
00000180  54 26 00 00 00 00 00 00  e0 95 53 f6 cc 08 c0 46  |T&........S....F|
00000190  06 cf 85 bd 69 99 e2 46  0d 00 00 00 00 00 00 00  |....i..F........|
000001a0  3f 26 00 00 00 00 00 00  e0 99 68 5e 2d 70 13 72  |?&........h^-p.r|
000001b0  4c d1 2e 30 73 38 d9 13  10 00 00 00 00 00 00 00  |L..0s8..........|
[...]
00002660  53 15 00 00 00 00 00 00  a4 eb 1b 3e 50 21 d8 74  |S..........>P!.t|
00002670  38 e6 83 3c 74 a7 20 b2  0a 00 00 00 00 00 00 00  |8..<t. .........|
00002680  37 15 00 00 00 00 00 00  e7 c6 0b ae d5 31 51 55  |7............1QU|
00002690  a4 eb 1b 3e 50 21 d8 74  0c 00 00 00 00 00 00 00  |...>P!.t........|
000026a0  21 15 00 00 00 00 00 00  00 40 cc c7 49 c4 54 09  |!........@..I.T.|
000026b0  a4 eb 1b 3e 50 21 d8 74  14 00 00 00 00 00 00 00  |...>P!.t........|
000026c0  0d 15 00 00 00 00 00 00  ce 6a 28 bc cf e7 79 c8  |.........j(...y.|
000026d0  a4 eb 1b 3e 50 21 d8 74  1a 00 00 00 00 00 00 00  |...>P!.t........|
000026e0  01 15 00 00 00 00 00 00  6c c0 c9 f7 7e 00 03 05  |........l...~...|
000026f0  a4 eb 1b 3e 50 21 d8 74  13 00 00 00 00 00 00 00  |...>P!.t........|
00002700  fb 14 00 00 00 00 00 00  74 d1 1b 8d f4 ff 7a ce  |........t.....z.|
00002710  a4 eb 1b 3e 50 21 d8 74  4b 44 53 74 6f 72 61 67  |...>P!.tKDStorag|
00002720  65 2e 62 69 6e 00 77 61  76 65 73 5f 68 65 69 67  |e.bin.waves_heig|
00002730  68 74 73 31 2e 64 64 73  00 61 6e 69 6d 61 74 65  |hts1.dds.animate|
00002740  64 4d 69 73 63 73 2e 78  6d 6c 00 4c 6f 77 65 72  |dMiscs.xml.Lower|
00002750  44 65 63 6b 2e 64 64 73  00 63 6f 6d 6d 61 6e 64  |Deck.dds.command|
00002760  5f 6d 61 70 70 69 6e 67  00 61 75 74 6f 74 65 73  |_mapping.autotes|
00002770  74 73 5f 64 72 61 77 5f  67 75 69 5f 73 65 74 74  |ts_draw_gui_sett|
00002780  69 6e 67 73 2e 78 6d 6c  00 48 6f 75 73 65 46 72  |ings.xml.HouseFr|
00002790  6f 6e 74 2e 64 64 73 00  70 72 65 73 65 74 5f 6b  |ont.dds.preset_k|
000027a0  65 79 62 6f 61 72 64 5f  31 2e 78 6d 6c 00 69 6e  |eyboard_1.xml.in|
000027b0  74 65 72 66 61 63 65 73  00 76 65 72 64 61 6e 61  |terfaces.verdana|
000027c0  5f 73 6d 61 6c 6c 2e 66  6f 6e 74 00 73 70 61 63  |_small.font.spac|
000027d0  65 5f 64 65 66 73 00 61  69 64 5f 6e 75 6c 6c 2e  |e_defs.aid_null.|
000027e0  64 64 73 00 63 61 6d 6f  75 66 6c 61 67 65 73 2e  |dds.camouflages.|
000027f0  78 6d 6c 00 63 68 75 6e  6b 2e 64 64 73 00 66 6f  |xml.chunk.dds.fo|
00002800  6e 74 63 6f 6e 66 69 67  2e 78 6d 6c 00 46 6f 61  |ntconfig.xml.Foa|
00002810  6d 4d 61 70 2e 64 64 73  00 42 75 69 6c 64 69 6e  |mMap.dds.Buildin|
00002820  67 2e 64 65 66 00 63 6f  6e 66 69 67 2e 78 6d 6c  |g.def.config.xml|
00002830  00 67 61 6d 65 5f 77 69  6e 64 5f 6e 6f 69 73 65  |.game_wind_noise|
00002840  2e 64 64 73 00 47 61 6d  65 50 61 72 61 6d 73 2e  |.dds.GameParams.|
00002850  64 61 74 61 00 44 65 62  75 67 44 72 61 77 45 6e  |data.DebugDrawEn|
00002860  74 69 74 79 2e 64 65 66  00 63 6f 6e 74 65 6e 74  |tity.def.content|
00002870  00 4c 6f 77 65 72 46 6f  72 77 61 72 64 54 72 61  |.LowerForwardTra|
00002880  6e 73 2e 64 64 73 00 55  49 50 61 72 61 6d 73 2e  |ns.dds.UIParams.|
[...]
00003bc0  2e 64 64 73 00 68 69 67  68 6c 69 67 68 74 5f 6e  |.dds.highlight_n|
00003bd0  6f 69 73 65 2e 64 64 73  00 73 70 61 63 65 5f 76  |oise.dds.space_v|
00003be0  61 72 69 61 74 69 6f 6e  5f 64 75 6d 6d 79 2e 64  |ariation_dummy.d|
00003bf0  64 73 00 77 61 76 65 73  5f 68 65 69 67 68 74 73  |ds.waves_heights|
00003c00  30 2e 64 64 73 00 8f 0c  9a ba 4f 40 b6 93 70 11  |0.dds.....O@..p.|
00003c10  03 07 0d 33 ed 77 00 00  00 00 00 00 00 00 05 00  |...3.w..........|
00003c20  00 00 01 00 00 00 f5 21  00 00 bf 00 45 5c 6c 36  |.......!....E\l6|
00003c30  00 00 00 00 00 00 8f ec  87 4a 28 d0 f7 c7 70 11  |.........J(...p.|
00003c40  03 07 0d 33 ed 77 1e 9b  ef 05 00 00 00 00 05 00  |...3.w..........|
00003c50  00 00 01 00 00 00 15 15  01 00 03 77 63 97 3e ab  |...........wc.>.|
00003c60  02 00 00 00 00 00 ad 70  a2 e7 ac 2c 4f 6b 70 11  |.......p...,Okp.|
00003c70  03 07 0d 33 ed 77 05 22  00 00 00 00 00 00 05 00  |...3.w."........|
00003c80  00 00 01 00 00 00 cb 01  00 00 6d b9 de c1 ad 0c  |..........m.....|
00003c90  00 00 00 00 00 00 8e c7  6a 58 7c 86 62 33 70 11  |........jX|.b3p.|
00003ca0  03 07 0d 33 ed 77 e0 23  00 00 00 00 00 00 05 00  |...3.w.#........|
00003cb0  00 00 01 00 00 00 65 00  00 00 f1 d2 87 5a d2 00  |......e......Z..|
00003cc0  00 00 00 00 00 00 8e 43  3d e9 cf 49 52 a4 70 11  |.......C=..IR.p.|
00003cd0  03 07 0d 33 ed 77 4d 1f  6a 05 00 00 00 00 05 00  |...3.wM.j.......|
00003ce0  00 00 01 00 00 00 bb 19  00 00 f7 53 4a b1 38 ab  |...........SJ.8.|
00003cf0  00 00 00 00 00 00 0e 3c  9a 6d 22 de 7b da 70 11  |.......<.m".{.p.|
00003d00  03 07 0d 33 ed 77 55 24  00 00 00 00 00 00 05 00  |...3.wU$........|
[...]
000070a0  00 00 01 00 00 00 90 24  00 00 62 c1 d9 06 f8 d8  |.......$..b.....|
000070b0  00 00 00 00 00 00 57 b3  82 06 56 f0 2a e6 70 11  |......W...V.*.p.|
000070c0  03 07 0d 33 ed 77 ea cc  15 0a 00 00 00 00 05 00  |...3.w..........|
000070d0  00 00 01 00 00 00 7c 01  00 00 84 a8 12 1d 61 04  |......|.......a.|
000070e0  00 00 00 00 00 00 57 64  91 29 c9 3c f0 96 70 11  |......Wd.).<..p.|
000070f0  03 07 0d 33 ed 77 a9 ef  15 0a 00 00 00 00 05 00  |...3.w..........|
00007100  00 00 01 00 00 00 6f 09  00 00 fc 56 94 f8 9a 37  |......o....V...7|
00007110  00 00 00 00 00 00 21 67  ac 70 22 ec ca b8 70 11  |......!g.p"...p.|
00007120  03 07 0d 33 ed 77 28 f9  15 0a 00 00 00 00 05 00  |...3.w(.........|
00007130  00 00 01 00 00 00 0b 2d  00 00 03 bd b0 50 67 e9  |.......-.....Pg.|
00007140  00 00 00 00 00 00 15 00  00 00 00 00 00 00 18 00  |................|
00007150  00 00 00 00 00 00 70 11  03 07 0d 33 ed 77 73 79  |......p....3.wsy|
00007160  73 74 65 6d 5f 64 61 74  61 5f 30 30 30 31 2e 70  |stem_data_0001.p|
00007170  6b 67 00                                          |kg.|
```

The general layout of the file appears to be at least in 3 chunks:

* A first chunk of metadata
* A all the file name strings `\0` separated, but also a few strings without extensions, probably directory names.
* A second chunk of metadata

Lets look for the IDs we found in the corresponding `.pkg` (here `system_data_0001.pkg`), for example `00 00 00 00 | bf 00 45 5c | 6c 36 00 00 | 00 00 00 00`.

```
[......................] 00 8f 0c  9a ba 4f 40 b6 93 70 11  |0.dds.....O@..p.|
00003c10  03 07 0d 33 ed 77 00 00  00 00 00 00 00 00 05 00  |...3.w..........|
00003c20  00 00 01 00 00 00 f5 21  00 00 bf 00 45 5c 6c 36  |.......!....E\l6|
00003c30  00 00 00 00 00 00 8f ec [...]
```

Ok, it's there, in the second chunk. And it also works if we test for other IDs. We have at least a link by ID between the `.idx` and the `.pkg` file.

We will come back later to the second chunk, remembering that, but lets focus on the first chunk for now.

### Format of the first metadata chunk of the '.idx' file

Lets try to understand the first part of the .idx file structure.

```shell
kakwa@linux 6775398/idx » hexdump -C system_data.idx | less

00000000  49 53 46 50 00 00 00 02  91 9d 39 b4 40 00 00 00  |ISFP......9.@...|
00000010  37 01 00 00 1c 01 00 00  01 00 00 00 00 00 00 00  |7...............|
00000020  28 00 00 00 00 00 00 00  f6 3b 00 00 00 00 00 00  |(........;......|
00000030  36 71 00 00 00 00 00 00  0e 00 00 00 00 00 00 00  |6q..............|
00000040  e0 26 00 00 00 00 00 00  8f 0c 9a ba 4f 40 b6 93  |.&..........O@..|
00000050  27 b9 08 b1 d1 a1 b1 db  13 00 00 00 00 00 00 00  |'...............|
00000060  ce 26 00 00 00 00 00 00  8f ec 87 4a 28 d0 f7 c7  |.&.........J(...|
00000070  a4 eb 1b 3e 50 21 d8 74  12 00 00 00 00 00 00 00  |...>P!.t........|
00000080  c1 26 00 00 00 00 00 00  ad 70 a2 e7 ac 2c 4f 6b  |.&.......p...,Ok|
00000090  27 b9 08 b1 d1 a1 b1 db  0e 00 00 00 00 00 00 00  |'...............|
000000a0  b3 26 00 00 00 00 00 00  4e 84 a5 6a 94 dc 1f 7f  |.&......N..j....|
000000b0  62 f5 aa 4b 5e 15 7f 93  10 00 00 00 00 00 00 00  |b..K^...........|
000000c0  a1 26 00 00 00 00 00 00  4e b0 fe 23 62 40 a5 65  |.&......N..#b@.e|
000000d0  27 b9 08 b1 d1 a1 b1 db  20 00 00 00 00 00 00 00  |'....... .......|
000000e0  91 26 00 00 00 00 00 00  8e c7 6a 58 7c 86 62 33  |.&........jX|.b3|
000000f0  27 b9 08 b1 d1 a1 b1 db  0f 00 00 00 00 00 00 00  |'...............|
00000100  91 26 00 00 00 00 00 00  8e 43 3d e9 cf 49 52 a4  |.&.......C=..IR.|
00000110  62 f5 aa 4b 5e 15 7f 93  16 00 00 00 00 00 00 00  |b..K^...........|
00000120  80 26 00 00 00 00 00 00  0e 3c 9a 6d 22 de 7b da  |.&.......<.m".{.|
00000130  4e b0 fe 23 62 40 a5 65  0b 00 00 00 00 00 00 00  |N..#b@.e........|
00000140  76 26 00 00 00 00 00 00  0e 48 58 ea 50 44 1a 47  |v&.......HX.PD.G|
00000150  df 61 50 67 c7 3a dd 7a  13 00 00 00 00 00 00 00  |.aPg.:.z........|
00000160  61 26 00 00 00 00 00 00  e0 9f c3 bd d2 12 20 04  |a&............ .|
00000170  09 28 f1 df 2d 04 93 de  0b 00 00 00 00 00 00 00  |.(..-...........|

00002690  a4 eb 1b 3e 50 21 d8 74  0c 00 00 00 00 00 00 00  |...>P!.t........|
000026a0  21 15 00 00 00 00 00 00  00 40 cc c7 49 c4 54 09  |!........@..I.T.|
000026b0  a4 eb 1b 3e 50 21 d8 74  14 00 00 00 00 00 00 00  |...>P!.t........|
000026c0  0d 15 00 00 00 00 00 00  ce 6a 28 bc cf e7 79 c8  |.........j(...y.|
000026d0  a4 eb 1b 3e 50 21 d8 74  1a 00 00 00 00 00 00 00  |...>P!.t........|
000026e0  01 15 00 00 00 00 00 00  6c c0 c9 f7 7e 00 03 05  |........l...~...|
000026f0  a4 eb 1b 3e 50 21 d8 74  13 00 00 00 00 00 00 00  |...>P!.t........|
00002700  fb 14 00 00 00 00 00 00  74 d1 1b 8d f4 ff 7a ce  |........t.....z.|
00002710  a4 eb 1b 3e 50 21 d8 74  4b 44 53 74 6f 72 61 67  |...>P!.tKDStorag|
00002720  65 2e 62 69 6e 00 77 61  76 65 73 5f 68 65 69 67  |e.bin.waves_heig|
00002730  68 74 73 31 2e 64 64 73  00 61 6e 69 6d 61 74 65  |hts1.dds.animate|
00002740  64 4d 69 73 63 73 2e 78  6d 6c 00 4c 6f 77 65 72  |dMiscs.xml.Lower|
00002750  44 65 63 6b 2e 64 64 73  00 63 6f 6d 6d 61 6e 64  |Deck.dds.command|
```

Staring at the hexdump long enough and we can start to see some patterns.

At regular interval, every 256 bits, we get a 64 bits integer with a relatively low value, hinting at individual metadata sets of 256 bits.

This look suspeciously like some kind of constant enum coded into a 64 bits integer.
My best guess right now would be some kind of file type code, itself define as a constant in the game engine like that:

```C
#define FILE_TYPE_1 0x01
#define FILE_TYPE_2 0x02
// [...]
```

Let's call it `file_type` for now.

Looking at the end of the first chunk (right before we get `KDStorage.bin`), we have to go back 4 x 64 bits to get something that looks like a `file_type`.

This means `file_type` is the first field in the 256 bits structure.

Let's look at the next 64 bits: `ce 26 00 00 00 00 00 00`, `c1 26 00 00 00 00 00 00`, `80 26 00 00 00 00 00 00`, etc. These values are again rather small.

Also these values, at least for the first ones, are suspeciously close to `00002718`, e.i right where the section containing the file names starts.

The last ones, `01 15 00 00 00 00 00 00`, `fb 14 00 00 00 00 00 00`, etc, are smaller, and suspiciously, they have similar value to the lenght of the file name section (approximately `0x00003c00 - 0x00002710 = 0x0000014f0`).

The second field is then probably some kind of offset. Most likely from the start of one 256 bits chunk to the start of one of the file names.

Looking at other `.idx` files seems to confirm that.

Let's call it `offset` for now.

Next, let's look at the remaining 128 bits.

```
8f 0c 9a ba 4f 40 b6 93 | 27 b9 08 b1 d1 a1 b1 db
8f ec 87 4a 28 d0 f7 c7 | a4 eb 1b 3e 50 21 d8 74
ad 70 a2 e7 ac 2c 4f 6b | 27 b9 08 b1 d1 a1 b1 db
4e 84 a5 6a 94 dc 1f 7f | 62 f5 aa 4b 5e 15 7f 93
4e b0 fe 23 62 40 a5 65 | 27 b9 08 b1 d1 a1 b1 db
8e c7 6a 58 7c 86 62 33 | 27 b9 08 b1 d1 a1 b1 db
```

First thing to note, all the bits are used. which disqualifies offsets or simple enum ids like before.

Right now, we are not even if sure these 128 bits are part of one 128 bits field (for example a hash), two 64 bits integers, four 32 integers, or any combination of 16, 32 or 64 bits that ends-up making a 128 bits chunk.

Looking at it more closely, the first 64 bits looks rather random, the last 64 bits however? we see quite a few values repeating themselves (ex: `27 b9 08 b1 d1 a1 b1 db`).

Lets check the whole file:


```shell
kakwa@linux 6775398/idx » hexdump -C system_data.idx| grep '00 00 00 00 00 00 00' | sed 's/^..........//' | sed 's/  .*//' | sort  | uniq -c | sort -n

# note: first number is the number of occurance
      1 00 00 00 00 00 00 00 af
      1 1c 6a 8d 7f df 8e b3 35
      1 28 00 00 00 00 00 00 00
      1 36 71 00 00 00 00 00 00
      1 37 01 00 00 1c 01 00 00
      1 7d e3 1c a4 35 3e 98 8b
      1 e0 95 53 f6 cc 08 c0 46
      2 12 46 58 36 c8 ec 47 8b
      2 4e b0 fe 23 62 40 a5 65
      2 d0 d7 a5 ce a8 86 0e ae
      3 1a aa c7 3c 4e 76 ad 94
      3 4c d1 2e 30 73 38 d9 13
      3 4c f0 ea c1 d5 5a 8d 12
      3 88 57 fc 1c 72 f3 84 fa
      3 ac 82 d5 f6 9e db 47 f9
      3 b1 86 45 dc 7a 63 37 38
      3 fc 27 83 2d 44 46 30 a3
      6 76 83 17 b5 cf dd b7 0e
      8 06 cf 85 bd 69 99 e2 46
      9 09 28 f1 df 2d 04 93 de
     10 a4 eb 1b 3e 50 21 d8 74
     10 d7 22 2f fc 0a 67 7a 0d
     14 aa db f0 18 01 89 b6 d8
     15 df 61 50 67 c7 3a dd 7a
     18 19 ac 65 3f 91 78 97 dc
     19 38 e6 83 3c 74 a7 20 b2
     19 59 dc e0 43 fc 88 b7 7c
     23 d3 9e 86 23 25 42 27 45
     24 0e 48 58 ea 50 44 1a 47
     33 d7 19 f3 03 3e 6e 59 03
     34 27 b9 08 b1 d1 a1 b1 db
     38 62 f5 aa 4b 5e 15 7f 93
```

Indeed, the repetitions are quite frequent, and running the same command on other files yeild roughly the same values:

```shell
kakwa@linux 6775398/idx » hexdump -C particles.idx| grep '00 00 00 00 00 00 00' | sed 's/^..........//' | sed 's/  .*//' | sort  | uniq -c | sort -n 
      1 27 b9 08 b1 d1 a1 b1 db
      1 28 00 00 00 00 00 00 00
      1 8e ae 00 00 00 00 00 00
      1 bd 01 00 00 b7 01 00 00
      4 ca 2c b7 97 24 b8 1c 86
      5 1f 19 a6 0c a2 9b 7f b3
     16 94 a1 23 f4 c5 41 b8 42
     20 91 cc 55 52 25 2a 42 d4
     81 de 3e 45 0e 99 dc 30 14
    317 66 52 00 d6 89 64 1d 2e
```

More over, `sound_music.idx` wchi as the name implies probably only contains sound files returns mostly one type:

```shell
kakwa@linux 6775398/idx » hexdump -C sound_music.idx| grep '00 00 00 00 00 00 00' | sed 's/^..........//' | sed 's/  .*//' | sort  | uniq -c | sort -n
    513 93 63 67 56 c2 97 75 69
```

Note: these commands are by no mean accurate, they are likely to catch garbage and miscount. But these are quick and dirty ways to validate hypothesis.

So it seems we are dealing with another `file_type` field. Let's call it `file_type2`, and rename the first one `file_type1`.

So in the end, making a few assumptions, for now, we have figured out that the rough format of this section:

```
+====+====+====+====+====+====+====+====++====+====+====+====+====+====+====+====+
| T1 | T1 | T1 | T1 | T1 | T1 | T1 | T1 || OF | OF | OF | OF | OF | OF | OF | OF |
+====+====+====+====+====+====+====+====++====+====+====+====+====+====+====+====+
|<---------- file type 1 -------------->||<------------ offset ----------------->|
|               64 bits                 ||               64 bits                 |
+====+====+====+====+====+====+====+====++====+====+====+====+====+====+====+====+
| UN | UN | UN | UN | UN | UN | UN | UN || T2 | T2 | T2 | T2 | T2 | T2 | T2 | T2 |
+====+====+====+====+====+====+====+====++====+====+====+====+====+====+====+====+
|<------------ unknown ---------------->||<---------- file type 2 -------------->|
|               64 bits                 ||               64 bits                 |
```

### Header

Now, lets start analyzing the header part of the `.idx`.

Here is the first bytes on an `.idx` file.

```
00000000  49 53 46 50 00 00 00 02  91 9d 39 b4 40 00 00 00  |ISFP......9.@...|
00000010  37 01 00 00 1c 01 00 00  01 00 00 00 00 00 00 00  |7...............|
00000020  28 00 00 00 00 00 00 00  f6 3b 00 00 00 00 00 00  |(........;......|
00000030  36 71 00 00 00 00 00 00  0e 00 00 00 00 00 00 00  |6q..............|
00000040  e0 26 00 00 00 00 00 00  8f 0c 9a ba 4f 40 b6 93  |.&..........O@..|
00000050  27 b9 08 b1 d1 a1 b1 db  13 00 00 00 00 00 00 00  |'...............|
00000060  ce 26 00 00 00 00 00 00  8f ec 87 4a 28 d0 f7 c7  |.&.........J(...|
00000070  a4 eb 1b 3e 50 21 d8 74  12 00 00 00 00 00 00 00  |...>P!.t........|
00000080  c1 26 00 00 00 00 00 00  ad 70 a2 e7 ac 2c 4f 6b  |.&.......p...,Ok|
00000090  27 b9 08 b1 d1 a1 b1 db  0e 00 00 00 00 00 00 00  |'...............|
````

These first bytes don't look like the `section` previously mentionned, we have a magic number (`ISFP`) and then the content doesn't look like a `section` at first (too many low value 32 bits integers).

This means we most likely have an header section containing things like:
* magic numbers
* types
* sizes
* number of entries/files

The first thing to determine is the size of the header. Looking at it, the first `section` starts at `0x38` (recognisable by they full 64 bits integers).

This means the header is 7 x 64 bits.

Lets analyze the content.

Looking at all the files, for the first 128 bits, we get:

```shell
kakwa@linux 6775398/idx » for i in *;do hexdump -C $i | head -n 1;done 
[...]
00000000  49 53 46 50 00 00 00 02  1b 73 f9 d5 40 00 00 00  |ISFP.....s..@...|
00000000  49 53 46 50 00 00 00 02  78 f0 2c 09 40 00 00 00  |ISFP....x.,.@...|
00000000  49 53 46 50 00 00 00 02  b8 fe ba b9 40 00 00 00  |ISFP........@...|
00000000  49 53 46 50 00 00 00 02  06 24 fa 2d 40 00 00 00  |ISFP.....$.-@...|
00000000  49 53 46 50 00 00 00 02  1e 7e f6 d9 40 00 00 00  |ISFP.....~..@...|
00000000  49 53 46 50 00 00 00 02  dd 21 74 c2 40 00 00 00  |ISFP.....!t.@...|
00000000  49 53 46 50 00 00 00 02  33 28 63 bd 40 00 00 00  |ISFP....3(c.@...|
00000000  49 53 46 50 00 00 00 02  cb 5c e2 0d 40 00 00 00  |ISFP.....\..@...|
00000000  49 53 46 50 00 00 00 02  cb e8 8a fd 40 00 00 00  |ISFP........@...|
00000000  49 53 46 50 00 00 00 02  6e 04 b1 62 40 00 00 00  |ISFP....n..b@...|
00000000  49 53 46 50 00 00 00 02  15 1c a2 f9 40 00 00 00  |ISFP........@...|
[...]
```

As we can see, the 1st, 2nd, and 4th 32 bits chunck are always the same, and looking at the values, we have respectively:
* a magic number (`ISFP`),
* `00 00 00 02` which is rather weird (it could be some kind of id, if we were little endian, but the format is big endian). Maybe it is actually part of the magic number. As it doesn't vary, it's not too important for the task at end there.
* `40 00 00 00` which like `type 1` looks like a low value enum, and given its position in the index file, within the header, we are most likely dealing with an archive type. Again, as it doesn't vary, it's not really important.

The 3rd 32 bits integer uses all the available bits, so it's unlikely a size. Maybe it's a CRC32 or a unique ID for the archives.

Edit: the `40 00 00 00` value upon closer inspection might not be an archive type, its value is 64 in decimal, which might be an header size, or simply storing the size of an integer.

So we have:
```
+====+====+====+====++====+====+====+====++====+====+====+====++====+====+====+====+
| MA | MA | MA | MA || 00 | 00 | 00 | 02 || ID | ID | ID | ID || 40 | 00 | 00 | 00 |
+====+====+====+====++====+====+====+====++====+====+====+====++====+====+====+====+
|<----- magic ----->||<----- ???? ------>||<---- id/crc ----->||<----- ??????? --->|
```

Now, lets look at the next 128 bits (second line in hexdump).

```shell
kakwa@linux 6775398/idx » for i in *;do hexdump -C $i | head -n 2 | tail -n 1;done                                                                                                                                                                                                                                    130 ↵
00000010  bd 01 00 00 b7 01 00 00  01 00 00 00 00 00 00 00  |................|
00000010  35 01 00 00 1d 01 00 00  01 00 00 00 00 00 00 00  |5...............|
00000010  5b 00 00 00 57 00 00 00  01 00 00 00 00 00 00 00  |[...W...........|
00000010  5c 00 00 00 58 00 00 00  01 00 00 00 00 00 00 00  |\...X...........|
00000010  10 18 00 00 ff 17 00 00  01 00 00 00 00 00 00 00  |................|
00000010  29 3b 00 00 0b 3b 00 00  01 00 00 00 00 00 00 00  |);...;..........|
00000010  04 02 00 00 02 02 00 00  01 00 00 00 00 00 00 00  |................|
00000010  1e 05 00 00 c2 03 00 00  01 00 00 00 00 00 00 00  |................|
00000010  a5 01 00 00 36 01 00 00  01 00 00 00 00 00 00 00  |....6...........|
00000010  13 04 00 00 fb 02 00 00  01 00 00 00 00 00 00 00  |................|
00000010  79 03 00 00 a9 02 00 00  01 00 00 00 00 00 00 00  |y...............|
``` 

So here, we recognize two 32 bits integers due to the `00 00`,  and then either a fixed 64 bits integer with always a `01 00 00 00 00 00 00 00` value, or something like two 32 bits integers with value `01 00 00 00` and `00 00 00 00` (as the value never varies, again, it's not that important).

Lets try to determine the two 32 bits values. Let's look at one of the files in particular:

```shell
kakwa@linux 6775398/idx » hexdump -C system_data.idx | less
[...]
00000010  37 01 00 00 1c 01 00 00  01 00 00 00 00 00 00 00  |7...............|
[...]

The first value is `37 01 00 00`, i.e. converted to decimal, `311`. Doing a strings `strings system_data.idx >listing` and remove manually the garbage (ex: `w6~n`) as best as possible, plus the `.pkg` file name, only keeping files and directory names, we get `310` entries, a remarquably close value.

Looking at other files, story is similar, this field roughly matches the number of strings we get from `strings` (never perfectly however, but if the names are too short, `strings` will ignore them, most likely explaining the small delta we have each time).

Consequently we can deduce it's most likely the number of entries (files and directories) in the index file.

Next, we have `1c 01 00 00`, i.e. converted to decimal, `284`. This value is suspisiously close to the previous value. As we have both directories and file names, this number probably represents the number of items which are actual files.

Let's validate that:

```shell
# Q&D filtering out names without an extension (no '.')
kakwa@linux 6775398/idx » cat listing | grep  '\.' | wc -l 
284
```

Bingo, we have the exact number we were looking for.

The last 64 bits could simply be ignored for now since they always have the same value.

So we have:

```
+====+====+====+====++====+====+====+====++====+====+====+====++====+====+====+====+
| FD | FD | FD | FD || FI | FI | FI | FI || 01 | 00 | 00 | 00 || 00 | 00 | 00 | 00 |
+====+====+====+====++====+====+====+====++====+====+====+====++====+====+====+====+
|<file + dir count >||<-- file count --->||<-------------- ???? ------------------>|
```

Next 128 bits:

```shell
kakwa@linux 6775398/idx » for i in *;do hexdump -C $i | head -n 3 | tail -n 1;done | less
[...]
00000020  28 00 00 00 00 00 00 00  58 8d 00 00 00 00 00 00  |(.......X.......|
00000020  28 00 00 00 00 00 00 00  7c 3f 02 00 00 00 00 00  |(.......|?......|
00000020  28 00 00 00 00 00 00 00  77 2f 00 00 00 00 00 00  |(.......w/......|
00000020  28 00 00 00 00 00 00 00  bc e9 01 00 00 00 00 00  |(...............|
00000020  28 00 00 00 00 00 00 00  c6 ea 02 00 00 00 00 00  |(...............|
00000020  28 00 00 00 00 00 00 00  a9 0f 00 00 00 00 00 00  |(...............|
00000020  28 00 00 00 00 00 00 00  c4 22 00 00 00 00 00 00  |(........"......|
00000020  28 00 00 00 00 00 00 00  b6 2b 00 00 00 00 00 00  |(........+......|
00000020  28 00 00 00 00 00 00 00  d6 41 00 00 00 00 00 00  |(........A......|
00000020  28 00 00 00 00 00 00 00  fd 21 00 00 00 00 00 00  |(........!......|
00000020  28 00 00 00 00 00 00 00  e1 25 00 00 00 00 00 00  |(........%......|
00000020  28 00 00 00 00 00 00 00  f5 31 00 00 00 00 00 00  |(........1......|
00000020  28 00 00 00 00 00 00 00  1e 42 00 00 00 00 00 00  |(........B......|
00000020  28 00 00 00 00 00 00 00  70 42 02 00 00 00 00 00  |(.......pB......|
[...]
```

So here, we have 2 64 bits integers, the first one is `28 00 00 00 00 00 00 00` and always has the same value, not sure what it represents, the value is somewhat close to the header size in bytes: 40 for this value, 56 for the full header size.

Maybe the header could vary in size in certain situations, and this represents its size minus some fixed part (like the first 16 bytes/128 bits). I'm also kind of betting that if the previous 64 bits integer (`01 00 00 00 00 00 00 00` changes, this will also change.

But as it never varies in the set of index files we have here, we cannot really make any deduction, only guesses. So once again, lets ignore it.

At this point, the idea of downloading other Wargaming games like World of Tanks or World of Warplanes popped-up, maybe this will give complementary information regarding the unknown fields that starts to pile-up.

But lets continue for now.

EDIT: It's no help, World of Tanks and World of Warplanes simply pack their resources in `.zip` files...
It's a wild guess, but I kind of expect WoWs to be the same in the futur.
The WoWs packing format feels in fact somewhat legacy, custom and far less efficient than a standard & run of the mill `.zip` file. Not to mention using `.zip` files means removing one bit of code to maintain.

The next value is again a 64 bits integer, it changes between each files.

lets focus on one file:

```
[...]
00000020  28 00 00 00 00 00 00 00  f6 3b 00 00 00 00 00 00  |(........;......|
[...]
```

At first, I though it might be a file size, but quickly checking the index file size, I got:

* index file size: 29043
* value of this field in decimal: 15350

Checking another file, I got 77461 and 44807.

So no, it's not the index size. However it is suspisously ~1/2 of the file size, and after having stared at hexdumps for hours, I had another idea.

The third chunk of the file is right after the bundle of dir/file name strings which varies in length wildly (e.i, it's not fixed length or a multiple of a fix length).

We probably need an offset pointing to the start of this section in the header.

And sure enough, looking where the bundle of strings stops, we get:

```
00003bd0  6f 69 73 65 2e 64 64 73  00 73 70 61 63 65 5f 76  |oise.dds.space_v|
00003be0  61 72 69 61 74 69 6f 6e  5f 64 75 6d 6d 79 2e 64  |ariation_dummy.d|
00003bf0  64 73 00 77 61 76 65 73  5f 68 65 69 67 68 74 73  |ds.waves_heights|
00003c00  30 2e 64 64 73 00 8f ec  87 4a 28 d0 f7 c7 70 11  |0.dds....J(...p.|
00003c10  03 07 0d 33 ed 77 1e 9b  ef 05 00 00 00 00 05 00  |...3.w..........|
```

Okay, the string bundle ends at 00003c05, that's quite near 3b f6, so this is certainly the offset to this third section or the end of the bundle of strings.

Most likely, the offset is not from the start of the file, but from a specific in the header (this field? end of header?), that's why we get a -15 difference (0x00003c05 - 0x3bf6 = 0xF = 15). This -15 value is constant between file.

So we have:
```
+====+====+====+====+====+====+====+====++====+====+====+====+====+====+====+====+
| HS | HS | HS | HS | HS | HS | HS | HS || OF | OF | OF | OF | OF | OF | OF | OF |
+====+====+====+====+====+====+====+====++====+====+====+====+====+====+====+====+
|<------------ header size (?) -------->||<---- offset third section start ----->|
```

Last 64 bits:


```shell
kakwa@linux 6623042/idx » for i in *;do hexdump -C $i | head -n 4 | tail -n 1;done | less
[...]
00000030  40 af 03 00 00 00 00 00  1b 00 00 00 00 00 00 00  |@...............|
00000030  54 8d 1f 00 00 00 00 00  21 00 00 00 00 00 00 00  |T.......!.......|
00000030  8e ae 00 00 00 00 00 00  17 00 00 00 00 00 00 00  |................|
00000030  02 81 00 00 00 00 00 00  13 00 00 00 00 00 00 00  |................|
00000030  c9 24 00 00 00 00 00 00  1f 00 00 00 00 00 00 00  |.$..............|
[...]
```

So, we have a 64 bits integer, which is relatively low value. This means it's most like a size or an offset.

If we pick one:

```
hexdump -C system_data.idx | less
[...]
00000030  36 71 00 00 00 00 00 00  13 00 00 00 00 00 00 00  |6q..............|
[...]
```

We can see that the `36 71 00 00 00 00 00 00`, 28982 once converted to decimal, is remarkably close to the file size (29043 bytes).

From their, we can guess it might be three things:

* the actual index file size
* an offset to something at the end of the file
* pointer to the end of the third section

Lets note that for now, and figure out the finer details at implementation time.

So, to recap, here is the header section format:

```
+====+====+====+====++====+====+====+====++====+====+====+====++====+====+====+====+
| MA | MA | MA | MA || 00 | 00 | 00 | 02 || ID | ID | ID | ID || 40 | 00 | 00 | 00 |
+====+====+====+====++====+====+====+====++====+====+====+====++====+====+====+====+
|<----- magic ----->||<----- ???? ------>||<---- id/crc ----->||<----- ??????? --->|

+====+====+====+====++====+====+====+====++====+====+====+====++====+====+====+====+
| FD | FD | FD | FD || FI | FI | FI | FI || 01 | 00 | 00 | 00 || 00 | 00 | 00 | 00 |
+====+====+====+====++====+====+====+====++====+====+====+====++====+====+====+====+
|<file + dir count >||<-- file count --->||<-------------- ???? ------------------>|

+====+====+====+====+====+====+====+=====++=====+====+====+====+====+====+====+====+
| HS | HS | HS | HS | HS | HS | HS | HS  ||  OF | OF | OF | OF | OF | OF | OF | OF |
+====+====+====+====+====+====+====+=====++=====+====+====+====+====+====+====+====+
|<------------ header size (?) --------->||<----- offset third section start ----->|

+====+====+====+====+====+====+====+=====+
| OE | OE | OE | OE | OE | OE | OE | OE  |
+====+====+====+====+====+====+====+=====+
|<---- offset third section end -------->|
```

### Format of the middle section

Not much to say there.

Here what it looks like:

```shell
kakwa@linux 6775398/idx » hexdump -C system_data.idx| less
[...]
00002700  fb 14 00 00 00 00 00 00  74 d1 1b 8d f4 ff 7a ce  |........t.....z.|
00002710  a4 eb 1b 3e 50 21 d8 74  4b 44 53 74 6f 72 61 67  |...>P!.tKDStorag|
00002720  65 2e 62 69 6e 00 77 61  76 65 73 5f 68 65 69 67  |e.bin.waves_heig|
00002730  68 74 73 31 2e 64 64 73  00 61 6e 69 6d 61 74 65  |hts1.dds.animate|
[...]
000028c0  74 73 2e 62 69 6e 00 6d  69 73 63 53 65 74 74 69  |ts.bin.miscSetti|
000028d0  6e 67 73 2e 78 6d 6c 00  64 61 6d 61 67 65 5f 64  |ngs.xml.damage_d|
000028e0  65 63 5f 32 5f 64 2e 64  64 32 00 64 61 6d 61 67  |ec_2_d.dd2.damag|
000028f0  65 5f 64 65 63 5f 31 5f  65 2e 64 64 73 00 63 72  |e_dec_1_e.dds.cr|
[...]
00003be0  61 72 69 61 74 69 6f 6e  5f 64 75 6d 6d 79 2e 64  |ariation_dummy.d|
00003bf0  64 73 00 77 61 76 65 73  5f 68 65 69 67 68 74 73  |ds.waves_heights|
00003c00  30 2e 64 64 73 00 8f 0c  9a ba 4f 40 b6 93 70 11  |0.dds.....O@..p.|
00003c10  03 07 0d 33 ed 77 00 00  00 00 00 00 00 00 05 00  |...3.w..........|
00003c20  00 00 01 00 00 00 f5 21  00 00 bf 00 45 5c 6c 36  |.......!....E\l6|
```

So it's a bunch of `\0` separated strings. The only thing interesting to note is that it's not a fixed size section.

### Format of the third section


Here what it looks like:

```shell
kakwa@linux 6775398/idx » hexdump -C system_data.idx| less
[...]
00003bf0  64 73 00 77 61 76 65 73  5f 68 65 69 67 68 74 73  |ds.waves_heights|
00003c00  30 2e 64 64 73 00 8f 0c  9a ba 4f 40 b6 93 70 11  |0.dds.....O@..p.|
00003c10  03 07 0d 33 ed 77 00 00  00 00 00 00 00 00 05 00  |...3.w..........|
00003c20  00 00 01 00 00 00 f5 21  00 00 bf 00 45 5c 6c 36  |.......!....E\l6|
00003c30  00 00 00 00 00 00 8f ec  87 4a 28 d0 f7 c7 70 11  |.........J(...p.|
00003c40  03 07 0d 33 ed 77 1e 9b  ef 05 00 00 00 00 05 00  |...3.w..........|
00003c50  00 00 01 00 00 00 15 15  01 00 03 77 63 97 3e ab  |...........wc.>.|
00003c60  02 00 00 00 00 00 ad 70  a2 e7 ac 2c 4f 6b 70 11  |.......p...,Okp.|
00003c70  03 07 0d 33 ed 77 05 22  00 00 00 00 00 00 05 00  |...3.w."........|
00003c80  00 00 01 00 00 00 cb 01  00 00 6d b9 de c1 ad 0c  |..........m.....|
00003c90  00 00 00 00 00 00 8e c7  6a 58 7c 86 62 33 70 11  |........jX|.b3p.|
00003ca0  03 07 0d 33 ed 77 e0 23  00 00 00 00 00 00 05 00  |...3.w.#........|
00003cb0  00 00 01 00 00 00 65 00  00 00 f1 d2 87 5a d2 00  |......e......Z..|
00003cc0  00 00 00 00 00 00 8e 43  3d e9 cf 49 52 a4 70 11  |.......C=..IR.p.|
00003cd0  03 07 0d 33 ed 77 4d 1f  6a 05 00 00 00 00 05 00  |...3.wM.j.......|
00003ce0  00 00 01 00 00 00 bb 19  00 00 f7 53 4a b1 38 ab  |...........SJ.8.|
00003cf0  00 00 00 00 00 00 0e 3c  9a 6d 22 de 7b da 70 11  |.......<.m".{.p.|
00003d00  03 07 0d 33 ed 77 55 24  00 00 00 00 00 00 05 00  |...3.wU$........|
00003d10  00 00 01 00 00 00 72 0d  00 00 83 0a 72 88 a3 5c  |......r.....r..\|
00003d20  00 00 00 00 00 00 98 45  00 7a 16 6e 84 21 70 11  |.......E.z.n.!p.|
00003d30  03 07 0d 33 ed 77 73 ce  cb 06 00 00 00 00 05 00  |...3.ws.........|
00003d40  00 00 01 00 00 00 f9 b6  b7 00 ff 7e f7 7b 5b 30  |...........~.{[0|
00003d50  b8 00 00 00 00 00 98 51  4a 00 2c 12 71 ad 70 11  |.......QJ.,.q.p.|
00003d60  03 07 0d 33 ed 77 7e 63  75 05 00 00 00 00 05 00  |...3.w~cu.......|
[...]
00007100  00 00 01 00 00 00 6f 09  00 00 fc 56 94 f8 9a 37  |......o....V...7|
00007110  00 00 00 00 00 00 21 67  ac 70 22 ec ca b8 70 11  |......!g.p"...p.|
00007120  03 07 0d 33 ed 77 28 f9  15 0a 00 00 00 00 05 00  |...3.w(.........|
00007130  00 00 01 00 00 00 0b 2d  00 00 03 bd b0 50 67 e9  |.......-.....Pg.|
00007140  00 00 00 00 00 00 15 00  00 00 00 00 00 00 18 00  |................|
00007150  00 00 00 00 00 00 70 11  03 07 0d 33 ed 77 73 79  |......p....3.wsy|
00007160  73 74 65 6d 5f 64 61 74  61 5f 30 30 30 31 2e 70  |stem_data_0001.p|
00007170  6b 67 00                                          |kg.|
00007173
(END)
```


Right away, we can notice 3 things:

* like before, it looks cyclical
* it contains the IDs of the pkg file
* it ends with the `.pkg` file name.

One cycle probably contains other metadata about a packaged file.

Lets try to first determine the size of these cycles.

first, lets "reallign" the hexdump.

Here the last file name strings ends at 00003c05 (last '\0'), which means 6 bytes.

hexdump has a convinient `-s` (skip) option for that.

```shell
# lets skip the first 6 bytes to allign hexdump output
kakwa@linux 6775398/idx » hexdump -s 6 -C system_data.idx| less
[...]
00003be6  6f 6e 5f 64 75 6d 6d 79  2e 64 64 73 00 77 61 76  |on_dummy.dds.wav|
00003bf6  65 73 5f 68 65 69 67 68  74 73 30 2e 64 64 73 00  |es_heights0.dds.|
00003c06  8f 0c 9a ba 4f 40 b6 93  70 11 03 07 0d 33 ed 77  |....O@..p....3.w|
00003c16  00 00 00 00 00 00 00 00  05 00 00 00 01 00 00 00  |................|
00003c26  f5 21 00 00 bf 00 45 5c  6c 36 00 00 00 00 00 00  |.!....E\l6......|
00003c36  8f ec 87 4a 28 d0 f7 c7  70 11 03 07 0d 33 ed 77  |...J(...p....3.w|
00003c46  1e 9b ef 05 00 00 00 00  05 00 00 00 01 00 00 00  |................|
00003c56  15 15 01 00 03 77 63 97  3e ab 02 00 00 00 00 00  |.....wc.>.......|
00003c66  ad 70 a2 e7 ac 2c 4f 6b  70 11 03 07 0d 33 ed 77  |.p...,Okp....3.w|
[...]
```

Much nicer!

With that, we immediately notice some cycle, with the `70 11 03 07 0d 33 ed 77` value.

There are 6 x 64 = 384 bits between each `70 11 03 07 0d 33 ed 77` value.

let's try to confirm that with the IDs.

We are spotting the `bf 00 45 5c 6c 36` seen previously in the `.pkg` file. 384 bits later, we see `03 77 63 97 3e ab 02`.

With a bit of digging (it's right in the middle of the `.pkg` file, and this ID is not neetly in a 64 bits aligned chunk because of the variable size of the Deflate blocks), we indeed find it.

We have indeed a 384 bits cycle, which neetly fits in 3 lines of hexdump!

So each of these is one record:

```
00003c06  8f 0c 9a ba 4f 40 b6 93  70 11 03 07 0d 33 ed 77  |....O@..p....3.w|
00003c16  00 00 00 00 00 00 00 00  05 00 00 00 01 00 00 00  |................|
00003c26  f5 21 00 00 bf 00 45 5c  6c 36 00 00 00 00 00 00  |.!....E\l6......|
```

```
00003c36  8f ec 87 4a 28 d0 f7 c7  70 11 03 07 0d 33 ed 77  |...J(...p....3.w|
00003c46  1e 9b ef 05 00 00 00 00  05 00 00 00 01 00 00 00  |................|
00003c56  15 15 01 00 03 77 63 97  3e ab 02 00 00 00 00 00  |.....wc.>.......|
```

```
00003c66  ad 70 a2 e7 ac 2c 4f 6b  70 11 03 07 0d 33 ed 77  |.p...,Okp....3.w|
00003c76  05 22 00 00 00 00 00 00  05 00 00 00 01 00 00 00  |."..............|
00003c86  cb 01 00 00 6d b9 de c1  ad 0c 00 00 00 00 00 00  |....m...........|
```

```
00003c96  8e c7 6a 58 7c 86 62 33  70 11 03 07 0d 33 ed 77  |..jX|.b3p....3.w|
00003ca6  e0 23 00 00 00 00 00 00  05 00 00 00 01 00 00 00  |.#..............|
00003cb6  65 00 00 00 f1 d2 87 5a  d2 00 00 00 00 00 00 00  |e......Z........|
```

```
00003cc6  8e 43 3d e9 cf 49 52 a4  70 11 03 07 0d 33 ed 77  |.C=..IR.p....3.w|
00003cd6  4d 1f 6a 05 00 00 00 00  05 00 00 00 01 00 00 00  |M.j.............|
00003ce6  bb 19 00 00 f7 53 4a b1  38 ab 00 00 00 00 00 00  |.....SJ.8.......|
```

All these records are from the same file,

lets grab a few from other files:

File 2:

```
0000f6fd  58 fe 65 b4 59 b6 b0 77  a4 8c 78 6a 58 aa 65 84  |X.e.Y..w..xjX.e.|
0000f70d  00 00 00 00 00 00 00 00  05 00 00 00 01 00 00 00  |................|
0000f71d  bc 99 05 00 f4 3a 67 8b  80 00 08 00 00 00 00 00  |.....:g.........|
```

```
0000f72d  a0 e5 c7 22 cc 49 d3 31  a4 8c 78 6a 58 aa 65 84  |...".I.1..xjX.e.|
0000f73d  3e 9e 7e 05 00 00 00 00  05 00 00 00 01 00 00 00  |>.~.............|
0000f74d  79 c6 03 00 dc b8 5f 80  80 00 08 00 00 00 00 00  |y....._.........|
```


File 3:

```
00002d0c  ed cf 33 f8 a5 94 53 56  0d a7 9c b9 bf 60 f5 3e  |..3...SV.....`.>|
00002d1c  40 a8 08 07 00 00 00 00  00 00 00 00 00 00 00 00  |@...............|
00002d2c  38 ab 00 00 5d cf 4e b6  38 ab 00 00 00 00 00 00  |8...].N.8.......|
```

```
00002d3c  ed ea da 52 4e 8f 70 ed  0d a7 9c b9 bf 60 f5 3e  |...RN.p......`.>|
00002d4c  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
00002d5c  98 00 00 00 a4 63 f2 9b  98 00 00 00 00 00 00 00  |.....c..........|
```

Lets study:

```
00003c06  8f 0c 9a ba 4f 40 b6 93  70 11 03 07 0d 33 ed 77  |....O@..p....3.w|
00003c16  00 00 00 00 00 00 00 00  05 00 00 00 01 00 00 00  |................|
00003c26  f5 21 00 00 bf 00 45 5c  6c 36 00 00 00 00 00 00  |.!....E\l6......|
```

For the first 128 bits we get:

```
00003c06  8f 0c 9a ba 4f 40 b6 93  70 11 03 07 0d 33 ed 77  |....O@..p....3.w|
```

From the fact the last 64 bits are constant, we can deduce we have probably two 64 bits integers in the first 128 bits

Once again, these are using all the available bits and seem rather random. It's difficult to link these to their role, so... lets simply ignore these for now.

```
+====+====+====+====+====+====+====+====++====+====+====+====+====+====+====+====+
| UO | UO | UO | UO | UO | UO | UO | UO || UT | UT | UT | UT | UT | UT | UT | UT |
+====+====+====+====+====+====+====+====++====+====+====+====+====+====+====+====+
|<------------ unknown 1 -------------->||<------------- unknow 2 -------------->|
|               64 bits                 ||               64 bits                 |
```

Next 64 bits, we have a low value 64 bits integer, so most likely an offset. It is also suspiciously at `0x0` for the first record which is also the first data chunk in the `.pkg` file.

So, it's most likely the start of a data chunk in a `.pkg` file. Looking at other records confirms that.

Next, we have two extremely low value 32 bits (0x5 and 0x1). So once again, most likely some kind of enum, lets call them `type1` and `type2`.

```
+====+====+====+====+====+====+====+====++====+====+====+====++====+====+====+====+
| OF | OF | OF | OF | OF | OF | OF | OF || T1 | T1 | T1 | T1 || T2 | T2 | T2 | T2 |
+====+====+====+====+====+====+====+====++====+====+====+====++====+====+====+====+
|<----------start offset pkg ---------->||<---- type 1 ----->||<----- type 2 ---->|
|               64 bits                 ||     32 bits       ||      32 bits      |
```

For the Last 128 bits, we get the following:
```
00003c26  f5 21 00 00 bf 00 45 5c  6c 36 00 00 00 00 00 00  |.!....E\l6......|
```

So, here, we see our `.pkg` ID (`bf 00 45 5c  6c 36`) right in the middle. Given its size, this ID is probably stored on 64 bits.

After that, last 32 bits, we get a bunch of `00`, maybe a reserved field, but more likely some kind of padding.

Before that, we get a low value 32 bits integer. When comparing with the `.pkg` file, `f5 21 00 00` is the offset where the first data chunk ends.


So it's the end offset. But 32 bits seems rather small to store such offset (specially given the start offset is 64 bits. Also, for other data chunks, this doesn't line-up.

However, it could very much be a relative offset (to the start of the data chunk).

Lets validate that with the second record

Record
``` 
00003c36  8f ec 87 4a 28 d0 f7 c7  70 11 03 07 0d 33 ed 77  |...J(...p....3.w|
00003c46  1e 9b ef 05 00 00 00 00  05 00 00 00 01 00 00 00  |................|
00003c56  15 15 01 00 03 77 63 97  3e ab 02 00 00 00 00 00  |.....wc.>.......|
```
More hexdump! (searching the data chunk using the `03 77 63 97  3e ab 02` ID and the start offset `1e 9b ef 05 00 00 00 00`, or `0x05ef9b1e` once we take endianess into account):

```shell
kakwa@linux World of Warships/res_packages » hexdump -C system_data_0001.pkg | less
[...]
05ef9b10  00 00 17 4b 28 42 80 40  00 00 00 00 00 00 8c 7d  |...K(B.@.......}|
05ef9b20  3b 50 5d d9 b6 dd 7d b6  3e 76 15 b2 5f 20 89 04  |;P]...}.>v.._ ..|
05ef9b30  55 bd 00 44 82 aa ec 7a  9c bd f7 79 45 57 39 40  |U..D...z...yEW9@|
05ef9b40  90 d0 4e 8c c0 01 9d 21  48 e8 0c 41 a2 ce 10 24  |..N....!H..A...$|
[...]
05f0b010  d1 d7 52 b0 de cc fa 9c  2b 8d b5 57 a4 02 ff 1a  |..R.....+..W....|
05f0b020  43 5d 2d 43 3f cc d1 0b  f8 88 92 a8 9f 5a 70 64  |C]-C?........Zpd|
05f0b030  46 fb 7f 00 00 00 00 03  77 63 97 3e ab 02 00 00  |F.......wc.>....|
05f0b040  00 00 00 94 b7 67 90 db  68 9e e6 d9 1f 36 62 6f  |.....g..h....6bo|
05f0b050  6f 22 76 f6 62 e3 62 77  67 7a 7a ba ab bb 8c aa  |o"v.b.bwgzz.....|
05f0b060  4a 52 95 5c ca a5 cf 64  d2 24 bd 27 41 d0 00 04  |JR.\...d.$.'A...|
[...]
```

We indeed find the start of our data chunk at `05ef9b1e` (`8c` after a bunch of `00` on the first line).

And looking for the `00 00 00 00 ID ID [...]` pattern in between the data chunks, we can determine the end of this chunk to be at `0x05f0b032`.

Doing `0x05f0b032 - 0x05ef9b1e`, we get `0x11514`, that's almost our `15 15 01 00` once we swap endianess, and add `1`.

```
+====+====+====+====++====+====+====+====+====+====+====+====++====+====+====+====+
| OE | OE | OE | OE || ID | ID | ID | ID | ID | ID | ID | ID || 00 | 00 | 00 | 00 |
+====+====+====+====++====+====+====+====+====+====+====+====++====+====+====+====+
|<-- offset end --->||<------------- ID '.pkg' ------------->||<---- padding ---->|
|     32 bits       ||               64 bits                 ||      32 bits      |
```

So to recap, we have:

```
+====+====+====+====+====+====+====+====++=====+====+====+====+====+====+====+====+
| UO | UO | UO | UO | UO | UO | UO | UO ||  UT | UT | UT | UT | UT | UT | UT | UT |
+====+====+====+====+====+====+====+====++=====+====+====+====+====+====+====+====+
|<------------ unknown 1 -------------->||<-------------- unknown 2 ------------->|
|               64 bits                 ||                64 bits                 |
+====+====+====+====+====+====+====+====++====+====+====+====++====+====+====+====+
| OF | OF | OF | OF | OF | OF | OF | OF || T1 | T1 | T1 | T1 || T2 | T2 | T2 | T2 |
+====+====+====+====+====+====+====+====++====+====+====+====++====+====+====+====+
|<--------- start offset pkg ---------->||<---- type 1 ----->||<----- type 2 ---->|
|               64 bits                 ||     32 bits       ||      32 bits      |
| OE | OE | OE | OE || ID | ID | ID | ID | ID | ID | ID | ID || 00 | 00 | 00 | 00 |
+====+====+====+====++====+====+====+====+====+====+====+====++====+====+====+====+
|<-- offset end --->||<------------- ID '.pkg' ------------->||<---- padding ---->|
|     32 bits       ||               64 bits                 ||      32 bits      |
```

### The last bits

So, okay, we have the core of the 3 part of the file.

But we can see the last few bits don't follow this pattern (specially with the `.pkg` file name), which means we have a footer:

```
# Last block
00007116  21 67 ac 70 22 ec ca b8  70 11 03 07 0d 33 ed 77  |!g.p"...p....3.w|
00007126  28 f9 15 0a 00 00 00 00  05 00 00 00 01 00 00 00  |(...............|
00007136  0b 2d 00 00 03 bd b0 50  67 e9 00 00 00 00 00 00  |.-.....Pg.......|
# Footer
00007146  15 00 00 00 00 00 00 00  18 00 00 00 00 00 00 00  |................|
00007156  70 11 03 07 0d 33 ed 77  73 79 73 74 65 6d 5f 64  |p....3.wsystem_d|
00007166  61 74 61 5f 30 30 30 31  2e 70 6b 67 00           |ata_0001.pkg.|
```

If we look at the content, we have 3 * 64 bits before the name starts (`73 79 73 74 65 6d 5f 64` for `system_d` in)

Looking at it more closely, given all the `00`, it seems we have three 64 bits integers there.

* `15 00 00 00 00 00 00 00`
* `18 00 00 00 00 00 00 00`
* `70 11 03 07 0d 33 ed 77`

`70 11 03 07 0d 33 ed 77` is the "unknown 2" we saw previously, so still no luck, but lets name it the same way.


`18 00 00 00 00 00 00 00` seems to have the same value accross all files, so probably not that important.

The only one that varies accross files is the `15 00 00 00 00 00 00 00`, but always in the same kind of values around 0x15. in decimal it's 21.

Strangely, `system_data_0001.pkg` is 20 chars long, 21 if we include the `\0` at the end.

So we can deduce it's actually the `.pkg` file name string size.

So we have

```
+====+====+====+====+====+====+====+====++=====+====+====+====+====+====+====+====+
| UO | UO | UO | UO | UO | UO | UO | UO ||  U3 | U3 | U3 | U3 | U3 | U3 | U3 | U3 |
+====+====+====+====+====+====+====+====++=====+====+====+====+====+====+====+====+
|<--------- size pkg file name -------->||<-------------- unknown 3 ------------->|
|               64 bits                 ||                64 bits                 |
+=====+====+====+====+====+====+====+====+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~...
|  UT | UT | UT | UT | UT | UT | UT | UT |             file name string
+=====+====+====+====+====+====+====+====+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~...
|<-------------- unknown 2 ------------->|
|                64 bits                 |
```

## Time to start the implementation!
