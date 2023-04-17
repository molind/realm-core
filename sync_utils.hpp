#include <string>

extern "C" {
namespace realm {

void ServerRealmContents(const std::string& inPath);
void ServerRealmChangeset(const std::string& inPath, int changeset);

} // namespace realm
}
