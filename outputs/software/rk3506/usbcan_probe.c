/* Experimental userspace probe for the 0471:1200 dual USB-CAN adapter.
 * Protocol was inferred from a CANTest 2.69 capture, not vendor documentation.
 * Scope: physical CAN1 and CAN2, 500 kbit/s, standard data frames.
 */
#include <errno.h>
#include <inttypes.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libusb-1.0/libusb.h>

#define VID 0x0471
#define PID 0x1200
#define EP_CMD_OUT 0x01
#define EP_CMD_IN  0x81
#define EP_CAN_OUT 0x02
#define EP_CAN_IN  0x82
#define USB_TIMEOUT_MS 1000

static volatile sig_atomic_t running = 1;

static void stop_running(int sig)
{
    (void)sig;
    running = 0;
}

static uint32_t load_le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static int command(libusb_device_handle *dev, const uint8_t *out,
                   int out_len, uint8_t expected_command)
{
    uint8_t in[16] = {0};
    int actual = 0;
    int rc = libusb_interrupt_transfer(dev, EP_CMD_OUT, (uint8_t *)out,
                                       out_len, &actual, USB_TIMEOUT_MS);
    if (rc != 0 || actual != out_len) {
        fprintf(stderr, "command %02x OUT failed: %s, %d/%d bytes\n",
                out[1], libusb_error_name(rc), actual, out_len);
        return -1;
    }
    rc = libusb_interrupt_transfer(dev, EP_CMD_IN, in, sizeof(in),
                                   &actual, USB_TIMEOUT_MS);
    if (rc != 0 || actual < 4 || in[0] != 0x12 ||
        in[1] != expected_command || in[3] != 0) {
        fprintf(stderr, "command %02x ACK failed: %s, len=%d, data=",
                out[1], libusb_error_name(rc), actual);
        for (int i = 0; i < actual; ++i) fprintf(stderr, "%02x", in[i]);
        fputc('\n', stderr);
        return -1;
    }
    return 0;
}

static int start_both_500k(libusb_device_handle *dev)
{
    static const uint8_t close_can2[] = {0x12,0x0f,0x12,0x00,0x10};
    static const uint8_t close_can1[] = {0x12,0x0f,0x12,0x00,0x00};
    static const uint8_t init_can1[] = {0x12,0x03,0x04,0x00,0x00,0x00,0x1c};
    static const uint8_t filter_can1[] = {
        0x12,0x04,0x0a,0x00,0x40,0x00,0x00,0x00,0x00,0xff,0xff,0xff,0xff
    };
    static const uint8_t start_can1[] = {0x12,0x0e,0x02,0x00,0x00};
    static const uint8_t init_can2[] = {0x12,0x03,0x04,0x00,0x10,0x00,0x1c};
    static const uint8_t filter_can2[] = {
        0x12,0x04,0x0a,0x00,0x50,0x00,0x00,0x00,0x00,0xff,0xff,0xff,0xff
    };
    static const uint8_t start_can2[] = {0x12,0x0e,0x02,0x00,0x10};

    if (command(dev, close_can2, sizeof(close_can2), 0x0f) ||
        command(dev, close_can1, sizeof(close_can1), 0x0f) ||
        command(dev, close_can2, sizeof(close_can2), 0x0f))
        return -1;
    return command(dev, init_can1, sizeof(init_can1), 0x03) ||
           command(dev, filter_can1, sizeof(filter_can1), 0x04) ||
           command(dev, start_can1, sizeof(start_can1), 0x0e) ||
           command(dev, init_can2, sizeof(init_can2), 0x03) ||
           command(dev, filter_can2, sizeof(filter_can2), 0x04) ||
           command(dev, start_can2, sizeof(start_can2), 0x0e);
}

