/*************************************************************************
 *
 * Copyright 2023 Realm Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 **************************************************************************/

#include "testsettings.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <fstream>
#include <ostream>
#include <set>
#include <chrono>

using namespace std::chrono;

#include <realm.hpp>
#include <external/json/json.hpp>
#include "test.hpp"
#include "test_types_helper.hpp"

// #include <valgrind/callgrind.h>
// #define PERFORMACE_TESTING

using namespace realm;
using namespace realm::util;
using namespace realm::test_util;
using unit_test::TestContext;

TEST(List_basic)
{
    Table table;
    auto list_col = table.add_column_list(type_Int, "int_list");
    int sum = 0;

    {
        Obj obj = table.create_object(ObjKey(5));
        CHECK_NOT(obj.is_null(list_col));
        auto list = obj.get_list<int64_t>(list_col);
        CHECK_NOT(obj.is_null(list_col));
        CHECK(list.is_empty());

        size_t return_cnt = 0, return_ndx = 0;
        list.sum(&return_cnt);
        CHECK_EQUAL(return_cnt, 0);
        list.max(&return_ndx);
        CHECK_EQUAL(return_ndx, not_found);
        return_ndx = 0;
        list.min(&return_ndx);
        CHECK_EQUAL(return_ndx, not_found);
        list.avg(&return_cnt);
        CHECK_EQUAL(return_cnt, 0);

        for (int i = 0; i < 100; i++) {
            list.add(i + 1000);
            sum += (i + 1000);
        }
    }
    {
        Obj obj = table.get_object(ObjKey(5));
        auto list1 = obj.get_list<int64_t>(list_col);
        CHECK_EQUAL(list1.size(), 100);
        CHECK_EQUAL(list1.get(0), 1000);
        CHECK_EQUAL(list1.get(99), 1099);
        auto list_base = obj.get_listbase_ptr(list_col);
        CHECK_EQUAL(list_base->size(), 100);
        CHECK(dynamic_cast<Lst<Int>*>(list_base.get()));

        CHECK_EQUAL(list1.sum(), sum);
        CHECK_EQUAL(list1.max(), 1099);
        CHECK_EQUAL(list1.min(), 1000);
        CHECK_EQUAL(list1.avg(), double(sum) / 100);

        auto list2 = obj.get_list<int64_t>(list_col);
        list2.set(50, 747);
        CHECK_EQUAL(list1.get(50), 747);
        list1.resize(101);
        CHECK_EQUAL(list1.get(100), 0);
        list1.resize(50);
        CHECK_EQUAL(list1.size(), 50);
    }
    {
        Obj obj = table.create_object(ObjKey(7));
        auto list = obj.get_list<int64_t>(list_col);
        list.resize(10);
        CHECK_EQUAL(list.size(), 10);
        for (int i = 0; i < 10; i++) {
            CHECK_EQUAL(list.get(i), 0);
        }
    }
    table.remove_object(ObjKey(5));
}

TEST(List_SimpleTypes)
{
    Group g;
    std::vector<CollectionBase*> lists;
    TableRef t = g.add_table("table");
    ColKey int_col = t->add_column_list(type_Int, "integers");
    ColKey bool_col = t->add_column_list(type_Bool, "booleans");
    ColKey string_col = t->add_column_list(type_String, "strings");
    ColKey double_col = t->add_column_list(type_Double, "doubles");
    ColKey timestamp_col = t->add_column_list(type_Timestamp, "timestamps");
    Obj obj = t->create_object(ObjKey(7));

    std::vector<int64_t> integer_vector = {1, 2, 3, 4};
    obj.set_list_values(int_col, integer_vector);

    std::vector<bool> bool_vector = {false, false, true, false, true};
    obj.set_list_values(bool_col, bool_vector);

    std::vector<StringData> string_vector = {"monday", "tuesday", "thursday", "friday", "saturday", "sunday"};
    obj.set_list_values(string_col, string_vector);

    std::vector<double> double_vector = {898742.09382, 3.14159265358979, 2.71828182845904};
    obj.set_list_values(double_col, double_vector);

    time_t seconds_since_epoc = time(nullptr);
    std::vector<Timestamp> timestamp_vector = {Timestamp(seconds_since_epoc, 0),
                                               Timestamp(seconds_since_epoc + 60, 0)};
    obj.set_list_values(timestamp_col, timestamp_vector);

    auto int_list = obj.get_list<int64_t>(int_col);
    lists.push_back(&int_list);
    std::vector<int64_t> vec(int_list.size());
    CHECK_EQUAL(integer_vector.size(), int_list.size());
    // {1, 2, 3, 4}
    auto it = int_list.begin();
    CHECK_EQUAL(*it, 1);
    std::copy(int_list.begin(), int_list.end(), vec.begin());
    unsigned j = 0;
    for (auto i : int_list) {
        CHECK_EQUAL(vec[j], i);
        CHECK_EQUAL(integer_vector[j++], i);
    }
    auto f = std::find(int_list.begin(), int_list.end(), 3);
    CHECK_EQUAL(3, *f++);
    CHECK_EQUAL(4, *f);

    for (unsigned i = 0; i < int_list.size(); i++) {
        CHECK_EQUAL(integer_vector[i], int_list[i]);
    }

    CHECK_EQUAL(3, int_list.remove(2));
    // {1, 2, 4}
    CHECK_EQUAL(integer_vector.size() - 1, int_list.size());
    CHECK_EQUAL(4, int_list[2]);
    int_list.resize(6);
    // {1, 2, 4, 0, 0, 0}
    CHECK_EQUAL(int_list[5], 0);
    int_list.swap(0, 1);
    // {2, 1, 4, 0, 0, 0}
    CHECK_EQUAL(2, int_list[0]);
    CHECK_EQUAL(1, int_list[1]);
    int_list.move(1, 4);
    // {2, 4, 0, 0, 1, 0}
    CHECK_EQUAL(4, int_list[1]);
    CHECK_EQUAL(1, int_list[4]);
    int_list.remove(1, 3);
    // {2, 0, 1, 0}
    CHECK_EQUAL(1, int_list[2]);
    int_list.resize(2);
    // {2, 0}
    CHECK_EQUAL(2, int_list.size());
    CHECK_EQUAL(2, int_list[0]);
    CHECK_EQUAL(0, int_list[1]);
    CHECK_EQUAL(lists[0]->size(), 2);
    CHECK_EQUAL(lists[0]->get_col_key(), int_col);

    int_list.clear();
    auto int_list2 = obj.get_list<int64_t>(int_col);
    CHECK_EQUAL(0, int_list2.size());

    CHECK_THROW_ANY(obj.get_list<util::Optional<int64_t>>(int_col));

    auto bool_list = obj.get_list<bool>(bool_col);
    lists.push_back(&bool_list);
    CHECK_EQUAL(bool_vector.size(), bool_list.size());
    for (unsigned i = 0; i < bool_list.size(); i++) {
        CHECK_EQUAL(bool_vector[i], bool_list[i]);
    }

    auto bool_list_nullable = obj.get_list<util::Optional<bool>>(bool_col);
    CHECK_THROW_ANY(bool_list_nullable.set(0, util::none));

    auto string_list = obj.get_list<StringData>(string_col);
    auto str_min = string_list.min();
    CHECK(!str_min);
    CHECK_EQUAL(string_list.begin()->size(), string_vector.begin()->size());
    CHECK_EQUAL(string_vector.size(), string_list.size());
    for (unsigned i = 0; i < string_list.size(); i++) {
        CHECK_EQUAL(string_vector[i], string_list[i]);
    }

    string_list.insert(2, "Wednesday");
    CHECK_EQUAL(string_vector.size() + 1, string_list.size());
    CHECK_EQUAL(StringData("Wednesday"), string_list.get(2));
    CHECK_THROW_ANY(string_list.set(2, StringData{}));
    CHECK_THROW_ANY(string_list.add(StringData{}));
    CHECK_THROW_ANY(string_list.insert(2, StringData{}));

    auto double_list = obj.get_list<double>(double_col);
    CHECK_EQUAL(double_vector.size(), double_list.size());
    for (unsigned i = 0; i < double_list.size(); i++) {
        CHECK_EQUAL(double_vector[i], double_list.get(i));
    }

    auto timestamp_list = obj.get_list<Timestamp>(timestamp_col);
    CHECK_EQUAL(timestamp_vector.size(), timestamp_list.size());
    for (unsigned i = 0; i < timestamp_list.size(); i++) {
        CHECK_EQUAL(timestamp_vector[i], timestamp_list.get(i));
    }
    size_t return_ndx = 7;
    timestamp_list.min(&return_ndx);
    CHECK_EQUAL(return_ndx, 0);
    timestamp_list.max(&return_ndx);
    CHECK_EQUAL(return_ndx, 1);

    auto timestamp_list2 = timestamp_list.clone();
    CHECK_EQUAL(timestamp_list2->size(), timestamp_list.size());

    t->remove_object(ObjKey(7));
    auto timestamp_list3 = timestamp_list.clone();
    CHECK_NOT(timestamp_list.is_attached());
    CHECK_EQUAL(timestamp_list3->size(), 0);
}

