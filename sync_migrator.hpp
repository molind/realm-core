#include <string>

extern "C" {
namespace realm {

void MigrateClientRealm(const std::string& inPath, const std::string& outPath);
void MigrateServerRealm(const std::string& inPath, const std::string& outPath);
void MigrateLocalRealm(const std::string& path);

} // namespace realm
}
