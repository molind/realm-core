#include "sync_utils.hpp"

int main(int argc, const char* argv[])
{
    auto usage = "Usage: sync_utils_app <explain> <synced realm path>";

    std::string_view mode(argv[1]);
    if (mode == "explain" && argc == 3) {
        realm::ExplainServerRealm(argv[2]);
    }
    else {
        printf("%s\n", usage);
    }
    return 0;
}