template <typename T>
struct NullableTypeConverter {
    using NullableType = util::Optional<T>;
    static bool is_null(NullableType t)
    {
        return !bool(t);
    }
};

template <>
struct NullableTypeConverter<Decimal128> {
    using NullableType = Decimal128;
    static bool is_null(Decimal128 val)
    {
        return val.is_null();
    }
};

TEST_TYPES(List_nullable, int64_t, float, double, Decimal128)
{
    Table table;
    auto list_col = table.add_column_list(ColumnTypeTraits<TEST_TYPE>::id, "int_list", true);
    ColumnSumType<TEST_TYPE> sum = TEST_TYPE(0);

    {
        Obj obj = table.create_object(ObjKey(5));
        CHECK_NOT(obj.is_null(list_col));
        auto list = obj.get_list<typename NullableTypeConverter<TEST_TYPE>::NullableType>(list_col);
        CHECK_NOT(obj.is_null(list_col));
        CHECK(list.is_empty());
        for (int i = 0; i < 100; i++) {
            TEST_TYPE val = TEST_TYPE(i + 1000);
            list.add(val);
            sum += (val);
        }
    }
    {
        Obj obj = table.get_object(ObjKey(5));
        auto list1 = obj.get_list<typename NullableTypeConverter<TEST_TYPE>::NullableType>(list_col);
        CHECK_EQUAL(list1.size(), 100);
        CHECK_EQUAL(list1.get(0), TEST_TYPE(1000));
        CHECK_EQUAL(list1.get(99), TEST_TYPE(1099));
        CHECK_NOT(list1.is_null(0));
        auto list_base = obj.get_listbase_ptr(list_col);
        CHECK_EQUAL(list_base->size(), 100);
        CHECK_NOT(list_base->is_null(0));
        CHECK(dynamic_cast<Lst<typename NullableTypeConverter<TEST_TYPE>::NullableType>*>(list_base.get()));

        CHECK_EQUAL(list1.sum(), sum);
        CHECK_EQUAL(list1.max(), TEST_TYPE(1099));
        CHECK_EQUAL(list1.min(), TEST_TYPE(1000));
        CHECK_EQUAL(list1.avg(), typename ColumnTypeTraits<TEST_TYPE>::average_type(sum) / 100);

        auto list2 = obj.get_list<typename NullableTypeConverter<TEST_TYPE>::NullableType>(list_col);
        list2.set(50, TEST_TYPE(747));
        CHECK_EQUAL(list1.get(50), TEST_TYPE(747));
        list1.set_null(50);
        CHECK(NullableTypeConverter<TEST_TYPE>::is_null(list1.get(50)));
        list1.resize(101);
        CHECK(NullableTypeConverter<TEST_TYPE>::is_null(list1.get(100)));
    }
    {
        Obj obj = table.create_object(ObjKey(7));
        auto list = obj.get_list<typename NullableTypeConverter<TEST_TYPE>::NullableType>(list_col);
        list.resize(10);
        CHECK_EQUAL(list.size(), 10);
        for (int i = 0; i < 10; i++) {
            CHECK(NullableTypeConverter<TEST_TYPE>::is_null(list.get(i)));
        }
    }
    table.remove_object(ObjKey(5));
}


TEST_TYPES(List_Ops, Prop<Int>, Prop<Float>, Prop<Double>, Prop<Decimal>, Prop<ObjectId>, Prop<UUID>, Prop<Timestamp>,
           Prop<String>, Prop<Binary>, Prop<Bool>, Nullable<Int>, Nullable<Float>, Nullable<Double>,
           Nullable<Decimal>, Nullable<ObjectId>, Nullable<UUID>, Nullable<Timestamp>, Nullable<String>,
           Nullable<Binary>, Nullable<Bool>)
{
    using underlying_type = typename TEST_TYPE::underlying_type;
    using type = typename TEST_TYPE::type;
    TestValueGenerator gen;
    Table table;
    ColKey col = table.add_column_list(TEST_TYPE::data_type, "values", TEST_TYPE::is_nullable);

    Obj obj = table.create_object();
    Lst<type> list = obj.get_list<type>(col);
    list.add(gen.convert_for_test<underlying_type>(1));
    list.add(gen.convert_for_test<underlying_type>(2));
    list.swap(0, 1);
    CHECK_EQUAL(list.get(0), gen.convert_for_test<underlying_type>(2));
    CHECK_EQUAL(list.get(1), gen.convert_for_test<underlying_type>(1));
    CHECK_EQUAL(list.find_first(gen.convert_for_test<underlying_type>(2)), 0);
    CHECK_EQUAL(list.find_first(gen.convert_for_test<underlying_type>(1)), 1);
    CHECK(!list.is_null(0));
    CHECK(!list.is_null(1));

    Lst<type> list1;
    CHECK_EQUAL(list1.size(), 0);
    list1 = list;
    CHECK_EQUAL(list1.size(), 2);
    list.add(gen.convert_for_test<underlying_type>(3));
    CHECK_EQUAL(list.size(), 3);
    CHECK_EQUAL(list1.size(), 3);

    Query q = table.where().size_equal(col, 3); // SizeListNode
    CHECK_EQUAL(q.count(), 1);
    q = table.column<Lst<type>>(col).size() == 3; // SizeOperator expresison
    CHECK_EQUAL(q.count(), 1);

    Lst<type> list2 = list;
    CHECK_EQUAL(list2.size(), 3);
    list2.clear();
    CHECK_EQUAL(list2.size(), 0);

    if constexpr (TEST_TYPE::is_nullable) {
        list2.insert_null(0);
        CHECK_EQUAL(list.size(), 1);
        type item0 = list2.get(0);
        CHECK(value_is_null(item0));
        CHECK(list.is_null(0));
        CHECK(list.get_any(0).is_null());
    }
}

