/*************************************************************************
 *
 * Copyright 2016 Realm Inc.
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

#include <realm/util/serializer.hpp>

#include <realm/binary_data.hpp>
#include <realm/geospatial.hpp>
#include <realm/keys.hpp>
#include <realm/null.hpp>
#include <realm/query_expression.hpp>
#include <realm/string_data.hpp>
#include <realm/table.hpp>
#include <realm/timestamp.hpp>
#include <realm/util/base64.hpp>

#include <cctype>
#include <cmath>
#include <iomanip>

namespace realm {

/* Uses Fliegel & Van Flandern algorithm */
static constexpr long date_to_julian(int y, int m, int d)
{
    return (1461 * (y + 4800 + (m - 14) / 12)) / 4 + (367 * (m - 2 - 12 * ((m - 14) / 12))) / 12 -
           (3 * ((y + 4900 + (m - 14) / 12) / 100)) / 4 + d - 32075;
}

static void julian_to_date(int jd, int* y, int* m, int* d)
{
    int L = jd + 68569;
    int n = (4 * L) / 146097;
    int i, j;

    L = L - (146097 * n + 3) / 4;
    i = (4000 * (L + 1)) / 1461001;
    L = L - (1461 * i) / 4 + 31;
    j = (80 * L) / 2447;
    *d = L - (2447 * j) / 80;
    L = j / 11;
    *m = j + 2 - (12 * L);
    *y = 100 * (n - 49) + i + L;
}

// Confirmed to work for all val < 16389
static void out_dec(char** buffer, unsigned val, int width)
{
    int w = width;
    char* p = *buffer;
    while (width > 0) {
        width--;
        unsigned div10 = (val * 6554) >> 16;
        p[width] = char(val - div10 * 10) + '0';
        val = div10;
    }
    *buffer += w;
}

static constexpr long epoc_julian_days = date_to_julian(1970, 1, 1); // 2440588
static constexpr int seconds_in_a_day = 24 * 60 * 60;

const char* Timestamp::to_string(std::array<char, 32>& buffer) const
{
    if (is_null()) {
        return "null";
    }
    int64_t seconds = get_seconds();
    int32_t nano = get_nanoseconds();
    if (nano < 0) {
        nano += Timestamp::nanoseconds_per_second;
        seconds--;
    }

    int64_t days = seconds / seconds_in_a_day;
    int julian_days = int(days + epoc_julian_days);

    int seconds_in_day = int(seconds - days * seconds_in_a_day);
    if (seconds_in_day < 0) {
        seconds_in_day += seconds_in_a_day;
        julian_days--;
    }

    int year;
    int month;
    int day;
    int hours = seconds_in_day / 3600;
    int remainingSeconds = seconds_in_day - hours * 3600;
    int minutes = remainingSeconds / 60;
    int secs = remainingSeconds - minutes * 60;

    julian_to_date(julian_days, &year, &month, &day);

    char* p = buffer.data();
    if (year < 0) {
        *p++ = '-';
        year = -year;
    }
    out_dec(&p, year, 4);
    *p++ = '-';
    out_dec(&p, month, 2);
    *p++ = '-';
    out_dec(&p, day, 2);
    *p++ = ' ';
    out_dec(&p, hours, 2);
    *p++ = ':';
    out_dec(&p, minutes, 2);
    *p++ = ':';
    out_dec(&p, secs, 2);
    *p = '\0';
    if (nano) {
        snprintf(p, 32 - (p - buffer.data()), ".%09d", nano);
    }
    return buffer.data();
}

