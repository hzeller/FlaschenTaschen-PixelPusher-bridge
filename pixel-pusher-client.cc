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

#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <stdio.h>
#include <math.h>

// Either with hostname or IP address. Assuming port 5078.
static int OpenPPSocket(const char *host) {
    struct addrinfo addr_hints = {0};
    addr_hints.ai_family = AF_INET;
    addr_hints.ai_socktype = SOCK_DGRAM;

    struct addrinfo *addr_result = NULL;
    int rc;
    if ((rc = getaddrinfo(host, "5078", &addr_hints, &addr_result)) != 0) {
        fprintf(stderr, "Resolving '%s': %s\n", host, gai_strerror(rc));
        return -1;
    }
    if (addr_result == NULL)
        return -1;
    int fd = socket(addr_result->ai_family,
                    addr_result->ai_socktype,
                    addr_result->ai_protocol);
    if (fd >= 0 &&
        connect(fd, addr_result->ai_addr, addr_result->ai_addrlen) < 0) {
        perror("connect()");
        close(fd);
        fd = -1;
    }

    freeaddrinfo(addr_result);
    return fd;
}

PixelPusherClient::PixelPusherClient(int strip_len, int strips,
                                     const char *pp_host,
                                     int max_transmit_bytes, int brightness)
    : width_(strip_len), height_(strips),
      socket_(OpenPPSocket(pp_host)),
      // Each row is one byte longer, because we use the first byte to
      // indicate the strip-index which we use in the
      row_size_(1 + sizeof(Color) * width_),
      rows_per_packet_((max_transmit_bytes - 4) / row_size_),
      brightness_percent_(brightness),
      pixel_buffer_(new char[row_size_ * height_]),
      sequence_number_(0) {
    // Prepare the pixel index at the beginning of each row
    memset(pixel_buffer_, 0, row_size_ * height_);
    for (uint8_t i = 0; i < height_; ++i) {
        pixel_buffer_[i * row_size_] = i;
    }
    if (rows_per_packet_ == 0) {
        fprintf(stderr, "Doh', can't even send a single row per packet ("
                "one row=%d bytes, but max packet-size=%d!)\n",
                (int)row_size_, max_transmit_bytes);
        abort();
    }
    int packets_needed = (height_ + rows_per_packet_ - 1) / rows_per_packet_;
    fprintf(stderr, "PP %s: %dx%d; %d UDP packets per frame "
            "(packet allows for max %d rows @ %d bytes/row)\n",
            pp_host, width_, height_, packets_needed, rows_per_packet_,
            (int)row_size_);
}

PixelPusherClient::~PixelPusherClient() {
    delete [] pixel_buffer_;
}

static int kBitPlanes = 8;

// Do CIE1931 luminance correction and scale to output bitplanes
static uint16_t luminance_cie1931(uint8_t c, uint8_t brightness) {
  float out_factor = ((1 << kBitPlanes) - 1);
  float v = (float) c * brightness / 255.0;
  return out_factor * ((v <= 8) ? v / 902.3 : pow((v + 16) / 116.0, 3));
}

struct ColorLookup {
  uint16_t color[256];
};
static ColorLookup *CreateLuminanceCIE1931LookupTable() {
  ColorLookup *for_brightness = new ColorLookup[100];
  for (int c = 0; c < 256; ++c)
    for (int b = 0; b < 100; ++b)
      for_brightness[b].color[c] = luminance_cie1931(c, b + 1);

  return for_brightness;
}

static inline Color CIEMapColor(uint8_t brightness, const Color& col) {
    static ColorLookup *luminance_lookup = CreateLuminanceCIE1931LookupTable();
    return Color(luminance_lookup[brightness - 1].color[col.r],
                 luminance_lookup[brightness - 1].color[col.g],
                 luminance_lookup[brightness - 1].color[col.b]);
}

void PixelPusherClient::SetPixel(int x, int y, const Color &col) {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) return;
    Color *row = (Color*) (pixel_buffer_ + y*row_size_ + 1);
    row[x] = CIEMapColor(brightness_percent_, col);
}


bool PixelPusherClient::SendPacket(int index) {
    if (socket_ < 0) return false;
    const size_t block_size = rows_per_packet_ * row_size_;
    const size_t start = index * block_size;
    const size_t end = height_ * row_size_;
    if (start >= end) return false;

    // Each packet has a sequence number in the beginning, but our pixel
    // buffer if contiguous. So we stuff it in front atomically with writev().
    iovec parts[2];
    parts[0].iov_base = &sequence_number_;
    parts[0].iov_len = sizeof(sequence_number_);
    parts[1].iov_base = pixel_buffer_ + start;
    parts[1].iov_len = std::min(end - start, block_size);
    writev(socket_, parts, 2);

    sequence_number_++;
    return (index + 1) * block_size < end;
}

void PixelPusherClient::Send() {
    for (int i = 0; SendPacket(i); ++i)
        ;
}