TEST_TYPES(List_Sort, Prop<int64_t>, Prop<float>, Prop<double>, Prop<Decimal128>, Prop<ObjectId>, Prop<Timestamp>,
           Prop<String>, Prop<BinaryData>, Prop<UUID>, Nullable<int64_t>, Nullable<float>, Nullable<double>,
           Nullable<Decimal128>, Nullable<ObjectId>, Nullable<Timestamp>, Nullable<String>, Nullable<BinaryData>,
           Nullable<UUID>)
{
    using type = typename TEST_TYPE::type;
    using underlying_type = typename TEST_TYPE::underlying_type;

    TestValueGenerator gen;
    Group g;
    TableRef t = g.add_table("table");
    ColKey col = t->add_column_list(TEST_TYPE::data_type, "values", TEST_TYPE::is_nullable);

    auto obj = t->create_object();
    auto list = obj.get_list<type>(col);

    std::vector<type> values = gen.values_from_int<type>({9, 4, 2, 7, 4, 1, 8, 11, 3, 4, 5, 22});
    std::vector<size_t> indices;
    type default_or_null = TEST_TYPE::default_value();
    values.push_back(default_or_null);
    obj.set_list_values(col, values);

    CHECK(list.has_changed());
    CHECK_NOT(list.has_changed());

    auto cmp = [&]() {
        CHECK_EQUAL(values.size(), indices.size());
        for (size_t i = 0; i < values.size(); i++) {
            CHECK_EQUAL(values[i], list.get(indices[i]));
        }
    };
    std::sort(values.begin(), values.end(), ::less());
    list.sort(indices);
    cmp();
    std::sort(values.begin(), values.end(), ::greater());
    list.sort(indices, false);
    cmp();
    CHECK_NOT(list.has_changed());

    underlying_type new_value = gen.convert_for_test<underlying_type>(6);
    values.push_back(new_value);
    list.add(type(new_value));
    CHECK(list.has_changed());
    std::sort(values.begin(), values.end(), ::less());
    list.sort(indices);
    cmp();

    values.resize(7);
    obj.set_list_values(col, values);
    std::sort(values.begin(), values.end(), ::greater());
    list.sort(indices, false);
    cmp();
}

TEST_TYPES(List_Distinct, Prop<int64_t>, Prop<float>, Prop<double>, Prop<Decimal128>, Prop<ObjectId>, Prop<Timestamp>,
           Prop<String>, Prop<BinaryData>, Prop<UUID>, Nullable<int64_t>, Nullable<float>, Nullable<double>,
           Nullable<Decimal128>, Nullable<ObjectId>, Nullable<Timestamp>, Nullable<String>, Nullable<BinaryData>,
           Nullable<UUID>)
{
    using type = typename TEST_TYPE::type;
    TestValueGenerator gen;
    Group g;
    TableRef t = g.add_table("table");
    ColKey col = t->add_column_list(TEST_TYPE::data_type, "values", TEST_TYPE::is_nullable);

    auto obj = t->create_object();
    auto list = obj.get_list<type>(col);

    std::vector<type> values = gen.values_from_int<type>({9, 4, 2, 7, 4, 9, 8, 11, 2, 4, 5});
    std::vector<type> distinct_values = gen.values_from_int<type>({9, 4, 2, 7, 8, 11, 5});
    type default_or_null = TEST_TYPE::default_value();
    values.push_back(default_or_null);
    distinct_values.push_back(default_or_null);
    std::vector<size_t> indices;
    obj.set_list_values(col, values);

    auto cmp = [&]() {
        CHECK_EQUAL(distinct_values.size(), indices.size());
        for (size_t i = 0; i < distinct_values.size(); i++) {
            CHECK_EQUAL(distinct_values[i], list.get(indices[i]));
        }
    };

    list.distinct(indices);
    cmp();
    list.distinct(indices, true);
    std::sort(distinct_values.begin(), distinct_values.end(), std::less<type>());
    cmp();
    list.distinct(indices, false);
    std::sort(distinct_values.begin(), distinct_values.end(), std::greater<type>());
    cmp();
}

TEST(List_MixedSwap)
{
    Group g;
    TableRef t = g.add_table("table");
    ColKey col = t->add_column_list(type_Mixed, "values");
    BinaryData bin("foo", 3);

    auto obj = t->create_object();
    auto list = obj.get_list<Mixed>(col);
    list.add("a");
    list.add("b");
    list.add("c");
    list.add(bin);
    list.move(2, 0);
    CHECK_EQUAL(list.get(0).get_string(), "c");
    CHECK_EQUAL(list.get(1).get_string(), "a");
    CHECK_EQUAL(list.get(2).get_string(), "b");
    CHECK_EQUAL(list.get(3).get_binary(), bin);
    list.swap(3, 2);
    CHECK_EQUAL(list.get(0).get_string(), "c");
    CHECK_EQUAL(list.get(1).get_string(), "a");
    CHECK_EQUAL(list.get(2).get_binary(), bin);
    CHECK_EQUAL(list.get(3).get_string(), "b");
}

TEST(List_DecimalMinMax)
{
    SHARED_GROUP_TEST_PATH(path);
    std::unique_ptr<Replication> hist(make_in_realm_history());
    DBRef sg = DB::create(*hist, path, DBOptions(crypt_key()));
    auto t = sg->start_write();
    auto table = t->add_table("the_table");
    auto col = table->add_column_list(type_Decimal, "the column");
    Obj o = table->create_object();
    Lst<Decimal128> lst = o.get_list<Decimal128>(col);
    std::string larger_than_max_int64_t = "123.45e99";
    lst.add(Decimal128(larger_than_max_int64_t));
    CHECK_EQUAL(lst.size(), 1);
    CHECK_EQUAL(lst.get(0), Decimal128(larger_than_max_int64_t));
    size_t min_ndx = realm::npos;
    auto min = lst.min(&min_ndx);
    CHECK(min);
    CHECK_EQUAL(min_ndx, 0);
    CHECK_EQUAL(min->get<Decimal128>(), Decimal128(larger_than_max_int64_t));
    lst.clear();
    CHECK_EQUAL(lst.size(), 0);
    std::string smaller_than_min_int64_t = "-123.45e99";
    lst.add(Decimal128(smaller_than_min_int64_t));
    CHECK_EQUAL(lst.size(), 1);
    CHECK_EQUAL(lst.get(0), Decimal128(smaller_than_min_int64_t));
    size_t max_ndx = realm::npos;
    auto max = lst.max(&max_ndx);
    CHECK(max);
    CHECK_EQUAL(max_ndx, 0);
    CHECK_EQUAL(max->get<Decimal128>(), Decimal128(smaller_than_min_int64_t));
}


