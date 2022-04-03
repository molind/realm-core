#include "sync_migrator.hpp"

int main(int argc, const char *argv[]) {
    if (argc != 4) {
        printf("Usage: sync_migrator_app <synced realm path> <base64 metadata key> <output realm path>\n");
        return 0;
    }

    realm::MigrateSyncedRealm(argv[1], argv[2], argv[3]);

    return 0;
}