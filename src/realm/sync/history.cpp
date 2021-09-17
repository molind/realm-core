#include <realm/history.hpp>
#include <realm/sync/noinst/client_history_impl.hpp>

namespace realm {
namespace sync {

std::unique_ptr<Replication> make_client_replication(const std::string& realm_path)
{
    return std::make_unique<_impl::ClientReplicationImpl>(realm_path); // Throws
}

} // namespace sync
} // namespace realm