template <typename T, typename U = T>
void test_lists_numeric_agg(TestContext& test_context, DBRef sg, const realm::DataType type_id, U null_value = U{},
                            bool optional = false)
{
    auto t = sg->start_write();
    auto table = t->add_table("the_table");
    auto col = table->add_column_list(type_id, "the column", optional);
    Obj o = table->create_object();
    Lst<T> lst = o.get_list<T>(col);
    for (int j = -1000; j < 1000; ++j) {
        T value = T(j);
        lst.add(value);
    }
    if (optional) {
        // given that sum/avg do not count nulls and min/max ignore nulls,
        // adding any number of null values should not affect the results of any aggregates
        for (size_t i = 0; i < 1000; ++i) {
            lst.add(null_value);
        }
    }
    for (int j = -1000; j < 1000; ++j) {
        CHECK_EQUAL(lst.get(j + 1000), T(j));
    }
    {
        size_t ret_ndx = realm::npos;
        auto min = lst.min(&ret_ndx);
        CHECK(min);
        CHECK(!min->is_null());
        CHECK_EQUAL(ret_ndx, 0);
        CHECK_EQUAL(min->template get<ColumnMinMaxType<T>>(), ColumnMinMaxType<T>(-1000));
        auto max = lst.max(&ret_ndx);
        CHECK(max);
        CHECK(!max->is_null());
        CHECK_EQUAL(ret_ndx, 1999);
        CHECK_EQUAL(max->template get<ColumnMinMaxType<T>>(), ColumnMinMaxType<T>(999));
        size_t ret_count = 0;
        auto sum = lst.sum(&ret_count);
        CHECK(sum);
        CHECK(!sum->is_null());
        CHECK_EQUAL(ret_count, 2000);
        CHECK_EQUAL(sum->template get<ColumnSumType<T>>(), ColumnSumType<T>(-1000));
        auto avg = lst.avg(&ret_count);
        CHECK(avg);
        CHECK(!avg->is_null());
        CHECK_EQUAL(ret_count, 2000);
        CHECK_EQUAL(avg->template get<ColumnAverageType<T>>(),
                    (ColumnAverageType<T>(-1000) / ColumnAverageType<T>(2000)));
    }

    lst.clear();
    CHECK_EQUAL(lst.size(), 0);
    {
        size_t ret_ndx = realm::npos;
        auto min = lst.min(&ret_ndx);
        CHECK(min);
        CHECK_EQUAL(ret_ndx, realm::npos);
        ret_ndx = realm::npos;
        auto max = lst.max(&ret_ndx);
        CHECK(max);
        CHECK_EQUAL(ret_ndx, realm::npos);
        size_t ret_count = realm::npos;
        auto sum = lst.sum(&ret_count);
        CHECK(sum);
        CHECK_EQUAL(ret_count, 0);
        ret_count = realm::npos;
        auto avg = lst.avg(&ret_count);
        CHECK(avg);
        CHECK_EQUAL(ret_count, 0);
    }

    lst.add(T(1));
    {
        size_t ret_ndx = realm::npos;
        auto min = lst.min(&ret_ndx);
        CHECK(min);
        CHECK(!min->is_null());
        CHECK_EQUAL(ret_ndx, 0);
        CHECK_EQUAL(min->template get<ColumnMinMaxType<T>>(), ColumnMinMaxType<T>(1));
        auto max = lst.max(&ret_ndx);
        CHECK(max);
        CHECK(!max->is_null());
        CHECK_EQUAL(ret_ndx, 0);
        CHECK_EQUAL(max->template get<ColumnMinMaxType<T>>(), ColumnMinMaxType<T>(1));
        size_t ret_count = 0;
        auto sum = lst.sum(&ret_count);
        CHECK(sum);
        CHECK(!sum->is_null());
        CHECK_EQUAL(ret_count, 1);
        CHECK_EQUAL(sum->template get<ColumnSumType<T>>(), ColumnSumType<T>(1));
        auto avg = lst.avg(&ret_count);
        CHECK(avg);
        CHECK(!avg->is_null());
        CHECK_EQUAL(ret_count, 1);
        CHECK_EQUAL(avg->template get<ColumnAverageType<T>>(), ColumnAverageType<T>(1));
    }

    t->rollback();
}

TEST(List_AggOps)
{
    SHARED_GROUP_TEST_PATH(path);

    std::unique_ptr<Replication> hist(make_in_realm_history());
    DBRef sg = DB::create(*hist, path, DBOptions(crypt_key()));

    test_lists_numeric_agg<int64_t>(test_context, sg, type_Int);
    test_lists_numeric_agg<float>(test_context, sg, type_Float);
    test_lists_numeric_agg<double>(test_context, sg, type_Double);
    test_lists_numeric_agg<Decimal128>(test_context, sg, type_Decimal);

    test_lists_numeric_agg<Optional<int64_t>>(test_context, sg, type_Int, Optional<int64_t>{}, true);
    test_lists_numeric_agg<float>(test_context, sg, type_Float, realm::null::get_null_float<float>(), true);
    test_lists_numeric_agg<double>(test_context, sg, type_Double, realm::null::get_null_float<double>(), true);
    test_lists_numeric_agg<Decimal128>(test_context, sg, type_Decimal, Decimal128(realm::null()), true);
}

TEST(List_NestedListColumns)
{
    SHARED_GROUP_TEST_PATH(path);
    DBRef db = DB::create(make_in_realm_history(), path);
    auto tr = db->start_write();
    auto table = tr->add_table("table");
    auto int_col = table->add_column(type_Int, "int");
    auto int_list_col = table->add_column(type_Int, "int_list", false, {CollectionType::List});
    auto list_col1 =
        table->add_column(type_Int, "int_list_list", false, {CollectionType::List, CollectionType::List});
    auto list_col2 = table->add_column(type_Int, "int_dict_list_list", false,
                                       {CollectionType::Dictionary, CollectionType::List, CollectionType::List});

    tr->commit_and_continue_as_read();
    CHECK_EQUAL(table->get_nesting_levels(int_col), 0);
    CHECK(!int_col.is_list());
    CHECK_EQUAL(table->get_nesting_levels(int_list_col), 0);
    CHECK(int_list_col.is_list());
    CHECK_EQUAL(table->get_nesting_levels(list_col1), 1);
    CHECK_EQUAL(table->get_nesting_levels(list_col2), 2);

    tr->promote_to_write();
    Obj obj = table->create_object();
    auto int_lst = obj.get_list_ptr<Int>({"int_list"});
    CHECK_EQUAL(int_lst->size(), 0);
    int_lst = obj.get_list_ptr<Int>({"int_dict_list_list", "Foo", 0});
    int_lst->add(7);
    int_lst = obj.get_list_ptr<Int>({"int_dict_list_list", "Bar", 0});
    int_lst->add(5);
    tr->commit_and_continue_as_read();

    tr->promote_to_write();
    table->remove_column(list_col2);
    tr->verify();
    tr->commit_and_continue_as_read();
}

