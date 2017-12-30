// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
//
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

#include "led-flaschen-taschen.h"

#include <assert.h>
#include <unistd.h>

// =======  Hardcoded configuration ============================================
// BJK, these are just assumptions by me. Adapt to your set-up and make sure
// the mapping below in SetPixel() works.
// Hostnames or IP addresses need to be hardcoded.
static const int kPixelPusherCount   = 3;
static const int kStripsPerPP        = 8;
static const int kStripLen           = 240;
static const int kZigZagCount        = 5;
static const char *kPixelPusherPort = "9897";

static const char *kPixelPusherHost[kPixelPusherCount] = {
    "192.168.88.20",
    "192.168.88.21",
    "192.168.88.22",
};

// =============================================================================

BJKPixelPusher::BJKPixelPusher(int max_udp_packet_size, int brightness) {
    for (int i = 0; i < kPixelPusherCount; ++i) {
        clients_.push_back(
            new PixelPusherClient(kStripLen, kStripsPerPP,
                                  kPixelPusherHost[i], kPixelPusherPort,
                                  max_udp_packet_size, brightness));
    }
}

BJKPixelPusher::~BJKPixelPusher() {
    for (size_t i = 0; i < clients_.size(); ++i) {
        delete clients_[i];
    }
}

int BJKPixelPusher::width() const {
    return kPixelPusherCount * kStripsPerPP * kZigZagCount;
}
int BJKPixelPusher::height() const {
    return kStripLen / kZigZagCount;
}

void BJKPixelPusher::SetPixel(int x, int y, const Color &col) {
    if (x < 0 || x >= width() || y < 0 || y >= height()) return;

    // BJK, here you need to do the mapping. This is just roughtly from
    // what I assume things are, but I might be off.

    const int columns_per_pp = kStripsPerPP * kZigZagCount;
    const int pp_index = x / columns_per_pp;
    assert(pp_index >= 0 && pp_index < (int)clients_.size());
    x %= columns_per_pp;

    int strip = x / kZigZagCount;
    int strip_offset = x % kZigZagCount * height();
    int strip_pos = ((x + strip) % 2 == 0) ? y : height() - 1 - y; // Zigzag
    clients_[pp_index]->SetPixel(strip_offset + strip_pos, strip, col);
}

void BJKPixelPusher::Send() {
    bool need_more_packets = true;
    for (int packet = 0; need_more_packets; ++packet) {
        need_more_packets = false;
        for (size_t i = 0; i < clients_.size(); ++i) {
            need_more_packets |= clients_[i]->SendPacket(packet);
        }
        usleep(3000);  // PixelPushers need some time to digest
    }
}
