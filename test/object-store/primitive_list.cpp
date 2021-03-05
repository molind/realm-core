////////////////////////////////////////////////////////////////////////////
//
// Copyright 2017 Realm Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
////////////////////////////////////////////////////////////////////////////

#include <catch2/catch.hpp>

#include "collection_fixtures.hpp"
#include "util/event_loop.hpp"
#include "util/index_helpers.hpp"
#include "util/test_file.hpp"

#include <realm/object-store/binding_context.hpp>
#include <realm/object-store/list.hpp>
#include <realm/object-store/object.hpp>
#include <realm/object-store/object_schema.hpp>
#include <realm/object-store/property.hpp>
#include <realm/object-store/results.hpp>
#include <realm/object-store/schema.hpp>
#include <realm/object-store/thread_safe_reference.hpp>

#include <realm/object-store/impl/realm_coordinator.hpp>
#include <realm/object-store/impl/object_accessor_impl.hpp>

#include <realm/db.hpp>
#include <realm/query_expression.hpp>
#include <realm/version.hpp>

#include <numeric>

using namespace realm;
using namespace realm::util;
namespace cf = realm::collection_fixtures;

struct StringifyingContext {
    template <typename T>
    std::string box(T value)
    {
        std::stringstream ss;
        ss << value;
        return ss.str();
    }

    std::string box(Obj obj)
    {
        return util::to_string(obj.get_key().value);
    }
};

struct less {
    template <class T, class U>
    auto operator()(T&& a, U&& b) const noexcept
    {
        return a < b;
    }
};
struct greater {
    template <class T, class U>
    auto operator()(T&& a, U&& b) const noexcept
    {
        return a > b;
    }
};

template <>
auto less::operator()<Timestamp&, Timestamp&>(Timestamp& a, Timestamp& b) const noexcept
{
    if (b.is_null())
        return false;
    if (a.is_null())
        return true;
    return a < b;
}

template <>
auto greater::operator()<Timestamp&, Timestamp&>(Timestamp& a, Timestamp& b) const noexcept
{
    if (a.is_null())
        return false;
    if (b.is_null())
        return true;
    return a > b;
}

namespace Catch {
template <>
struct StringMaker<List> {
    static std::string convert(List const& list)
    {
        std::stringstream ss;
        auto type = list.get_type();
        ss << string_for_property_type(type & ~PropertyType::Flags);
        if (is_nullable(type))
            ss << "?";
        ss << "{";

        StringifyingContext ctx;
        for (size_t i = 0, count = list.size(); i < count; ++i)
            ss << list.get(ctx, i) << ", ";
        auto str = ss.str();
        str.pop_back();
        str.back() = '}';
        return str;
    }
};
template <>
struct StringMaker<Results> {
    static std::string convert(Results const& r)
    {
        auto& results = const_cast<Results&>(r);
        std::stringstream ss;
        auto type = results.get_type();
        ss << string_for_property_type(type & ~PropertyType::Flags);
        if (is_nullable(type))
            ss << "?";
        ss << "{";

        StringifyingContext ctx;
        for (size_t i = 0, count = results.size(); i < count; ++i)
            ss << results.get(ctx, i) << ", ";
        auto str = ss.str();
        str.pop_back();
        str.back() = '}';
        return str;
    }
};
template <>
struct StringMaker<util::None> {
    static std::string convert(util::None)
    {
        return "[none]";
    }
};
} // namespace Catch

template <typename T>
T get(Mixed)
{
    abort();
}

template <>
Mixed get(Mixed m)
{
    return m;
}

template <>
int64_t get(Mixed m)
{
    return m.get_int();
}
template <>
float get(Mixed m)
{
    return m.get_type() == type_Float ? m.get_float() : static_cast<float>(m.get_double());
}
template <>
double get(Mixed m)
{
    return m.get_double();
}
template <>
Timestamp get(Mixed m)
{
    return m.get_timestamp();
}
template <>
Decimal128 get(Mixed m)
{
    return m.get<Decimal128>();
}

