#include <string>

extern "C" {
namespace realm {

void MigrateSyncedRealm(const std::string &inPath, const std::string &key, const std::string &outPath);

}
}