TEST(List_NestedList_Insert)
{
    SHARED_GROUP_TEST_PATH(path);
    DBRef db = DB::create(make_in_realm_history(), path);
    auto tr = db->start_write();
    auto table = tr->add_table("table");
    auto list_col1 =
        table->add_column(type_Int, "int_list_list", false, {CollectionType::List, CollectionType::List});
    auto list_col2 = table->add_column(type_Int, "int_dict_list_list", false,
                                       {CollectionType::Dictionary, CollectionType::List, CollectionType::List});
    CHECK_EQUAL(table->get_nesting_levels(list_col1), 1);
    CHECK_EQUAL(table->get_nesting_levels(list_col2), 2);
    Obj obj = table->create_object();

    CollectionListPtr list = obj.get_collection_list(list_col1);
    CHECK(list->is_empty());
    list->insert_collection(0);
    auto collection = list->get_collection(0);
    auto val = list->get_any(0);
    CHECK(val.is_type(type_List));
    dynamic_cast<Lst<Int>*>(collection.get())->add(5);

    auto dict = obj.get_collection_list(list_col2);
    dict->insert_collection("Foo");
    auto list_foo = dict->get_collection_list("Foo");
    val = obj.get_any(list_col2);
    CHECK(val.is_type(type_Dictionary));
    list_foo->insert_collection(0);
    auto list_foo_0 = list_foo->get_collection(0);
    dynamic_cast<Lst<Int>*>(list_foo_0.get())->add(5);

    // Get collection by path
    auto int_lst = obj.get_list_ptr<Int>({"int_dict_list_list", "Foo", 0});
    CHECK_EQUAL(int_lst->get(0), 5);

    dict->insert_collection("Foo");
    auto list3 = dict->get_collection_list("Foo");
    // list3 points to the same list as list2
    list3->insert_collection(0);
    auto collection3 = list3->get_collection(0);
    dynamic_cast<Lst<Int>*>(collection3.get())->insert(0, 8);
    // list2 must now update so that the following get() does not return 8
    CHECK_EQUAL(dynamic_cast<Lst<Int>*>(list_foo_0.get())->get(0), 5);

    tr->commit_and_continue_as_read();
    CHECK_NOT(list->is_empty());
    CHECK_EQUAL(obj.get_collection_list(list_col1)->get_collection(0)->get_any(0).get_int(), 5);
    // tr->to_json(std::cout);
    tr->promote_to_write();
    {
        list->insert_collection(0);
        auto lst = list->get_collection(0);
        dynamic_cast<Lst<Int>*>(lst.get())->add(47);

        obj.get_list_ptr<Int>({"int_dict_list_list", "Foo", 1})->set(0, 100);
    }
    tr->commit_and_continue_as_read();
    // tr->to_json(std::cout);
    CHECK_EQUAL(dynamic_cast<Lst<Int>*>(collection.get())->get(0), 5);
    CHECK_EQUAL(dynamic_cast<Lst<Int>*>(list_foo_0.get())->get(0), 100);

    tr->promote_to_write();
    obj.remove();
    tr->commit_and_continue_as_read();
    CHECK_EQUAL(list->size(), 0);
    CHECK_EQUAL(dict->size(), 0);
    CHECK_EQUAL(list_foo->size(), 0);
    CHECK_EQUAL(collection->size(), 0);
    CHECK_EQUAL(list_foo_0->size(), 0);
}

TEST(List_Nested_InMixed)
{
    SHARED_GROUP_TEST_PATH(path);
    DBRef db = DB::create(make_in_realm_history(), path);
    auto tr = db->start_write();
    auto table = tr->add_table("table");
    auto col_any = table->add_column(type_Mixed, "something");

    Obj obj = table->create_object();

    obj.set_collection(col_any, CollectionType::Dictionary);
    auto dict = obj.get_dictionary_ptr(col_any);
    CHECK(dict->is_empty());
    dict->insert("Four", 4);
    obj.set_collection(col_any, CollectionType::Dictionary); // Idempotent
    tr->verify();
    tr->commit_and_continue_as_read();
    /*
    {
      "table": [
        {
          "_key": 0,
          "something": {
            "Four": 4
          }
        }
      ]
    }
    */
    CHECK_EQUAL(dict->get("Four"), Mixed(4));

    tr->promote_to_write();
    dict->insert_collection("Dict", CollectionType::Dictionary);
    auto dict2 = dict->get_dictionary("Dict");
    CHECK(dict2->is_empty());
    dict2->insert("Five", 5);
    tr->verify();
    tr->commit_and_continue_as_read();
    /*
    {
      "table": [
        {
          "_key": 0,
          "something": {
            "Dict": {
              "Five": 5
            },
            "Four": 4
          }
        }
      ]
    }
    */

    tr->promote_to_write();
    dict2->insert_collection("List", CollectionType::List);
    {
        auto list = dict2->get_list("List");
        CHECK(list->is_empty());
        list->add(8);
        list->add(9);
    }
    tr->verify();
    {
        std::stringstream ss;
        tr->to_json(ss, 0, nullptr, JSONOutputMode::output_mode_xjson_plus);
        auto j = nlohmann::json::parse(ss.str());
    }
    // std::cout << std::setw(2) << j << std::endl;
    tr->commit_and_continue_as_read();
    /*
    {
      "table": [
        {
          "_key": 0,
          "something": {
            "Dict": {
              "Five": 5,
              "List": [
                8,
                9
              ]
            },
            "Four": 4
          }
        }
      ]
    }
    */

    auto list = obj.get_collection_ptr({"something", "Dict", "List"});
    CHECK_EQUAL(dynamic_cast<Lst<Mixed>*>(list.get())->get(0).get_int(), 8);

    tr->promote_to_write();
    // Assign another value. The old dictionary should be disposed.
    obj.set(col_any, Mixed(5));
    tr->verify();
    tr->commit_and_continue_as_read();

    tr->promote_to_write();
    obj.set_collection(col_any, CollectionType::List);
    auto list2 = std::dynamic_pointer_cast<Lst<Mixed>>(obj.get_collection_ptr(col_any));
    CHECK(list2->is_empty());
    list2->add("Hello");
    list2->insert_collection(0, CollectionType::Dictionary);
    list2->add(42);
    dict2 = list2->get_dictionary(0);
    dict2->insert("Six", 6);
    tr->verify();
    dict2->insert("Seven", 7);
    list2->set_collection(2, CollectionType::Dictionary);
    dict2 = list2->get_dictionary(2);
    dict2->insert("Hello", "World");
    dict2->insert("Date", Timestamp(std::chrono::system_clock::now()));
    {
        std::stringstream ss;
        tr->to_json(ss, 0, nullptr, JSONOutputMode::output_mode_xjson_plus);
        auto j = nlohmann::json::parse(ss.str());
        // std::cout << std::setw(2) << j << std::endl;
    }
    tr->verify();
    tr->commit_and_continue_as_read();
    /*
    {
      "table": [
        {
          "_key": 0,
          "something": [
            {
              "Seven": 7,
              "Six": 6
            },
            "Hello",
            {
              "Date": "2023-05-09 07:52:49",
              "Hello": "World"
            }
          ]
        }
      ]
    }
    */
    CHECK_EQUAL(list2->get(1), Mixed("Hello"));
    tr->promote_to_write();
    list2->remove(1);
    CHECK_EQUAL(dict2->get("Hello"), Mixed("World"));
}

