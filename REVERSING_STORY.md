# Introduction

First, a disclaimer, this is the first time I'm doing this kind of exercise, so the process described here is probably not the best approach.

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

Looking at the end of the file, we have this pattern repeating one final time:

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
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
|                                                                                               |
|                              Compressed Data (RFC 1951/Deflate)                               |
|                                                                                               |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
+====++====++====++====+ +====++====++====++====+====++====++====++====+ +====++====++====++====+
| 00 || 00 || 00 || 00 | | XX || XX || XX || XX | XX || XX || 00 || 00 | | 00 || 00 || 00 || 00 |
+====++====++====++====+ +====++====++====++====+====++====++====++====+ +====++====++====++====+
|<----- 0 padding ---->| |<--------- some kind of ID ----------------->| <----- 0 padding ----->|
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

kakwa@tsingtao Games/World of Warships » find ./ -type f | grep -v cef | grep -v replays | grep -v crashes | grep -v '.pkg' | grep -v '.dll' | grep -v '.log'  | grep -v '\.exe' | less
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
kakwa@tsingtao bin/6775398/idx » strings -n 5 system_data.idx
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

### bin directory and Game versions

There are a few things to note about the `./bin` directory: it contains several sub-directory looking like that:

```shell
kakwa@tsingtao World of Warships/bin » du -hd 1 | sort -h
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

### Index (.idx) file format
