#include "utf8.h"

int utf8_glyph_len(unsigned char ch) {
    if (ch < 128) {
        return 1;
    }
    else if (ch >> 5 == 0b110) {
        return 2;
    }
    else if (ch >> 4 == 0b1110) {
        return 3;
    }
    else if (ch >> 3 == 0b11110) {
        return 4;
    }
    return -1;
}

int utf8_codepoint_to_bytes(uint32_t cp, char *buf) {
    switch (cp) {
        case 0x0000 ... 0x007F:
            buf[0] = cp;
            return 1;
        case 0x0080 ... 0x07FF:
            buf[0] = 0b11000000 | (0b00011111 & (cp >> 6));
            buf[1] = 0b10000000 | (0b00111111 & (cp >> 0));
            return 2;
        case 0x0800 ... 0xFFFF:
            buf[0] = 0b11100000 | (0b00001111 & (cp >> 12));
            buf[1] = 0b10000000 | (0b00111111 & (cp >> 6));
            buf[2] = 0b10000000 | (0b00111111 & (cp >> 0));
            return 3;
        case 0x10000 ... 0x10FFFF:
            buf[0] = 0b11110000 | (0b00000111 & (cp >> 18));
            buf[1] = 0b10000000 | (0b00111111 & (cp >> 12));
            buf[2] = 0b10000000 | (0b00111111 & (cp >> 6));
            buf[3] = 0b10000000 | (0b00111111 & (cp >> 0));
            return 4;
        default:
            return -1;
    }
}

int utf8_bytes_to_codepoint(const char *buf, u_int32_t *cp) {
    int length = utf8_glyph_len(*buf);
    switch (length) {
        case 1:
            *cp = (((*(buf + 0)) & 0b00111111) << 0);
            break;
        case 2:
            *cp = (((*(buf + 0)) & 0b00011111) << 6) +
                (((*(buf + 1)) & 0b00111111) << 0);
            break;
        case 3:
            *cp = (((*(buf + 0)) & 0b00001111) << 12) +
                (((*(buf + 1)) & 0b00111111) << 6) +
                (((*(buf + 2)) & 0b00111111) << 0);
            break;
        case 4:
            *cp = (((*(buf + 0)) & 0b00000111) << 18) +
                (((*(buf + 1)) & 0b00111111) << 12) +
                (((*(buf + 2)) & 0b00111111) << 6) +
                (((*(buf + 3)) & 0b00111111) << 0);
            break;
    }
    return length;
}
