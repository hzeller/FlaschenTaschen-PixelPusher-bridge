FlaschenTaschen -&gt; PixelPusher Bridge
========================================

(Unless you are bjk or patrickod at the 33C3, this is probably too specific.
Nothing to see here in that case).

A bridge that receives FlaschenTaschen protocol and sends it to a PixelPusher
installation.

First step: _super simple_, does _not_ use the PP discovery protocol, just
assumes hard-coded IP addresses and sizes of connected PixelPushers.

### Configuration needed

This is very specific to BJK's installation: Code needs to be modified
in [bjk-pixel-pusher.cc](./bjk-pixel-pusher.cc) to set the
IP-addresses/hostnames of the pixel pushers, and possibly adapt the
SetPixel() to properly reflect the mapping.

### Operation
```
usage: ./ft-server [options]
Options:
        -u <udp-size>        : Maximum UDP to PixelPusher. Default 1460.
        -d                   : Become daemon
        --layer-timeout <sec>: Layer timeout: clearing after non-activity (Default: 15)
```

This can run as any use, does not need to be root.

### Clients

Then, the standard flaschen-taschen tools (
https://github.com/hzeller/flaschen-taschen/tree/master/client ) can be used
to send images and other content to this installation, by pointing them to this
bridge host:

```
  # git clone the flaschen-taschen repo.
  cd flaschen-taschen/client
  make send-text send-image send-video
  ./send-image -h <ft-bridge-host> -g120x48 my-animated.gif
  ./send-text  -h <ft-bridge-host> -g120x48 -l15 -cffff00 -o0000ff -f path/to/font.bdf "Hello world"
```

Also all these nice demos by Carl: https://github.com/cgorringe/ft-demos

Instead of `-h <bridge-host>`, often it is more convenient to set the environment
variable `export FT_DISPLAY=<bridge-host>`.

Other tools also started to speak FlaschenTaschen. The latest
[vlc compiled from git](https://wiki.videolan.org/UnixCompile/),
can play videos via FlaschenTaschen:

```
vlc --vout flaschen --flaschen-display=<ft-bridge-host> \
     --flaschen-width=120 --flaschen-height=48 <filename or youtube url>
```

#### Tweaking
Due to limited memory and IP fragment re-assembly, Jas' physical PixelPusher
has some relatively small limit on the UDP size, not sure what it is.
Right now, we conservatively assume 1460 bytes.

It should work fine, but that requires four UDP packets per update of 240x8.
Might be interesting to tweak the `-u` parameter to see if the PP can accept
larger packets.
A size &gt;= 5772 would be ideal, because then we can get 8 rows with 240 LEDs
out at once.
Note, there is also a `stripsperpacket` configuration in the
PixelPusher `pixel.rc` configuration file, so that might need to be modified
alongside to 8.

Don't worry about tweaking if there is no lagging issue.
