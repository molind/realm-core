#include "sync_utils.hpp"

int main(int argc, const char* argv[])
{
    auto usage = "Usage: sync_utils_app explain <synced realm path> [commit_number]";

    std::string_view mode(argv[1]);
    if (mode == "explain" && (argc == 3 || argc == 4)) {
        if (argc == 3) {
            realm::ServerRealmContents(argv[2]);
        } else {
            realm::ServerRealmChangeset(argv[2], std::stoi(argv[3]));
        }
    } else {
        printf("%s\n", usage);
    }
    return 0;
}
