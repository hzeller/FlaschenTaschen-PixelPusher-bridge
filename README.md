A bridge that receives FlaschenTaschen protocol and sends it to a PixelPusher
installation.

First step: super simple, does not read beacons, just assumes hard-coded
IP addresses and sizes of connected PixelPushers.

This is very specific to BJK's installation: Code needs to be modified
in [bjk-pixel-pusher.cc](./bjk-pixel-pusher.cc) to set the
IP-addresses/hostnames of the pixel pushers, and possibly adapt the
SetPixel() to properly reflect the mapping.

```
usage: ./ft-server [options]
Options:
        -d                   : Become daemon
        --layer-timeout <sec>: Layer timeout: clearing after non-activity (Default: 15)
```

Then, the standard flaschen taschen tools can be used to send images and stuff
to this installation

```
  send-image -h <host-where-ft-server-is-running> -g120x48 foo.gif
```