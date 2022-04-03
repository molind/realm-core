#include "sync_migrator.hpp"
#define EXPORT  __attribute__((visibility("default")))

#include <realm.hpp>
#include <realm/history.hpp>
#include <realm/sync/noinst/client_history_impl.hpp>

#include <map>

extern "C" {
namespace realm {

EXPORT void MigrateSyncedRealm(const std::string &inPath, const std::string &key, const std::string &outPath) {
    std::map<std::string, std::string> renames;
    
    auto hist = realm::sync::make_client_replication(inPath);
    //auto hist = realm::make_in_realm_history();
    realm::DBOptions options;
    options.allow_file_format_upgrade = true;

    auto db = realm::DB::create(*hist, options);

    std::cerr << "File upgraded to latest version: " << inPath << std::endl;
    
    auto link_depth = 0;
    auto tr = db->start_read();
    tr->to_json(std::cout, link_depth, &renames);
}

}
}