TEST(List_NestedList_Remove)
{
    SHARED_GROUP_TEST_PATH(path);
    DBRef db = DB::create(make_in_realm_history(), path);
    auto tr = db->start_write();
    auto table = tr->add_table("table");
    auto list_col = table->add_column(type_Int, "int_list_list", false, {CollectionType::List, CollectionType::List});
    auto list_col2 = table->add_column(type_Int, "int_dict_list_list", false,
                                       {CollectionType::Dictionary, CollectionType::List, CollectionType::List});

    CHECK_EQUAL(table->get_nesting_levels(list_col), 1);
    CHECK_EQUAL(table->get_nesting_levels(list_col2), 2);

    Obj obj = table->create_object();
    auto list1 = obj.get_list_ptr<Int>({"int_list_list", 0});
    list1->add(5);
    auto list2 = obj.get_list_ptr<Int>({"int_dict_list_list", "Foo", 0});
    list2->add(5);

    tr->commit_and_continue_as_read();
    CHECK_NOT(list1->is_empty());
    CHECK_NOT(list2->is_empty());
    CHECK_EQUAL(obj.get_collection_list(list_col)->get_collection(0)->get_any(0).get_int(), 5);
    CHECK_EQUAL(list2->get(0), 5);
    // transaction
    {
        tr->promote_to_write();

        list1->add(47);
        list2->set(0, 100);

        tr->commit_and_continue_as_read();
    }
    CHECK_EQUAL(list1->get(0), 5);
    CHECK_EQUAL(list1->get(1), 47);
    CHECK_EQUAL(list2->get(0), 100);

    tr->promote_to_write();
    obj.get_collection_list(list_col)->remove(0);
    CHECK_THROW_ANY(obj.get_collection_list(list_col2)->remove("Bar"));
    auto list_foo = obj.get_collection_list(list_col2)->get_collection_list("Foo");
    obj.get_collection_list(list_col2)->remove("Foo");
    // The above operation removed list_foo
    CHECK_THROW_ANY(list_foo->insert_collection(1));
    tr->verify();
    tr->commit_and_continue_as_read();

    CHECK_EQUAL(list1->size(), 0);
    CHECK_EQUAL(list2->size(), 0);
    tr->promote_to_write();
    obj.remove();
    tr->commit_and_continue_as_read();
}

TEST(List_NestedList_Links)
{
    SHARED_GROUP_TEST_PATH(path);
    DBRef db = DB::create(make_in_realm_history(), path);
    auto tr = db->start_write();
    auto target = tr->add_table("target");
    auto origin = tr->add_table("origin");
    auto list_col = origin->add_column(*target, "obj_list_list", {CollectionType::List, CollectionType::List});

    Obj o = origin->create_object();
    Obj t = target->create_object();

    auto list = o.get_collection_list(list_col);
    CHECK(list->is_empty());
    list->insert_collection(0);
    dynamic_cast<LnkLst*>(list->get_collection(0).get())->add(target->create_object().get_key());
    list->insert_collection(1);
    auto collection = list->get_collection(1);
    auto ll = dynamic_cast<LnkLst*>(collection.get());
    ll->add(t.get_key());
    CHECK_EQUAL(t.get_backlink_count(), 1);
    tr->commit_and_continue_as_read();
    tr->promote_to_write();
    t.remove();
    tr->commit_and_continue_as_read();
    CHECK_EQUAL(ll->size(), 0);
    tr->promote_to_write();
    t = target->create_object();
    ll->add(t.get_key());
    CHECK_EQUAL(t.get_backlink_count(), 1);
    list->remove(1);
    CHECK_EQUAL(t.get_backlink_count(), 0);
}

TEST(List_NestedList_Embedded)
{
    Group g;
    auto target = g.add_table("target", Table::Type::Embedded);
    auto origin = g.add_table("origin");
    auto list_col = origin->add_column(*target, "embedded", {CollectionType::List, CollectionType::List});

    Obj o = origin->create_object();

    {
        // Remove entry in parent list
        auto list = o.get_collection_list(list_col);
        CHECK(list->is_empty());
        list->insert_collection(0);
        auto collection = list->get_collection(0);
        auto ll = dynamic_cast<LnkLst*>(collection.get());
        ll->create_and_insert_linked_object(0);
        CHECK_EQUAL(target->size(), 1);
        list->remove(0);
        CHECK_EQUAL(target->size(), 0);
    }
    {
        // Remove object
        auto list = o.get_collection_list(list_col);
        CHECK(list->is_empty());
        list->insert_collection(0);
        auto collection = list->get_collection(0);
        auto ll = dynamic_cast<LnkLst*>(collection.get());
        ll->create_and_insert_linked_object(0);
        CHECK_EQUAL(target->size(), 1);
        o.remove();
        CHECK_EQUAL(target->size(), 0);
    }
    o = origin->create_object();
    {
        // Clear table
        auto list = o.get_collection_list(list_col);
        CHECK(list->is_empty());
        list->insert_collection(0);
        auto collection = list->get_collection(0);
        auto ll = dynamic_cast<LnkLst*>(collection.get());
        ll->create_and_insert_linked_object(0);
        CHECK_EQUAL(target->size(), 1);
        origin->clear();
        CHECK_EQUAL(target->size(), 0);
    }
}

TEST(List_NestedSet_Links)
{
    SHARED_GROUP_TEST_PATH(path);
    DBRef db = DB::create(make_in_realm_history(), path);
    auto tr = db->start_write();
    auto target = tr->add_table("target");
    auto origin = tr->add_table("origin");
    auto list_col = origin->add_column(*target, "obj_list_set", {CollectionType::List, CollectionType::Set});

    Obj o = origin->create_object();
    Obj t = target->create_object();

    auto list = o.get_collection_list(list_col);
    CHECK(list->is_empty());
    list->insert_collection(0);
    dynamic_cast<LnkSet*>(list->get_collection(0).get())->insert(target->create_object().get_key());
    list->insert_collection(1);
    auto collection = list->get_collection(1);
    auto ll = dynamic_cast<LnkSet*>(collection.get());
    ll->insert(t.get_key());
    CHECK_EQUAL(t.get_backlink_count(), 1);
    tr->commit_and_continue_as_read();
    tr->promote_to_write();
    t.remove();
    tr->commit_and_continue_as_read();
    CHECK_EQUAL(ll->size(), 0);
    tr->promote_to_write();
    t = target->create_object();
    ll->insert(t.get_key());
    CHECK_EQUAL(t.get_backlink_count(), 1);
    list->remove(1);
    CHECK_EQUAL(t.get_backlink_count(), 0);
}

