## CCExtractor
check AUTHORS.TXT for history and developers

## License
GPL 2.0. 


## Description
Since the original port, the whole code has been rewritten (more than once,
one might add) and support for most subtitle formats around the world has
been added (teletext, DVB, CEA-708, ISDB...)

## Basic Usage 
(please run ccextractor with no parameters for the complete manual -
this is for your convenience, really).

ccextractor reads a video stream looking for closed captions (subtitles).
It can do two things:

- Save the data to a "raw", unprocessed file which you can later use
  as input for other tools, such as McPoodle's excellent suite. 
- Generate a subtitles file (.srt,.smi, or .txt) which you can directly 
  use with your favourite player.

Running ccextractor without parameters shows the help screen. Usage is 
trivial - you just need to pass the input file and (optionally) some
details about the input and output files.


## Languages
Usually English captions are transmitted in line 21 field 1 data,
using channel 1, so the default values are correct so you don't
need to do anything and you don't need to understand what it all
means.

If you want the Spanish captions, you may need to play a bit with
the parameters. From what I've been, Spanish captions are usually
sent in field 2, and sometimes in channel 2. 

So try adding these parameter combinations to your other parameters.

-2 
-cc2
-2 -cc2

If there are Spanish subtitles, one of them should work. 

## McPoodle's page
http://www.theneitherworld.com/mcpoodle/SCC_TOOLS/DOCS/SCC_TOOLS.HTML

Essential CC related information and free (with source) tools.

## Encoding
This version, in both its Linux and Windows builds generates by
default Unicode files. You can use -latin1 and -utf8 if you prefer 
these encodings (usually it just depends on what your specific
player likes).

## Future work
- Please check www.ccextractor.org for news and future work.