static int send_test_request(libusb_device_handle *dev)
{
    static const uint8_t record[19] = {
        0x30,0x00,0x00,0x00,0x00,0x00,0x18,0x60,0x24,0x00,0x00,
        0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88
    };
    int actual = 0;
    int rc = libusb_bulk_transfer(dev, EP_CAN_OUT, (uint8_t *)record,
                                  sizeof(record), &actual, USB_TIMEOUT_MS);
    if (rc != 0 || actual != (int)sizeof(record)) {
        fprintf(stderr, "CAN 0x123 OUT failed: %s, %d/%zu bytes\n",
                libusb_error_name(rc), actual, sizeof(record));
        return -1;
    }
    puts("sent id=123 dlc=8 data=11 22 33 44 55 66 77 88");
    return 0;
}

static void print_record(const uint8_t record[19])
{
    uint32_t timestamp = load_le32(record);
    uint32_t descriptor = load_le32(record + 5);
    uint32_t id = descriptor >> 21;
    unsigned dlc = (descriptor >> 8) & 0x0f;
    unsigned channel = (descriptor & 0x1000U) ? 2U : 1U;
    uint32_t remaining = descriptor & 0x001fffffU;
    uint32_t expected = ((uint32_t)dlc << 8) |
                        (channel == 2U ? 0x1000U : 0U);

    if (dlc > 8 || remaining != expected) {
        fprintf(stderr, "unknown record: ts=%" PRIu32 " desc=%08" PRIx32 "\n",
                timestamp, descriptor);
        return;
    }
    printf("ts=%" PRIu32 " id=%03" PRIx32 " dlc=%u data=",
           timestamp, id, dlc);
    for (unsigned i = 0; i < dlc; ++i)
        printf("%s%02x", i ? " " : "", record[11 + i]);
    printf(" can=%u\n", channel);
    fflush(stdout);
}

int main(int argc, char **argv)
{
    int run_test = argc == 2 && strcmp(argv[1], "--test") == 0;
    if (argc > 2 || (argc == 2 && !run_test)) {
        fprintf(stderr, "usage: %s [--test]\n", argv[0]);
        return 2;
    }
    libusb_context *ctx = NULL;
    libusb_device_handle *dev = NULL;
    int claimed = 0;
    int rc = libusb_init(&ctx);
    if (rc != 0) {
        fprintf(stderr, "libusb_init: %s\n", libusb_error_name(rc));
        return 1;
    }
    dev = libusb_open_device_with_vid_pid(ctx, VID, PID);
    if (!dev) {
        fprintf(stderr, "USB device %04x:%04x not found or permission denied\n", VID, PID);
        goto fail;
    }
    if (libusb_kernel_driver_active(dev, 0) == 1)
        libusb_detach_kernel_driver(dev, 0);
    rc = libusb_claim_interface(dev, 0);
    if (rc != 0) {
        fprintf(stderr, "claim interface 0: %s\n", libusb_error_name(rc));
        goto fail;
    }
    claimed = 1;
    if (start_both_500k(dev) != 0) goto fail;

    signal(SIGINT, stop_running);
    signal(SIGTERM, stop_running);
    puts("CAN1 and CAN2 started at 500 kbit/s; Ctrl-C to stop");
    if (run_test && send_test_request(dev) != 0) goto fail;
    while (running) {
        uint8_t buffer[64];
        int actual = 0;
        rc = libusb_bulk_transfer(dev, EP_CAN_IN, buffer, sizeof(buffer),
                                  &actual, 250);
        if (rc == LIBUSB_ERROR_TIMEOUT) continue;
        if (rc != 0) {
            fprintf(stderr, "bulk IN: %s\n", libusb_error_name(rc));
            break;
        }
        if (actual % 19 != 0) {
            fprintf(stderr, "bulk IN unexpected length: %d\n", actual);
            continue;
        }
        for (int offset = 0; offset < actual; offset += 19)
            print_record(buffer + offset);
    }

    if (claimed) libusb_release_interface(dev, 0);
    libusb_close(dev);
    libusb_exit(ctx);
    return 0;

fail:
    if (claimed) libusb_release_interface(dev, 0);
    if (dev) libusb_close(dev);
    libusb_exit(ctx);
    return 1;
}
