#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

static void print_usage(void)
{
    printf("Usage: snapshot create [message]\n");
    printf("       snapshot list\n");
    printf("       snapshot restore <id>\n");
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        print_usage();
        return 1;
    }

    if (strcmp(argv[1], "create") == 0) {
        const char *msg = argc > 2 ? argv[2] : "snapshot";
        uint64_t snap_id = 0;
        if (zirvfs_snap_create(0, msg, &snap_id) < 0) {
            printf("snapshot: create failed (no ZirvFS volume?)\n");
            return 1;
        }
        printf("%llu\n", (unsigned long long)snap_id);
        return 0;

    } else if (strcmp(argv[1], "list") == 0) {
        zirvfs_snapshot_info_t snaps[128];
        uint32_t count = 128;
        if (zirvfs_snap_list(0, snaps, &count) < 0) {
            printf("snapshot: list failed\n");
            return 1;
        }
        for (uint32_t i = 0; i < count; i++) {
            printf("%llu %llu %s\n",
                   (unsigned long long)snaps[i].id,
                   (unsigned long long)snaps[i].timestamp,
                   snaps[i].message);
        }
        return 0;

    } else if (strcmp(argv[1], "restore") == 0) {
        if (argc < 3) {
            printf("Usage: snapshot restore <id>\n");
            return 1;
        }
        uint64_t id = 0;
        const char *p = argv[2];
        while (*p) { id = id * 10 + (unsigned long)(*p - '0'); p++; }
        if (zirvfs_snap_restore(0, id) < 0) {
            printf("snapshot: restore of %llu failed\n",
                   (unsigned long long)id);
            return 1;
        }
        return 0;

    } else {
        print_usage();
        return 1;
    }
}
