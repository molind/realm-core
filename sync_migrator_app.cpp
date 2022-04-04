#include "sync_migrator.hpp"

int main(int argc, const char *argv[]) {
    if (argc != 3) {
        printf("Usage: sync_migrator_app <synced realm path> <output realm path>\n");
        return 0;
    }
    realm::MigrateSyncedRealm(argv[1], argv[2]);
    return 0;
}