TEMPLATE_TEST_CASE("primitive list", "[primitives]", cf::MixedVal, cf::Int, cf::Bool, cf::Float, cf::Double,
                   cf::String, cf::Binary, cf::Date, cf::OID, cf::Decimal, cf::UUID, cf::BoxedOptional<cf::Int>,
                   cf::BoxedOptional<cf::Bool>, cf::BoxedOptional<cf::Float>, cf::BoxedOptional<cf::Double>,
                   cf::BoxedOptional<cf::OID>, cf::BoxedOptional<cf::UUID>, cf::UnboxedOptional<cf::String>,
                   cf::UnboxedOptional<cf::Binary>, cf::UnboxedOptional<cf::Date>, cf::UnboxedOptional<cf::Decimal>)
{
    auto values = TestType::values();
    using T = typename TestType::Type;
    using W = typename TestType::Wrapped;
    using Boxed = typename TestType::Boxed;

    InMemoryTestFile config;
    config.cache = false;
    config.automatic_change_notifications = false;
    config.schema = Schema{
        {"object", {{"value", PropertyType::Array | TestType::property_type()}}},
    };
    auto r = Realm::get_shared_realm(config);
    auto r2 = Realm::get_shared_realm(config);

    auto table = r->read_group().get_table("class_object");
    auto table2 = r2->read_group().get_table("class_object");
    r->begin_transaction();
    Obj obj = table->create_object();
    ColKey col = table->get_column_key("value");

    List list(r, obj, col);
    auto results = list.as_results();
    CppContext ctx(r);

    SECTION("get_realm()") {
        REQUIRE(list.get_realm() == r);
        REQUIRE(results.get_realm() == r);
    }
#if 0
    SECTION("get_query()") {
        REQUIRE(list.get_query().count() == 0);
        REQUIRE(results.get_query().count() == 0);
        list.add(static_cast<T>(values[0]));
        REQUIRE(list.get_query().count() == 1);
        REQUIRE(results.get_query().count() == 1);
    }
#endif
    SECTION("get_origin_row_index()") {
        REQUIRE(list.get_parent_object_key() == obj.get_key());
        table->create_object();
        REQUIRE(list.get_parent_object_key() == obj.get_key());
    }

    SECTION("get_type()") {
        REQUIRE(list.get_type() == TestType::property_type());
        REQUIRE(results.get_type() == TestType::property_type());
    }

    SECTION("get_object_type()") {
        REQUIRE(results.get_object_type() == StringData());
    }

    SECTION("is_valid()") {
        REQUIRE(list.is_valid());
        REQUIRE(results.is_valid());

        SECTION("invalidate") {
            r->invalidate();
            REQUIRE_FALSE(list.is_valid());
            REQUIRE_FALSE(results.is_valid());
        }

        SECTION("close") {
            r->close();
            REQUIRE_FALSE(list.is_valid());
            REQUIRE_FALSE(results.is_valid());
        }

        SECTION("delete row") {
            obj.remove();
            REQUIRE_FALSE(list.is_valid());
            REQUIRE_FALSE(results.is_valid());
        }

        SECTION("rollback transaction creating list") {
            r->cancel_transaction();
            REQUIRE_FALSE(list.is_valid());
            REQUIRE_FALSE(results.is_valid());
        }
    }

    SECTION("verify_attached()") {
        REQUIRE_NOTHROW(list.verify_attached());

        SECTION("invalidate") {
            r->invalidate();
            REQUIRE_THROWS(list.verify_attached());
        }

        SECTION("close") {
            r->close();
            REQUIRE_THROWS(list.verify_attached());
        }

        SECTION("delete row") {
            obj.remove();
            REQUIRE_THROWS(list.verify_attached());
        }

        SECTION("rollback transaction creating list") {
            r->cancel_transaction();
            REQUIRE_THROWS(list.verify_attached());
        }
    }

    SECTION("verify_in_transaction()") {
        REQUIRE_NOTHROW(list.verify_in_transaction());

        SECTION("invalidate") {
            r->invalidate();
            REQUIRE_THROWS(list.verify_in_transaction());
        }

        SECTION("close") {
            r->close();
            REQUIRE_THROWS(list.verify_in_transaction());
        }

        SECTION("delete row") {
            obj.remove();
            REQUIRE_THROWS(list.verify_in_transaction());
        }

        SECTION("end write") {
            r->commit_transaction();
            REQUIRE_THROWS(list.verify_in_transaction());
        }
    }

    if (!list.is_valid() || !r->is_in_transaction())
        return;

    for (T value : values)
        list.add(value);

    SECTION("move()") {
        if (list.size() < 3)
            return;

        list.move(1, 2);
        std::swap(values[1], values[2]);
        REQUIRE(list == values);
        REQUIRE(results == values);

        list.move(2, 1);
        std::swap(values[1], values[2]);
        REQUIRE(list == values);
        REQUIRE(results == values);

        list.move(0, 2);
        std::rotate(values.begin(), values.begin() + 1, values.begin() + 3);
        REQUIRE(list == values);
        REQUIRE(results == values);

        list.move(2, 0);
        std::rotate(values.begin(), values.begin() + 2, values.begin() + 3);
        REQUIRE(list == values);
        REQUIRE(results == values);
    }

    SECTION("remove()") {
        size_t pos = 1;
        while (list.size()) {
            size_t ndx = pos % list.size();
            list.remove(ndx);
            values.erase(values.begin() + ndx);
            REQUIRE(list == values);
            REQUIRE(results == values);
            ++pos;
        }
    }

    SECTION("remove_all()") {
        list.remove_all();
        REQUIRE(list.size() == 0);
        REQUIRE(results.size() == 0);
    }

    SECTION("swap()") {
        if (list.size() < 3)
            return;

        list.swap(0, 2);
        std::swap(values[0], values[2]);
        REQUIRE(list == values);
        REQUIRE(results == values);

        if (list.size() < 4)
            return;

        list.swap(1, 3);
        std::swap(values[1], values[3]);
        REQUIRE(list == values);
        REQUIRE(results == values);
    }

    SECTION("delete_all()") {
        list.delete_all();
        REQUIRE(list.size() == 0);
        REQUIRE(results.size() == 0);
    }

    SECTION("clear()") {
        results.clear();
        REQUIRE(list.size() == 0);
        REQUIRE(results.size() == 0);
    }

    SECTION("get()") {
        for (size_t i = 0; i < values.size(); ++i) {
            CAPTURE(i);
            REQUIRE(list.get<T>(i) == values[i]);
            REQUIRE(results.get<T>(i) == values[i]);
            REQUIRE(any_cast<Boxed>(list.get(ctx, i)) == Boxed(values[i]));
            REQUIRE(any_cast<Boxed>(results.get(ctx, i)) == Boxed(values[i]));
        }
        REQUIRE_THROWS(list.get<T>(values.size()));
        REQUIRE_THROWS(results.get<T>(values.size()));
        REQUIRE_THROWS(list.get(ctx, values.size()));
        REQUIRE_THROWS(results.get(ctx, values.size()));
    }

    SECTION("first()") {
        REQUIRE(*results.first<T>() == values.front());
        REQUIRE(any_cast<Boxed>(*results.first(ctx)) == Boxed(values.front()));
        list.remove_all();
        REQUIRE(results.first<T>() == util::none);
    }

    SECTION("last()") {
        REQUIRE(*results.last<T>() == values.back());
        list.remove_all();
        REQUIRE(results.last<T>() == util::none);
    }

    SECTION("set()") {
        for (size_t i = 0; i < values.size(); ++i) {
            CAPTURE(i);
            auto rev = values.size() - i - 1;
            list.set(i, static_cast<T>(values[rev]));
            REQUIRE(list.get<T>(i) == values[rev]);
            REQUIRE(results.get<T>(i) == values[rev]);
        }
        for (size_t i = 0; i < values.size(); ++i) {
            CAPTURE(i);
            list.set(ctx, i, TestType::to_any(values[i]));
            REQUIRE(list.get<T>(i) == values[i]);
            REQUIRE(results.get<T>(i) == values[i]);
        }
        for (size_t i = 0; i < values.size(); ++i) {
            CAPTURE(i);
            auto rev = values.size() - i - 1;
            Mixed val(values[rev]);
            list.set_any(i, val);
            REQUIRE(list.get_any(i) == val);
            REQUIRE(results.get_any(i) == val);
        }

        REQUIRE_THROWS(list.set(list.size(), static_cast<T>(values[0])));
    }

    SECTION("find()") {
        for (size_t i = 0; i < values.size(); ++i) {
            CAPTURE(i);
            REQUIRE(list.find<T>(values[i]) == i);
            REQUIRE(list.find_any(Mixed(values[i])) == i);
            REQUIRE(results.index_of<T>(values[i]) == i);

            REQUIRE(list.find(ctx, TestType::to_any(values[i])) == i);
            REQUIRE(results.index_of(ctx, TestType::to_any(values[i])) == i);
#if 0
            auto q = TestType::unwrap(values[i], [&] (auto v) { return table->get_subtable(0, 0)->column<W>(0) == v; });
            REQUIRE(list.find(Query(q)) == i);
            REQUIRE(results.index_of(std::move(q)) == i);
#endif
        }

        list.remove(0);
        REQUIRE(list.find(static_cast<T>(values[0])) == npos);
        REQUIRE(results.index_of(static_cast<T>(values[0])) == npos);

        REQUIRE(list.find(ctx, TestType::to_any(values[0])) == npos);
        REQUIRE(results.index_of(ctx, TestType::to_any(values[0])) == npos);
    }
    SECTION("sorted index_of()") {
        auto sorted = list.sort({{"self", true}});
        std::sort(begin(values), end(values), less());
        for (size_t i = 0; i < values.size(); ++i) {
            CAPTURE(i);
            REQUIRE(sorted.index_of<T>(values[i]) == i);
        }

        sorted = list.sort({{"self", false}});
        std::sort(begin(values), end(values), greater());
        for (size_t i = 0; i < values.size(); ++i) {
            CAPTURE(i);
            REQUIRE(sorted.index_of<T>(values[i]) == i);
        }
    }

#if 0
    SECTION("filtered index_of()") {
        REQUIRE_THROWS(results.index_of(table->get(0)));
        auto q = TestType::unwrap(values[0], [&] (auto v) { return table->get_subtable(0, 0)->column<W>(0) != v; });
        auto filtered = list.filter(std::move(q));
        for (size_t i = 1; i < values.size(); ++i) {
            CAPTURE(i);
            REQUIRE(filtered.index_of(static_cast<T>(values[i])) == i - 1);
        }
    }
#endif
    SECTION("sort()") {
        auto unsorted = list.sort(std::vector<std::pair<std::string, bool>>{});
        REQUIRE(unsorted == values);

        auto sorted = list.sort(SortDescriptor({{col}}, {true}));
        auto sorted2 = list.sort({{"self", true}});
        std::sort(begin(values), end(values), less());
        REQUIRE(sorted == values);
        REQUIRE(sorted2 == values);

        sorted = list.sort(SortDescriptor({{col}}, {false}));
        sorted2 = list.sort({{"self", false}});
        std::sort(begin(values), end(values), greater());
        REQUIRE(sorted == values);
        REQUIRE(sorted2 == values);

        auto execption_string =
            util::format("Cannot sort on key path 'not self': arrays of '%1' can only be sorted on 'self'",
                         string_for_property_type(TestType::property_type() & ~PropertyType::Flags));
        REQUIRE_THROWS_WITH(list.sort({{"not self", true}}), execption_string);
        REQUIRE_THROWS_WITH(list.sort({{"self", true}, {"self", false}}),
                            util::format("Cannot sort array of '%1' on more than one key path",
                                         string_for_property_type(TestType::property_type() & ~PropertyType::Flags)));
    }

    SECTION("distinct()") {
        for (T value : values)
            list.add(value);
        auto values2 = values;
        values2.insert(values2.end(), values.begin(), values.end());

        auto undistinct = list.as_results().distinct(std::vector<std::string>{});
        REQUIRE(undistinct == values2);

        auto distinct = results.distinct(DistinctDescriptor({{col}}));
        auto distinct2 = results.distinct({"self"});
        REQUIRE(distinct == values);
        REQUIRE(distinct2 == values);

        REQUIRE_THROWS_WITH(
            results.distinct({{"not self"}}),
            util::format("Cannot sort on key path 'not self': arrays of '%1' can only be sorted on 'self'",
                         string_for_property_type(TestType::property_type() & ~PropertyType::Flags)));
        REQUIRE_THROWS_WITH(results.distinct({{"self"}, {"self"}}),
                            util::format("Cannot sort array of '%1' on more than one key path",
                                         string_for_property_type(TestType::property_type() & ~PropertyType::Flags)));
    }
#if 0
    SECTION("filter()") {
        T v = values.front();
        values.erase(values.begin());

        auto q = TestType::unwrap(v, [&] (auto v) { return table->column<Lst<W>>(col) != v; });
        Results filtered = list.filter(std::move(q));
        REQUIRE(filtered == values);

        q = TestType::unwrap(v, [&] (auto v) { return table->column<Lst<W>>(col) == v; });
        filtered = list.filter(std::move(q));
        REQUIRE(filtered.size() == 1);
        REQUIRE(*filtered.first<T>() == v);
    }
#endif

    SECTION("min()") {
        if (!TestType::can_minmax()) {
            REQUIRE_THROWS(list.min());
            REQUIRE_THROWS(results.min());
            return;
        }

        REQUIRE(get<W>(*list.min()) == TestType::min());
        REQUIRE(get<W>(*results.min()) == TestType::min());
        list.remove_all();
        REQUIRE(list.min() == util::none);
        REQUIRE(results.min() == util::none);
    }

    SECTION("max()") {
        if (!TestType::can_minmax()) {
            REQUIRE_THROWS(list.max());
            REQUIRE_THROWS(results.max());
            return;
        }

        REQUIRE(get<W>(*list.max()) == TestType::max());
        REQUIRE(get<W>(*results.max()) == TestType::max());
        list.remove_all();
        REQUIRE(list.max() == util::none);
        REQUIRE(results.max() == util::none);
    }

    SECTION("sum()") {
        if (!TestType::can_sum()) {
            REQUIRE_THROWS(list.sum());
            return;
        }

        REQUIRE(get<W>(list.sum()) == TestType::sum());
        REQUIRE(get<W>(*results.sum()) == TestType::sum());
        list.remove_all();
        REQUIRE(get<W>(list.sum()) == TestType::empty_sum_value());
        REQUIRE(get<W>(*results.sum()) == TestType::empty_sum_value());
    }

    SECTION("average()") {
        if (!TestType::can_average()) {
            REQUIRE_THROWS(list.average());
            return;
        }

        REQUIRE(get<typename TestType::AvgType>(*list.average()) == TestType::average());
        REQUIRE(get<typename TestType::AvgType>(*results.average()) == TestType::average());
        list.remove_all();
        REQUIRE(list.average() == util::none);
        REQUIRE(results.average() == util::none);
    }

    SECTION("operator==()") {
        Obj obj1 = table->create_object();
        REQUIRE(list == List(r, obj, col));
        REQUIRE_FALSE(list == List(r, obj1, col));
    }

    SECTION("hash") {
        Obj obj1 = table->create_object();
        std::hash<List> h;
        REQUIRE(h(list) == h(List(r, obj, col)));
        REQUIRE_FALSE(h(list) == h(List(r, obj1, col)));
    }

    SECTION("handover") {
        r->commit_transaction();

        auto list2 = ThreadSafeReference(list).resolve<List>(r);
        REQUIRE(list == list2);
        auto results2 = ThreadSafeReference(results).resolve<Results>(r);
        REQUIRE(results2 == values);
    }

    SECTION("notifications") {
        r->commit_transaction();

        auto sorted = results.sort({{"self", true}});

        size_t calls = 0;
        CollectionChangeSet change, rchange, srchange;
        auto token = list.add_notification_callback([&](CollectionChangeSet c, std::exception_ptr) {
            change = c;
            ++calls;
        });
        auto rtoken = results.add_notification_callback([&](CollectionChangeSet c, std::exception_ptr) {
            rchange = c;
            ++calls;
        });
        auto srtoken = sorted.add_notification_callback([&](CollectionChangeSet c, std::exception_ptr) {
            srchange = c;
            ++calls;
        });

        SECTION("add value to list") {
            // Remove the existing copy of this value so that the sorted list
            // doesn't have dupes resulting in an unstable order
            advance_and_notify(*r);
            r->begin_transaction();
            list.remove(0);
            r->commit_transaction();

            advance_and_notify(*r);
            r->begin_transaction();
            list.insert(0, static_cast<T>(values[0]));
            r->commit_transaction();

            advance_and_notify(*r);
            REQUIRE_INDICES(change.insertions, 0);
            REQUIRE_INDICES(rchange.insertions, 0);
            // values[0] is max(), so it ends up at the end of the sorted list
            REQUIRE_INDICES(srchange.insertions, values.size() - 1);
        }

        SECTION("remove value from list") {
            advance_and_notify(*r);
            r->begin_transaction();
            list.remove(1);
            r->commit_transaction();

            advance_and_notify(*r);
            REQUIRE_INDICES(change.deletions, 1);
            REQUIRE_INDICES(rchange.deletions, 1);
            // values[1] is min(), so it's index 0 for non-optional and 1 for
            // optional (as nulls sort to the front)
            REQUIRE_INDICES(srchange.deletions, TestType::is_optional);
        }

        SECTION("clear list") {
            advance_and_notify(*r);

            r->begin_transaction();
            list.remove_all();
            r->commit_transaction();
            advance_and_notify(*r);
            REQUIRE(change.deletions.count() == values.size());
            REQUIRE(rchange.deletions.count() == values.size());
            REQUIRE(srchange.deletions.count() == values.size());
        }

        SECTION("delete containing row") {
            advance_and_notify(*r);
            REQUIRE(calls == 3);

            r->begin_transaction();
            obj.remove();
            r->commit_transaction();
            advance_and_notify(*r);
            REQUIRE(calls == 6);
            REQUIRE(change.deletions.count() == values.size());
            REQUIRE(rchange.deletions.count() == values.size());
            REQUIRE(srchange.deletions.count() == values.size());

            r->begin_transaction();
            table->create_object();
            r->commit_transaction();
            advance_and_notify(*r);
            REQUIRE(calls == 6);
        }

        SECTION("deleting containing row before first run of notifier") {
            r2->begin_transaction();
            table2->begin()->remove();
            r2->commit_transaction();
            advance_and_notify(*r);
            REQUIRE(change.deletions.count() == values.size());
        }
    }

#if REALM_ENABLE_SYNC && REALM_HAVE_SYNC_STABLE_IDS
    SECTION("sync compatibility") {
        if (!util::EventLoop::has_implementation())
            return;

        SyncServer server;
        SyncTestFile sync_config(server, "shared");
        sync_config.schema = config.schema;
        sync_config.schema_version = 0;

        {
            auto r = Realm::get_shared_realm(sync_config);
            r->begin_transaction();

            CppContext ctx(r);
            auto obj = Object::create(ctx, r, *r->schema().find("object"), util::Any(AnyDict{}));
            auto list = any_cast<List>(obj.get_property_value<util::Any>(ctx, "value"));
            list.add(static_cast<T>(values[0]));

            r->commit_transaction();
            wait_for_upload(*r);
        }

        util::File::remove(sync_config.path);

        {
            auto r = Realm::get_shared_realm(sync_config);
            auto table = r->read_group().get_table("class_object");

            util::EventLoop::main().run_until([&] {
                return table->size() == 1;
            });

            CppContext ctx(r);
            Object obj(r, "object", 0);
            auto list = any_cast<List>(obj.get_property_value<util::Any>(ctx, "value"));
            REQUIRE(list.get<T>(0) == values[0]);
        }
    }
#endif
}
