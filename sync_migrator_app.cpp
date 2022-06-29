#include "sync_migrator.hpp"

int main(int argc, const char* argv[])
{
    auto usage = "Usage: sync_migrator_app <client/local> <synced realm path> <output realm path>";

    std::string_view mode(argv[1]);
    if (mode == "client" && argc == 4) {
        realm::MigrateClientRealm(argv[2], argv[3]);
    }
    else if (mode == "local" && argc == 3) {
        realm::MigrateLocalRealm(argv[2]);
    }
    else {
        printf("%s\n", usage);
    }
    return 0;
}