namespace util {
namespace serializer {

template <>
std::string print_value<>(BinaryData data)
{
    if (data.is_null()) {
        return "NULL";
    }
    return util::format("binary(%1)", print_value<StringData>(StringData(data.data(), data.size())));
}

template <>
std::string print_value<>(bool b)
{
    if (b) {
        return "true";
    }
    return "false";
}

template <typename T>
inline std::string print_with_nan_check(T val)
{
    // we standardize NaN because some implementations (windows) will
    // print the different types of NaN such as "nan(ind)" to indicate "indefinite"
    if (std::isnan(val)) {
        // preserving the sign of nan is not strictly required but is good etiquette
        if (std::signbit(val)) {
            return "-nan";
        }
        return "nan";
    }
    std::stringstream ss;
    ss << std::setprecision(std::numeric_limits<T>::max_digits10) << val;
    return ss.str();
}

template <>
std::string print_value<>(float val)
{
    return print_with_nan_check(val);
}

template <>
std::string print_value<>(double val)
{
    return print_with_nan_check(val);
}

template <>
std::string print_value<>(realm::null)
{
    return "NULL";
}

static bool contains_invalids(StringData data)
{
    // the custom whitelist is different from std::isprint because it doesn't include quotations
    const static std::string whitelist = " {|}~:;<=>?@!#$%&()*+,-./[]^_`";
    const char* ptr = data.data();
    const char* end = ptr + data.size();
    while (ptr < end) {
        using unsigned_char_t = unsigned char;
        char c = *ptr;
        size_t len = sequence_length(c);
        if (len == 1) {
            // std::isalnum takes an int, but is undefined for negative values so we must pass an unsigned char
            if (!std::isalnum(unsigned_char_t(c)) && whitelist.find(c) == std::string::npos) {
                return true;
            }
        }
        else {
            // multibyte utf8, check if subsequent bytes have the upper bit set
            for (size_t i = 1; i < len; i++) {
                ++ptr;
                if (ptr == end || (*ptr & 0x80) == 0) {
                    return true;
                }
            }
        }
        ++ptr;
    }
    return false;
}

template <>
std::string print_value<>(StringData data)
{
    if (data.is_null()) {
        return "NULL";
    }
    std::string out;
    const char* start = data.data();
    const size_t len = data.size();

    if (contains_invalids(data)) {
        std::string encode_buffer;
        encode_buffer.resize(util::base64_encoded_size(len));
        util::base64_encode(data, encode_buffer);
        out = "B64\"" + encode_buffer + "\"";
    }
    else {
        out.reserve(len + 2);
        out += '"';
        for (const char* i = start; i != start + len; ++i) {
            out += *i;
        }
        out += '"';
    }
    return out;
}

template <>
std::string print_value<>(realm::Timestamp t)
{
    if (t.is_null()) {
        return "NULL";
    }
    std::stringstream ss;
    if (t.is_null()) {
        ss << "null";
    }
    else {
        ss << "T" << t.get_seconds() << ":" << t.get_nanoseconds();
    }
    return ss.str();
}

template <>
std::string print_value<>(realm::ObjectId oid)
{
    return "oid(" + oid.to_string() + ")";
}

template <>
std::string print_value<>(realm::ObjKey k)
{
    std::stringstream ss;
    if (!k) {
        ss << "NULL";
    }
    else {
        ss << "O" << k.value;
    }
    return ss.str();
}

std::string print_value(realm::ObjLink link, Group* g)
{
    if (!link) {
        return "NULL";
    }
    else {
        TableRef target_table = g ? g->get_table(link.get_table_key()) : TableRef();
        if (ColKey pk_col = target_table ? target_table->get_primary_key_column() : ColKey{}) {
            if (auto obj = target_table->try_get_object(link.get_obj_key())) {
                auto pk_val = obj.get_any(pk_col);
                std::ostringstream ostr;
                ostr << "obj(" << util::serializer::print_value(target_table->get_name()) << "," << pk_val << ')';
                return ostr.str();
            }
        }
    }
    std::stringstream ss;
    ss << "L" << link.get_table_key().value << ":" << link.get_obj_key().value;
    return ss.str();
}

template <>
std::string print_value<>(realm::UUID uuid)
{
    return "uuid(" + uuid.to_string() + ")";
}

template <>
std::string print_value<>(realm::TypeOfValue type)
{
    return '"' + type.to_string() + '"';
}

#if REALM_ENABLE_GEOSPATIAL
template <>
std::string print_value<>(const realm::Geospatial& geo)
{
    return geo.to_string();
}
#endif

// The variable name must be unique with respect to the already chosen variables at
// this level of subquery nesting and with respect to the names of the columns in the table.
// This assumes that columns can start with '$' and that we might one day want to support
// referencing the parent table columns in the subquery. This is currently disabled by an assertion in the
// core SubQuery constructor.
std::string SerialisationState::get_variable_name(ConstTableRef table)
{
    std::string guess_prefix = "$";
    const char start_char = 'x';
    char add_char = start_char;

    auto next_guess = [&]() {
        add_char = (((add_char + 1) - 'a') % ('z' - 'a' + 1)) + 'a';
        if (add_char == start_char) {
            guess_prefix += add_char;
        }
    };

    while (true) {
        std::string guess = guess_prefix + add_char;
        bool found_duplicate = false;
        for (size_t i = 0; i < subquery_prefix_list.size(); ++i) {
            if (guess == subquery_prefix_list[i]) {
                found_duplicate = true;
                break;
            }
        }
        if (found_duplicate) {
            next_guess();
            continue;
        }
        if (table->get_column_key(guess) != ColKey()) {
            next_guess();
            continue;
        }
        return guess;
    }
}

std::string SerialisationState::get_column_name(ConstTableRef table, ColKey col_key)
{
    ColumnType col_type = table->get_real_column_type(col_key);
    if (col_type == col_type_BackLink) {
        const Table::BacklinkOrigin origin = table->find_backlink_origin(col_key);
        REALM_ASSERT(origin);
        StringData source_table_name = origin->first->get_class_name();
        std::string source_col_name = get_column_name(origin->first, origin->second);

        return format("@links.%1.%2", source_table_name, source_col_name);
    }
    else if (col_key != ColKey()) {
        std::string col_name = table->get_column_name(col_key);
        size_t pos = col_name.find_first_of(" \t\r\n");
        while (pos != std::string::npos) {
            switch (col_name[pos]) {
                case ' ':
                    // space is unchanged
                    break;
                case '\t':
                    col_name[pos] = 't';
                    break;
                case '\r':
                    col_name[pos] = 'r';
                    break;
                case '\n':
                    col_name[pos] = 'n';
                    break;
            }
            col_name = col_name.substr(0, pos) + "\\" + col_name.substr(pos);
            pos += 2;
            pos = col_name.find_first_of(" \t\r\n", pos);
        }
        return col_name;
    }
    return "";
}

std::string SerialisationState::describe_column(ConstTableRef table, ColKey col_key)
{
    if (table && col_key) {
        std::string desc;
        if (!subquery_prefix_list.empty()) {
            desc += subquery_prefix_list.back() + value_separator;
        }
        desc += get_column_name(table, col_key);
        return desc;
    }
    return "";
}

std::string SerialisationState::get_backlink_column_name(ConstTableRef from, ColKey col_key)
{
    ColumnType col_type = col_key.get_type();
    REALM_ASSERT_EX(col_type == col_type_Link, col_type);

    auto target_table = from->get_opposite_table(col_key);
    auto backlink_col = from->get_opposite_column(col_key);
    return get_column_name(target_table, backlink_col);
}

std::string SerialisationState::describe_columns(const LinkMap& link_map, ColKey target_col_key)
{
    std::string desc;
    if (!subquery_prefix_list.empty()) {
        desc += subquery_prefix_list.back();
    }
    if (link_map.links_exist()) {
        if (!desc.empty()) {
            desc += util::serializer::value_separator;
        }
        desc += link_map.description(*this);
    }
    ConstTableRef target = link_map.get_target_table();
    if (target && target_col_key) {
        if (!desc.empty()) {
            desc += util::serializer::value_separator;
        }
        desc += get_column_name(target, target_col_key);
    }
    return desc;
}

std::string SerialisationState::describe_expression_type(util::Optional<ExpressionComparisonType> type)
{
    if (type) {
        switch (*type) {
            case ExpressionComparisonType::Any:
                return "ANY ";
            case ExpressionComparisonType::All:
                return "ALL ";
            case ExpressionComparisonType::None:
                return "NONE ";
        }
    }
    return "";
}

} // namespace serializer
} // namespace util
} // namespace realm
