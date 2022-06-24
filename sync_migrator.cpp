#include "sync_migrator.hpp"
#define EXPORT __attribute__((visibility("default")))

#include <realm.hpp>
#include <realm/history.hpp>
#include <realm/object-store/object_store.hpp>
#include <realm/sync/noinst/client_history_impl.hpp>

#include <map>
#include <set>
#include <fstream>

extern "C" {
namespace realm {

void MigrateRealm(DBRef inRealm, DBRef outRealm)
{
    auto readTr = inRealm->start_read();
    auto writeTr = outRealm->start_write();

    int version = ObjectStore::get_schema_version(*readTr);
    ObjectStore::set_schema_version(*writeTr, version);

    // служебные таблицы в сетевом реалме. их не надо копировать
    auto filteredTables = std::set<std::string>{"metadata",      "class___Permission", "class___Role",
                                                "class___Class", "class___Realm",      "class___User"};
    // бывало мы заливали историю поиска в сетевую базу. выковыривать ее оттуда не надо.
    filteredTables.emplace("class_ModelSearchHistoryItem");

    auto tableKeys = readTr->get_table_keys();
    for (const auto tableKey : tableKeys) {
        auto tableName = readTr->get_table_name(tableKey);
        if (filteredTables.find(tableName) != filteredTables.end())
            continue;

        auto table = readTr->get_table(tableKey);
        auto pkCol = table->get_primary_key_column();
        auto writeTable = writeTr->add_table_with_primary_key(tableName, table->get_column_type(pkCol),
                                                              table->get_column_name(pkCol));

        auto columns = table->get_column_keys();
        for (const auto column : columns) {
            if (column == pkCol)
                continue;

            auto attr = table->get_column_attr(column);
            const auto& colName = table->get_column_name(column);
            auto writeCol =
                writeTable->add_column(table->get_column_type(column), colName, attr.test(col_attr_Nullable));
            // у исходной таблицы не всегда folderUuid индексирован
            if (attr.test(col_attr_Indexed) || colName == "folderUuid")
                writeTable->add_search_index(writeCol);
        }

        auto writeColumns = writeTable->get_column_keys();
        for (const auto& obj : *table) {
            auto writeObj = writeTable->create_object_with_primary_key(obj.get_any(pkCol));
            for (int i = 0; i < columns.size(); i++) {
                const auto& column = columns[i];
                if (column == pkCol)
                    continue;

                writeObj.set(writeColumns[i], obj.get_any(column));
            }
        }
    }
    // проверяем, что все прочиталось хорошо
    // auto stream = std::ofstream("in.json");
    // readTr->to_json(stream);
    // stream.close();
    // stream = std::ofstream("out.json");
    // writeTr->to_json(stream);
    // stream.close();

    writeTr->commit();
}

EXPORT void MigrateClientRealm(const std::string& inPath, const std::string& outPath)
{
    auto syncHist = realm::sync::make_client_replication(inPath);

    realm::DBOptions options;
    options.allow_file_format_upgrade = true;

    auto syncDB = realm::DB::create(*syncHist, options);
    auto localDB = realm::DB::create(outPath);

    MigrateRealm(syncDB, localDB);
}

EXPORT void MigrateServerRealm(const std::string& inPath, const std::string& outPath)
{
    auto syncHist = realm::sync::make_client_replication(inPath);

    realm::DBOptions options;
    options.allow_file_format_upgrade = true;

    auto syncDB = realm::DB::create(*syncHist, options);
    auto localDB = realm::DB::create(outPath);

    MigrateRealm(syncDB, localDB);
}

EXPORT void MigrateLocalRealm(const std::string& path)
{
    auto history = realm::make_in_realm_history(path);

    realm::DBOptions options;
    options.allow_file_format_upgrade = true;

    auto inDB = realm::DB::create(*history, options);

    // auto stream = std::ofstream("out.json");
    // inDB->start_read()->to_json(stream);
    // stream.close();
}

} // namespace realm
}