TEST(List_NestedDict_Links)
{
    SHARED_GROUP_TEST_PATH(path);
    DBRef db = DB::create(make_in_realm_history(), path);
    auto tr = db->start_write();
    auto target = tr->add_table("target");
    auto origin = tr->add_table("origin");
    auto list_col = origin->add_column(*target, "obj_list_dict", {CollectionType::List, CollectionType::Dictionary});

    Obj o = origin->create_object();
    Obj t = target->create_object();

    auto list = o.get_collection_list(list_col);
    CHECK(list->is_empty());
    list->insert_collection(0);
    dynamic_cast<Dictionary*>(list->get_collection(0).get())->insert("Key", target->create_object().get_key());
    list->insert_collection(1);
    auto collection = list->get_collection(1);
    auto dict = dynamic_cast<Dictionary*>(collection.get());
    dict->insert("Hello", t.get_key());
    CHECK_EQUAL(t.get_backlink_count(), 1);
    tr->commit_and_continue_as_read();
    tr->promote_to_write();
    t.remove();
    tr->commit_and_continue_as_read();
    CHECK_EQUAL(dict->get("Hello"), Mixed());
    tr->promote_to_write();
    t = target->create_object();
    dict->insert("Hello", t.get_key());
    CHECK_EQUAL(t.get_backlink_count(), 1);
    list->remove(1);
    CHECK_EQUAL(t.get_backlink_count(), 0);
}

TEST(List_NestedDictList_Links)
{
    SHARED_GROUP_TEST_PATH(path);
    DBRef db = DB::create(make_in_realm_history(), path);
    auto tr = db->start_write();
    auto target = tr->add_table("target");
    auto origin = tr->add_table("origin");
    origin->add_column(*target, "obj_list_list",
                       {CollectionType::Dictionary, CollectionType::List, CollectionType::List});

    Obj o = origin->create_object();
    Obj t = target->create_object();

    auto foo_coll_0 = o.get_collection_ptr({"obj_list_list", "Foo", 0});
    auto foo_coll_1 = o.get_collection_ptr({"obj_list_list", "Foo", 1});
    auto bar_coll_0 = o.get_collection_ptr({"obj_list_list", "Bar", 0});
    auto bar_coll_1 = o.get_collection_ptr({"obj_list_list", "Bar", 1});
    auto foo_ll0 = dynamic_cast<LnkLst*>(foo_coll_0.get());
    auto foo_ll1 = dynamic_cast<LnkLst*>(foo_coll_1.get());
    auto bar_ll0 = dynamic_cast<LnkLst*>(bar_coll_0.get());
    auto bar_ll1 = dynamic_cast<LnkLst*>(bar_coll_1.get());

    foo_ll0->add(t.get_key());
    foo_ll1->add(target->create_object().get_key());
    bar_ll0->add(target->create_object().get_key());
    bar_ll1->add(target->create_object().get_key());
    CHECK_EQUAL(t.get_backlink_count(), 1);
    t.remove();
    CHECK_EQUAL(foo_ll0->size(), 0);
}

TEST(List_NestedList_Unresolved)
{
    SHARED_GROUP_TEST_PATH(path);
    DBRef db = DB::create(make_in_realm_history(), path);
    auto tr = db->start_write();
    auto target = tr->add_table_with_primary_key("target", type_String, "_id");
    auto origin = tr->add_table("origin");
    origin->add_column(*target, "links", {CollectionType::Dictionary, CollectionType::List, CollectionType::List});

    Obj o = origin->create_object();
    Obj t = target->create_object_with_primary_key("Adam");

    auto foo_coll_0 = o.get_collection_ptr({"links", "Foo", 0});
    auto foo_coll_1 = o.get_collection_ptr({"links", "Foo", 1});
    auto bar_coll_0 = o.get_collection_ptr({"links", "Bar", 0});
    auto bar_coll_1 = o.get_collection_ptr({"links", "Bar", 1});
    auto foo_ll0 = dynamic_cast<LnkLst*>(foo_coll_0.get());
    auto foo_ll1 = dynamic_cast<LnkLst*>(foo_coll_1.get());
    auto bar_ll0 = dynamic_cast<LnkLst*>(bar_coll_0.get());
    auto bar_ll1 = dynamic_cast<LnkLst*>(bar_coll_1.get());

    foo_ll0->add(t.get_key());
    foo_ll1->add(target->create_object_with_primary_key("Brian").get_key());
    bar_ll0->add(target->create_object_with_primary_key("Charlie").get_key());
    bar_ll1->add(target->create_object_with_primary_key("Daniel").get_key());
    CHECK_EQUAL(t.get_backlink_count(), 1);
    target->invalidate_object(t.get_key());
    CHECK_EQUAL(foo_ll0->size(), 0);
    target->create_object_with_primary_key("Adam");
    CHECK_EQUAL(foo_ll0->size(), 1);
}

TEST(List_NestedSet_Unresolved)
{
    SHARED_GROUP_TEST_PATH(path);
    DBRef db = DB::create(make_in_realm_history(), path);
    auto tr = db->start_write();
    auto target = tr->add_table_with_primary_key("target", type_String, "_id");
    auto origin = tr->add_table("origin");
    origin->add_column(type_Mixed, "links", true,
                       {CollectionType::Dictionary, CollectionType::List, CollectionType::Set});

    Obj o = origin->create_object();
    Obj t = target->create_object_with_primary_key("Adam");

    auto foo_coll_0 = o.get_collection_ptr({"links", "Foo", 0});
    auto foo_coll_1 = o.get_collection_ptr({"links", "Foo", 1});
    auto bar_coll_0 = o.get_collection_ptr({"links", "Bar", 0});
    auto bar_coll_1 = o.get_collection_ptr({"links", "Bar", 1});
    auto foo_ll0 = dynamic_cast<Set<Mixed>*>(foo_coll_0.get());
    auto foo_ll1 = dynamic_cast<Set<Mixed>*>(foo_coll_1.get());
    auto bar_ll0 = dynamic_cast<Set<Mixed>*>(bar_coll_0.get());
    auto bar_ll1 = dynamic_cast<Set<Mixed>*>(bar_coll_1.get());

    foo_ll0->insert(t.get_link());
    foo_ll0->insert(5);
    foo_ll0->insert("Hello");
    foo_ll1->insert(target->create_object_with_primary_key("Brian").get_link());
    bar_ll0->insert(target->create_object_with_primary_key("Charlie").get_link());
    bar_ll1->insert(target->create_object_with_primary_key("Daniel").get_link());
    CHECK_EQUAL(t.get_backlink_count(), 1);
    target->invalidate_object(t.get_key());
    auto obj = target->create_object_with_primary_key("Adam");
    CHECK_EQUAL(obj.get_backlink_count(), 1);
}

