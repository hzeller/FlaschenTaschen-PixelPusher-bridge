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

#ifndef LED_FLASCHEN_TASCHEN_H_
#define LED_FLASCHEN_TASCHEN_H_

#include "flaschen-taschen.h"

#include <vector>
#include <string>

class PixelPusherClient : public FlaschenTaschen {
public:
    // Send pixels to particular socket with maximum bytes. This implements
    // the FlaschenTaschen client interface and simulate a screen which is
    // "strip_len" wide and "strips" height.
    // The "max_transmit_bytes" is needed as the standard embedded PixelPusher
    // only supports very small UDP packets (8kBytes or even 1460 bytes or so).
    // So with smaller max_transmit_bytes, the Send() method will send more than
    // one packet.
    // "pp_host" is the name of the pixel pusher installation, either hostname
    // or IP address. Data will be send to port 5078.
    PixelPusherClient(int strip_len, int strips,
                      const char *pp_host, int max_transmit_bytes);
    virtual ~PixelPusherClient();

    virtual int width() const { return width_; }
    virtual int height() const { return height_; }

    virtual void SetPixel(int x, int y, const Color &col);
    virtual void Send();

private:
    const int width_;
    const int height_;

    const int socket_;

    const size_t row_size_;
    const int rows_per_packet_;

    char *pixel_buffer_;
    uint32_t sequence_number_;
};

class BJKPixelPusher : public FlaschenTaschen {
public:
    BJKPixelPusher(int udp_packet_size);
    virtual ~BJKPixelPusher();

    virtual int width() const;
    virtual int height() const;

    virtual void SetPixel(int x, int y, const Color &col);
    virtual void Send();

private:
    std::vector<PixelPusherClient *> clients_;
};

#endif // LED_FLASCHEN_TASCHEN_H_
