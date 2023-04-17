#include "sync_utils.hpp"
#define EXPORT __attribute__((visibility("default")))

#include <realm.hpp>
#include <realm/history.hpp>
#include <realm/object-store/object_store.hpp>
#include <realm/sync/noinst/client_history_impl.hpp>
#include <realm/sync/noinst/server/server_history.hpp>

#include <map>
#include <set>
#include <fstream>
#include <iostream>

extern "C" {
namespace realm {

// void MigrateRealm(DBRef inRealm, DBRef outRealm)
// {
//     auto readTr = inRealm->start_read();
//     auto writeTr = outRealm->start_write();

//     int version = ObjectStore::get_schema_version(*readTr);
//     ObjectStore::set_schema_version(*writeTr, version);

//     // служебные таблицы в сетевом реалме. их не надо копировать
//     auto filteredTables = std::set<std::string>{"metadata",      "class___Permission", "class___Role",
//                                                 "class___Class", "class___Realm",      "class___User"};
//     // бывало мы заливали историю поиска в сетевую базу. выковыривать ее оттуда не надо.
//     filteredTables.emplace("class_ModelSearchHistoryItem");

//     auto tableKeys = readTr->get_table_keys();
//     for (const auto tableKey : tableKeys) {
//         auto tableName = readTr->get_table_name(tableKey);
//         if (filteredTables.find(tableName) != filteredTables.end())
//             continue;

//         auto table = readTr->get_table(tableKey);
//         auto pkCol = table->get_primary_key_column();
//         auto writeTable = writeTr->add_table_with_primary_key(tableName, table->get_column_type(pkCol),
//                                                               table->get_column_name(pkCol));

//         auto columns = table->get_column_keys();
//         for (const auto column : columns) {
//             if (column == pkCol)
//                 continue;

//             auto attr = table->get_column_attr(column);
//             const auto& colName = table->get_column_name(column);
//             auto writeCol =
//                 writeTable->add_column(table->get_column_type(column), colName, attr.test(col_attr_Nullable));
//             // у исходной таблицы не всегда folderUuid индексирован
//             if (attr.test(col_attr_Indexed) || colName == "folderUuid")
//                 writeTable->add_search_index(writeCol);
//         }

//         auto writeColumns = writeTable->get_column_keys();
//         for (const auto& obj : *table) {
//             auto writeObj = writeTable->create_object_with_primary_key(obj.get_any(pkCol));
//             for (int i = 0; i < columns.size(); i++) {
//                 const auto& column = columns[i];
//                 if (column == pkCol)
//                     continue;

//                 writeObj.set(writeColumns[i], obj.get_any(column));
//             }
//         }
//     }
//     // проверяем, что все прочиталось хорошо
//     // auto stream = std::ofstream("in.json");
//     // readTr->to_json(stream);
//     // stream.close();
//     // stream = std::ofstream("out.json");
//     // writeTr->to_json(stream);
//     // stream.close();

//     writeTr->commit();
// }

// EXPORT void MigrateClientRealm(const std::string& inPath, const std::string& outPath)
// {
//     auto syncHist = realm::sync::make_client_replication(inPath);

//     realm::DBOptions options;
//     options.allow_file_format_upgrade = true;

//     auto syncDB = realm::DB::create(*syncHist, options);
//     auto localDB = realm::DB::create(outPath);

//     MigrateRealm(syncDB, localDB);
// }

// EXPORT void MigrateLocalRealm(const std::string& path)
// {
//     auto history = realm::make_in_realm_history(path);

//     realm::DBOptions options;
//     options.allow_file_format_upgrade = true;

//     auto inDB = realm::DB::create(*history, options);

//     // auto stream = std::ofstream("out.json");
//     // inDB->start_read()->to_json(stream);
//     // stream.close();
// }

namespace {

struct MakeServerHistory {
    class HistoryContext : public _impl::ServerHistory::Context {
    public:
        std::mt19937_64& server_history_get_random() noexcept override final
        {
            return m_random;
        }

    private:
        std::mt19937_64 m_random;
    };
    class WrapServerHistory : public HistoryContext, public _impl::ServerHistory {
    public:
        WrapServerHistory()
            : _impl::ServerHistory{static_cast<ServerHistory::Context&>(*this)}
        {
        }
    };

    static std::unique_ptr<_impl::ServerHistory> make_history()
    {
        return std::make_unique<WrapServerHistory>();
    }
};

} // unnamed namespace

EXPORT void ServerRealmContents(const std::string& path)
{
    DBOptions options;
    options.logger = util::Logger::get_default_logger();
    // options.allow_file_format_upgrade = true;
    auto db = DB::create(MakeServerHistory::make_history(), path, options);

    // size_t free_space, used_space;
    // db->get_stats(free_space, used_space);
    // printf("%zu %zu\n", free_space, used_space);

    const auto& repl = db->get_replication();
    // printf("%d\n", repl->get_history_type());

    if (auto sh = dynamic_cast<_impl::ServerHistory *>(repl)) {

        // sync::VersionInfo version_info;
        // bool has_upstream_sync_status;
        // sync::file_ident_type partial_file_ident;
        // sync::version_type partial_progress_reference_version;

        // sh->get_status(version_info, has_upstream_sync_status, partial_file_ident, partial_progress_reference_version);

        // auto changesets = sh->get_parsed_changesets(1, version_info.realm_version);

        // for (const auto& c : changesets) {
        //     printf("%llu\t%llu\t%lu\n", c.version, c.origin_timestamp, c.original_changeset_size);
        // }

        const auto& hc = sh->get_history_contents();
        printf("Client files:\n");
        for (const auto& cf : hc.client_files) {
            printf("%lld\t%llu\t%llu\n", cf.client_type, cf.client_version, cf.locked_server_version);
        }

        printf("Changesets:\n");
        for (int i=0; i< hc.sync_history.size(); ++i) {
            auto& changeset = hc.sync_history[i];
            std::time_t seconds;
            long nanos;
            sync::map_changeset_timestamp(changeset.timestamp, seconds, nanos);

            // std::time_t ct = std::time(reinterpret_cast<std::time_t *>(&changeset.timestamp));
            // const auto timestamp = ctime(&ct);
            const auto& stime = ctime(&seconds);

            printf("%d\t%llu\t%llu\t%lu\t%s", i+1, changeset.client_file_ident, changeset.client_version, changeset.changeset.size(), stime);
        }

        printf("%lu\n", hc.sync_history.size());

        // auto stream = std::ofstream("out.json");
        // db->start_read()->to_json(stream);
        // stream.close();

        // не работает :(
        // sh->verify();
    }
    // _impl::ChangesetInputStream in(*history, 0, 2);
    // auto span = in.next_block();

    // printf("%zu %zu\n", span.size(), span.size_bytes());
}

void ServerRealmChangeset(const std::string& path, int changeset_index) {
    DBOptions options;
    options.logger = util::Logger::get_default_logger();
    auto db = DB::create(MakeServerHistory::make_history(), path, options);
    const auto& repl = db->get_replication();

    if (auto sh = dynamic_cast<_impl::ServerHistory *>(repl)) {
        const auto& changesets = sh->get_parsed_changesets(changeset_index, changeset_index+1);
        const auto& changeset = *changesets.begin();

        printf("Changeset: %d\nLast integrated ver: %llu\nOrig size: %lu\n", changeset_index, changeset.last_integrated_remote_version, changeset.original_changeset_size);
        std::cout << changeset;
    }
}

} // namespace realm
}
