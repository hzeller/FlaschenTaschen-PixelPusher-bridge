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
#include <netdb.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

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
                                     const char *pp_host, int max_transmit_bytes)
    : width_(strip_len), height_(strips),
      socket_(OpenPPSocket(pp_host)),
      // Each row is one byte longer, because we use the first byte to
      // indicate the strip-index which we use in the
      row_size_(1 + sizeof(Color) * width_),
      rows_per_packet_(max_transmit_bytes / row_size_),
      pixel_buffer_(new char[row_size_ * height_]),
      sequence_number_(0) {
    // Prepare the pixel index at the beginning of each row
    memset(pixel_buffer_, 0, row_size_ * height_);
    for (uint8_t i = 0; i < height_; ++i) {
        pixel_buffer_[i * row_size_] = i;
    }
    if (rows_per_packet_ == 0) {
        fprintf(stderr, "Doh', can't even send a single row per packet ("
                "one row=%d bytes, but max packet-size=%d!\n",
                (int)row_size_, max_transmit_bytes);
        assert(0);
    }
    fprintf(stderr, "PP %s: %dx%d; row-bytes = %d\n",
            pp_host, width_, height_, (int)row_size_);
}

PixelPusherClient::~PixelPusherClient() {
    delete [] pixel_buffer_;
}

void PixelPusherClient::SetPixel(int x, int y, const Color &col) {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) return;
    Color *row = (Color*) (pixel_buffer_ + y*row_size_ + 1);
    row[x] = col;
}

void PixelPusherClient::Send() {
    if (socket_ < 0) return;
    size_t remaining = height_ * row_size_;
    char *pixel_pos = pixel_buffer_;

    // Each packet has a sequence number in the beginning, but our pixel
    // buffer if contiguous. So we stuff it in front atomically with writev().
    iovec parts[2];
    parts[0].iov_base = &sequence_number_;
    parts[0].iov_len = sizeof(sequence_number_);
    while (remaining > 0) {
        parts[1].iov_base = pixel_pos;
        parts[1].iov_len = std::min(remaining, rows_per_packet_ * row_size_);

        writev(socket_, parts, 2);

        pixel_pos += parts[1].iov_len;
        remaining -= parts[1].iov_len;
        sequence_number_++;
    }
}
