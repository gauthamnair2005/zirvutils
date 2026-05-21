/* lspci — list PCI devices
 * Enumerates all detected PCI devices via SYS_PCI_READ (syscall 103).
 * Part of Zirvium MOSIX utilities. */
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

static const char *class_name(uint8_t class, uint8_t subclass, uint8_t prog_if)
{
    (void)prog_if;
    switch (class) {
    case 0x00: return "Unclassified";
    case 0x01: return "Mass storage";
    case 0x02:
        switch (subclass) {
        case 0x00: return "Ethernet";
        case 0x80: return "WLAN";
        default:   return "Network";
        }
    case 0x03: return "Display";
    case 0x04: return "Multimedia";
    case 0x06: return "Bridge";
    case 0x07: return "Communication";
    case 0x08: return "Peripheral";
    case 0x09: return "Input device";
    case 0x0C:
        switch (subclass) {
        case 0x03: return prog_if == 0x30 ? "xHCI USB 3.0" : "USB";
        case 0x07: return "Thunderbolt";
        default:   return "Serial bus";
        }
    case 0x0D: return "Wireless";
    default:   return "Other";
    }
}

int main(void)
{
    printf("PCI devices detected:\n");
    printf("BUS:DEV.FN  VENDOR:DEVICE  CLASS       IRQ  DRIVER\n");

    int count = 0;
    for (uint32_t i = 0; ; i++) {
        pci_dev_info_t info;
        if (pci_read(i, &info) < 0) break;

        printf("%02x:%02x.%x  %04x:%04x  %-10s %3d",
               info.bus, info.dev, info.func,
               info.vendor_id, info.device_id,
               class_name(info.class_code, info.subclass, info.prog_if),
               info.irq_line);

        if (info.driver_name[0])
            printf("  %s", info.driver_name);
        if (info.revision)
            printf("  (rev %02x)", info.revision);

        printf("\n");
        count++;
    }

    printf("Total: %d device(s)\n", count);
    return 0;
}