TEST(List_NestedDict_Unresolved)
{
    SHARED_GROUP_TEST_PATH(path);
    DBRef db = DB::create(make_in_realm_history(), path);
    auto tr = db->start_write();
    auto target = tr->add_table_with_primary_key("target", type_String, "_id");
    auto origin = tr->add_table("origin");
    origin->add_column(type_Mixed, "links", true,
                       {CollectionType::Dictionary, CollectionType::List, CollectionType::Dictionary});

    Obj o = origin->create_object();
    Obj t = target->create_object_with_primary_key("Adam");

    auto foo_coll_0 = o.get_collection_ptr({"links", "Foo", 0});
    auto foo_coll_1 = o.get_collection_ptr({"links", "Foo", 1});
    auto bar_coll_0 = o.get_collection_ptr({"links", "Bar", 0});
    auto bar_coll_1 = o.get_collection_ptr({"links", "Bar", 1});
    auto foo_ll0 = dynamic_cast<Dictionary*>(foo_coll_0.get());
    auto foo_ll1 = dynamic_cast<Dictionary*>(foo_coll_1.get());
    auto bar_ll0 = dynamic_cast<Dictionary*>(bar_coll_0.get());
    auto bar_ll1 = dynamic_cast<Dictionary*>(bar_coll_1.get());

    foo_ll0->insert("A", t.get_link());
    foo_ll0->insert("B", 5);
    foo_ll0->insert("C", "Hello");
    foo_ll1->insert("A", target->create_object_with_primary_key("Brian").get_link());
    bar_ll0->insert("A", target->create_object_with_primary_key("Charlie").get_link());
    bar_ll1->insert("A", target->create_object_with_primary_key("Daniel").get_link());
    CHECK_EQUAL(t.get_backlink_count(), 1);
    target->invalidate_object(t.get_key());
    CHECK(foo_ll0->get("A").is_null());
    auto obj = target->create_object_with_primary_key("Adam");
    CHECK_EQUAL(obj.get_backlink_count(), 1);
    CHECK_EQUAL(foo_ll0->get("A"), Mixed(obj.get_link()));
}

TEST(List_NestedList_Path)
{
    Group g;
    auto top_table = g.add_table_with_primary_key("top", type_String, "_id");
    auto embedded_table = g.add_table("embedded", Table::Type::Embedded);
    auto list_col =
        top_table->add_column(*embedded_table, "embedded_list", {CollectionType::List, CollectionType::List});
    auto dict_col =
        top_table->add_column(*embedded_table, "embedded_dict", {CollectionType::Dictionary, CollectionType::List});
    auto string_col = top_table->add_column_list(type_String, "strings");
    auto float_col =
        top_table->add_column(type_Float, "floats", false, {CollectionType::Dictionary, CollectionType::List});
    embedded_table->add_column(type_Int, "integers", false, {CollectionType::Dictionary, CollectionType::List});
    auto col_any = top_table->add_column(type_Mixed, "Any");

    Obj o = top_table->create_object_with_primary_key("Adam");

    // First level list
    {
        auto list_string = o.get_list<String>(string_col);
        auto path = list_string.get_path();
        CHECK_EQUAL(path.path_from_top.size(), 1);
        CHECK_EQUAL(path.path_from_top[0], string_col);
    }

    // List nested in Dictionary
    {
        auto list_float = o.get_list_ptr<Float>({"floats", "Foo"});
        list_float->add(5.f);
        auto path = list_float->get_path();
        CHECK_EQUAL(path.path_from_top.size(), 2);
        CHECK_EQUAL(path.path_from_top[0], float_col);
        CHECK_EQUAL(path.path_from_top[1], "Foo");
    }

    // List nested in Dictionary contained in embedded object contained in list of list
    {
        auto list = o.get_collection_list(list_col);
        list->insert_collection(0);
        list->insert_collection(1);
        list->insert_collection(2);
        auto coll = list->get_collection(2);
        auto ll = dynamic_cast<LnkLst*>(coll.get());
        ll->create_and_insert_linked_object(0);
        auto embedded_obj = ll->create_and_insert_linked_object(1);
        auto list_int = embedded_obj.get_list_ptr<Int>({"integers", "Foo"});
        list_int->add(5);
        auto path = list_int->get_path();
        CHECK_EQUAL(path.path_from_top.size(), 5);
        CHECK_EQUAL(path.path_from_top[0], list_col);
        CHECK_EQUAL(path.path_from_top[1], 2);
        CHECK_EQUAL(path.path_from_top[2], 1);
        CHECK_EQUAL(path.path_from_top[3], "integers");
        CHECK_EQUAL(path.path_from_top[4], "Foo");
    }

    // List nested in Dictionary contained in embedded object contained in Dictionary of list
    {
        auto list = o.get_collection_list(dict_col);
        list->insert_collection("A");
        list->insert_collection("B");
        list->insert_collection("C");
        auto coll = list->get_collection("C");
        auto ll = dynamic_cast<LnkLst*>(coll.get());
        ll->create_and_insert_linked_object(0);
        auto embedded_obj = ll->create_and_insert_linked_object(1);
        auto list_int = embedded_obj.get_list_ptr<Int>({"integers", "Foo"});
        list_int->add(5);
        auto path = list_int->get_path();
        CHECK_EQUAL(path.path_from_top.size(), 5);
        CHECK_EQUAL(path.path_from_top[0], dict_col);
        CHECK_EQUAL(path.path_from_top[1], "C");
        CHECK_EQUAL(path.path_from_top[2], 1);
        CHECK_EQUAL(path.path_from_top[3], "integers");
        CHECK_EQUAL(path.path_from_top[4], "Foo");
    }
    // Collections contained in Mixed
    {
        o.set_collection(col_any, CollectionType::Dictionary);
        auto dict = o.get_dictionary_ptr(col_any);
        dict->insert_collection("List", CollectionType::List);
        auto list = dict->get_list("List");
        list->add(Mixed(5));
        list->insert_collection(1, CollectionType::Dictionary);
        auto dict2 = list->get_dictionary(1);
        auto path = dict2->get_path();
        CHECK_EQUAL(path.path_from_top.size(), 3);
        CHECK_EQUAL(path.path_from_top[0], col_any);
        CHECK_EQUAL(path.path_from_top[1], "List");
        CHECK_EQUAL(path.path_from_top[2], 1);
    }
}

TEST(List_Nested_Replication)
{
    SHARED_GROUP_TEST_PATH(path);
    DBRef db = DB::create(make_in_realm_history(), path);
    auto tr = db->start_write();
    auto table = tr->add_table("table");
    auto col_any = table->add_column(type_Mixed, "something");

    Obj obj = table->create_object();

    obj.set_collection(col_any, CollectionType::Dictionary);
    auto dict = obj.get_dictionary_ptr(col_any);
    dict->insert_collection("level1", CollectionType::Dictionary);
    auto dict2 = dict->get_dictionary("level1");
    dict2->insert("Paul", "McCartney");
    tr->commit_and_continue_as_read();

    {
        auto wt = db->start_write();
        auto t = wt->get_table("table");
        auto o = *t->begin();
        auto d = o.get_collection_ptr({"something", "level1"});

        dynamic_cast<Dictionary*>(d.get())->insert("John", "Lennon");
        wt->commit();
    }

    struct Parser : _impl::NoOpTransactionLogParser {
        TestContext& test_context;
        Parser(TestContext& context)
            : test_context(context)
        {
        }

        bool collection_insert(size_t ndx)
        {
            auto collection_path = get_path();
            CHECK(collection_path[1] == expected_path[1]);
            CHECK(ndx == 0);
            return true;
        }

        Path expected_path;
    } parser(test_context);

    parser.expected_path.push_back("");
    parser.expected_path.push_back("level1");
    tr->advance_read(&parser);
}