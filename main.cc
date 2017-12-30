// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation version 2.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://gnu.org/licenses/gpl-2.0.txt>

// Flaschen Taschen Server

#include <arpa/inet.h>
#include <errno.h>
#include <getopt.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <string>

#include "composite-flaschen-taschen.h"
#include "ft-thread.h"
#include "led-flaschen-taschen.h"
#include "ft-server.h"

// Max packet size can be up to 64k with UDP, however, the physical pixel
// pusher has some limit much lower than that due to lower memory.
// Not sure what it is, might be worthwhile to increase until it stops working.
static const int kDefaultPacketSize = 1460;

static const int kMaxUDPPacketSize = 65507;  // largest practical w/ IPv4 header

static int usage(const char *progname) {
    fprintf(stderr, "usage: %s [options]\n", progname);
    fprintf(stderr, "Options:\n"
            "\t-u <udp-size>        : Maximum UDP to PixelPusher. Default %d.\n"
            "\t-b <brightness%%>     : Percent brightness. Default: 100\n"
            "\t-d                   : Become daemon\n"
            "\t--layer-timeout <sec>: Layer timeout: clearing after non-activity (Default: 15)\n", kDefaultPacketSize);
    return 1;
}

int main(int argc, char *argv[]) {
    int layer_timeout = 15;
    bool as_daemon = false;
    int udp_packet_size = kDefaultPacketSize;
    int brightness = 100;

    enum LongOptionsOnly {
        OPT_LAYER_TIMEOUT = 1002,
    };

    static struct option long_options[] = {
        { "daemon",             no_argument,       NULL, 'd'},
        { "layer-timeout",      required_argument, NULL,  OPT_LAYER_TIMEOUT },
        { 0,                    0,                 0,    0  },
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "I:du:b:", long_options, NULL)) != -1) {
        switch (opt) {
        case 'd':
            as_daemon = true;
            break;
        case OPT_LAYER_TIMEOUT:
            layer_timeout = atoi(optarg);
            break;
        case 'u':
            udp_packet_size = atoi(optarg);
            break;
        case 'b':
            brightness = atoi(optarg);
            break;
        default:
            return usage(argv[0]);
        }
    }

    if (layer_timeout < 1) {
        layer_timeout = 1;
    }
    if (brightness < 0) brightness = 0;
    if (brightness > 100) brightness = 100;

    if (udp_packet_size < 0 || udp_packet_size > kMaxUDPPacketSize) {
        fprintf(stderr, "-u %d is outside usable packet size of 0..%d\n",
                udp_packet_size, kMaxUDPPacketSize);
        return usage(argv[0]);
    }

    FlaschenTaschen *display = new BJKPixelPusher(udp_packet_size, brightness);

    // Start all the services and report problems (such as sockets already
    // bound to) before we become a daemon
    if (!udp_server_init(1337)) {
        return 1;
    }

    fprintf(stderr, "Sending result to %dx%d PixelPusher installation\n",
            display->width(), display->height());

    // Commandline parsed, immediate errors reported. Time to become daemon.
    if (as_daemon && daemon(0, 0) != 0) {  // Become daemon.
        perror("Failed to become daemon");
    }

    display->Send();  // Clear screen.

    ft::Mutex mutex;

    // The display we expose to the user provides composite layering which can
    // be used by the UDP server.
    CompositeFlaschenTaschen layered_display(display, 16);
    layered_display.StartLayerGarbageCollection(&mutex, layer_timeout);

    udp_server_run_blocking(&layered_display, &mutex);  // last server blocks.
    delete display;
}
