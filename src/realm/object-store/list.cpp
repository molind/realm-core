////////////////////////////////////////////////////////////////////////////
//
// Copyright 2015 Realm Inc.
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

#include <realm/object-store/list.hpp>

#include <realm/object-store/object_schema.hpp>
#include <realm/object-store/object_store.hpp>
#include <realm/object-store/results.hpp>
#include <realm/object-store/schema.hpp>
#include <realm/object-store/shared_realm.hpp>

namespace {
using namespace realm;

template <typename T>
struct ListType {
    using type = Lst<T>;
};

template <>
struct ListType<Obj> {
    using type = LnkLst;
};

} // namespace

namespace realm {
using namespace _impl;

List::List() noexcept = default;
List::~List() = default;

List::List(const List&) = default;
List& List::operator=(const List&) = default;
List::List(List&&) = default;
List& List::operator=(List&&) = default;

List::List(std::shared_ptr<Realm> r, const Obj& parent_obj, ColKey col)
    : Collection(std::move(r), parent_obj, col)
    , m_list_base(std::dynamic_pointer_cast<LstBase>(m_coll_base))
    , m_is_embedded(m_type == PropertyType::Object && as<Obj>().get_target_table()->is_embedded())
{
}

List::List(std::shared_ptr<Realm> r, const LstBase& list)
    : Collection(std::move(r), list)
    , m_list_base(std::dynamic_pointer_cast<LstBase>(m_coll_base))
    , m_is_embedded(m_type == PropertyType::Object && as<Obj>().get_target_table()->is_embedded())
{
}

Query List::get_query() const
{
    return get_table()->where(as<Obj>());
}

ConstTableRef List::get_table() const
{
    verify_attached();
    if (m_type == PropertyType::Object)
        return m_list_base->get_target_table();
    throw std::runtime_error("not implemented");
}

template <typename T>
T List::get(size_t row_ndx) const
{
    verify_valid_row(row_ndx);
    return as<T>().get(row_ndx);
}

template <>
Obj List::get(size_t row_ndx) const
{
    verify_valid_row(row_ndx);
    auto& list = as<Obj>();
    return list.get_target_table()->get_object(list.get(row_ndx));
}

template <typename T>
size_t List::find(T const& value) const
{
    verify_attached();
    return as<T>().find_first(value);
}

template <>
size_t List::find(Obj const& o) const
{
    verify_attached();
    if (!o.is_valid())
        return not_found;
    validate(o);

    return as<Obj>().find_first(o.get_key());
}

size_t List::find(Query&& q) const
{
    verify_attached();
    if (m_type == PropertyType::Object) {
        ObjKey key = get_query().and_query(std::move(q)).find();
        return key ? as<Obj>().find_first(key) : not_found;
    }
    throw std::runtime_error("not implemented");
}

template <typename T>
void List::add(T value)
{
    verify_in_transaction();
    as<T>().add(value);
}

template <>
void List::add(Obj o)
{
    verify_in_transaction();
    if (m_is_embedded)
        throw InvalidEmbeddedOperationException();
    validate(o);
    as<Obj>().add(o.get_key());
}

template <typename T>
void List::insert(size_t row_ndx, T value)
{
    verify_in_transaction();
    verify_valid_row(row_ndx, true);
    as<T>().insert(row_ndx, value);
}

template <>
void List::insert(size_t row_ndx, Obj o)
{
    verify_in_transaction();
    verify_valid_row(row_ndx, true);
    validate(o);
    if (m_is_embedded)
        throw InvalidEmbeddedOperationException();
    as<Obj>().insert(row_ndx, o.get_key());
}

void List::move(size_t source_ndx, size_t dest_ndx)
{
    verify_in_transaction();
    verify_valid_row(source_ndx);
    verify_valid_row(dest_ndx); // Can't be one past end due to removing one earlier
    if (source_ndx == dest_ndx)
        return;

    m_list_base->move(source_ndx, dest_ndx);
}

void List::remove(size_t row_ndx)
{
    verify_in_transaction();
    verify_valid_row(row_ndx);
    m_list_base->remove(row_ndx, row_ndx + 1);
}

void List::remove_all()
{
    verify_in_transaction();
    m_list_base->clear();
}

template <typename T>
void List::set(size_t row_ndx, T value)
{
    verify_in_transaction();
    verify_valid_row(row_ndx);
    //    validate(row);
    as<T>().set(row_ndx, value);
}

void List::insert_any(size_t row_ndx, Mixed value)
{
    verify_in_transaction();
    verify_valid_row(row_ndx, true);
    m_list_base->insert_any(row_ndx, value);
}

void List::set_any(size_t row_ndx, Mixed value)
{
    verify_in_transaction();
    verify_valid_row(row_ndx);
    m_list_base->set_any(row_ndx, value);
}

Mixed List::get_any(size_t row_ndx) const
{
    verify_valid_row(row_ndx);
    return m_list_base->get_any(row_ndx);
}

size_t List::find_any(Mixed value) const
{
    return m_list_base->find_any(value);
}

template <>
void List::set(size_t row_ndx, Obj o)
{
    verify_in_transaction();
    verify_valid_row(row_ndx);
    validate(o);
    if (m_is_embedded)
        throw InvalidEmbeddedOperationException();
    as<Obj>().set(row_ndx, o.get_key());
}

Obj List::add_embedded()
{
    verify_in_transaction();

    if (!m_is_embedded)
        throw InvalidEmbeddedOperationException();

    return as<Obj>().create_and_insert_linked_object(size());
}

Obj List::set_embedded(size_t list_ndx)
{
    verify_in_transaction();
    verify_valid_row(list_ndx);

    if (!m_is_embedded)
        throw InvalidEmbeddedOperationException();

    return as<Obj>().create_and_set_linked_object(list_ndx);
}

Obj List::insert_embedded(size_t list_ndx)
{
    verify_in_transaction();
    verify_valid_row(list_ndx, true);

    if (!m_is_embedded)
        throw InvalidEmbeddedOperationException();

    return as<Obj>().create_and_insert_linked_object(list_ndx);
}


void List::swap(size_t ndx1, size_t ndx2)
{
    verify_in_transaction();
    verify_valid_row(ndx1);
    verify_valid_row(ndx2);
    m_list_base->swap(ndx1, ndx2);
}

void List::delete_at(size_t row_ndx)
{
    verify_in_transaction();
    verify_valid_row(row_ndx);
    if (m_type == PropertyType::Object)
        as<Obj>().remove_target_row(row_ndx);
    else
        m_list_base->remove(row_ndx, row_ndx + 1);
}

void List::delete_all()
{
    verify_in_transaction();
    if (m_type == PropertyType::Object)
        as<Obj>().remove_all_target_rows();
    else
        m_list_base->clear();
}

Results List::sort(SortDescriptor order) const
{
    verify_attached();
    if ((m_type == PropertyType::Object)) {
        return Results(m_realm, std::dynamic_pointer_cast<LnkLst>(m_list_base), util::none, std::move(order));
    }
    else {
        DescriptorOrdering o;
        o.append_sort(order);
        return Results(m_realm, m_list_base, std::move(o));
    }
}

Results List::sort(std::vector<std::pair<std::string, bool>> const& keypaths) const
{
    return as_results().sort(keypaths);
}

Results List::filter(Query q) const
{
    verify_attached();
    return Results(m_realm, std::dynamic_pointer_cast<LnkLst>(m_list_base), get_query().and_query(std::move(q)));
}

Results List::snapshot() const
{
    return as_results().snapshot();
}

// The simpler definition of void_t below does not work in gcc 4.9 due to a bug
// in that version of gcc (https://gcc.gnu.org/bugzilla/show_bug.cgi?id=64395)

// template<class...> using VoidT = void;
namespace _impl {
template <class...>
struct MakeVoid {
    using type = void;
};
} // namespace _impl
template <class... T>
using VoidT = typename _impl::MakeVoid<T...>::type;

template <class, class = VoidT<>>
struct HasMinmaxType : std::false_type {
};
template <class T>
struct HasMinmaxType<T, VoidT<typename ColumnTypeTraits<T>::minmax_type>> : std::true_type {
};

template <class, class = VoidT<>>
struct HasSumType : std::false_type {
};
template <class T>
struct HasSumType<T, VoidT<typename ColumnTypeTraits<T>::sum_type>> : std::true_type {
};

template <bool cond>
struct If;

template <>
struct If<true> {
    template <typename T, typename Then, typename Else>
    static auto call(T self, Then&& fn, Else&&)
    {
        return fn(self);
    }
};
template <>
struct If<false> {
    template <typename T, typename Then, typename Else>
    static auto call(T, Then&&, Else&& fn)
    {
        return fn();
    }
};

util::Optional<Mixed> List::max(ColKey col) const
{
    if (get_type() == PropertyType::Object)
        return as_results().max(col);
    size_t out_ndx = not_found;
    auto result = m_list_base->max(&out_ndx);
    if (result.is_null()) {
        throw realm::Results::UnsupportedColumnTypeException(m_list_base->get_col_key(), m_list_base->get_table(),
                                                             "max");
    }
    return out_ndx == not_found ? none : util::make_optional(result);
}

util::Optional<Mixed> List::min(ColKey col) const
{
    if (get_type() == PropertyType::Object)
        return as_results().min(col);

    size_t out_ndx = not_found;
    auto result = m_list_base->min(&out_ndx);
    if (result.is_null()) {
        throw realm::Results::UnsupportedColumnTypeException(m_list_base->get_col_key(), m_list_base->get_table(),
                                                             "min");
    }
    return out_ndx == not_found ? none : util::make_optional(result);
}

Mixed List::sum(ColKey col) const
{
    if (get_type() == PropertyType::Object)
        return *as_results().sum(col);

    auto result = m_list_base->sum();
    if (result.is_null()) {
        throw realm::Results::UnsupportedColumnTypeException(m_list_base->get_col_key(), m_list_base->get_table(),
                                                             "sum");
    }
    return result;
}

util::Optional<Mixed> List::average(ColKey col) const
{
    if (get_type() == PropertyType::Object)
        return as_results().average(col);
    size_t count = 0;
    auto result = m_list_base->avg(&count);
    if (result.is_null()) {
        throw realm::Results::UnsupportedColumnTypeException(m_list_base->get_col_key(), m_list_base->get_table(),
                                                             "average");
    }
    return count == 0 ? none : util::make_optional(result);
}

bool List::operator==(List const& rgt) const noexcept
{
    return m_list_base->get_table() == rgt.m_list_base->get_table() &&
           m_list_base->get_key() == rgt.m_list_base->get_key() &&
           m_list_base->get_col_key() == rgt.m_list_base->get_col_key();
}

List List::freeze(std::shared_ptr<Realm> const& frozen_realm) const
{
    REALM_ASSERT(frozen_realm->is_frozen());
    return List(frozen_realm, *frozen_realm->import_copy_of(*m_list_base));
}

List List::thaw(std::shared_ptr<Realm> const& live_realm) const
{
    REALM_ASSERT(!live_realm->is_frozen());
    return List(live_realm, *live_realm->import_copy_of(*m_list_base));
}

#define REALM_PRIMITIVE_LIST_TYPE(T)                                                                                 \
    template T List::get<T>(size_t) const;                                                                           \
    template size_t List::find<T>(T const&) const;                                                                   \
    template void List::add<T>(T);                                                                                   \
    template void List::insert<T>(size_t, T);                                                                        \
    template void List::set<T>(size_t, T);

REALM_PRIMITIVE_LIST_TYPE(bool)
REALM_PRIMITIVE_LIST_TYPE(int64_t)
REALM_PRIMITIVE_LIST_TYPE(float)
REALM_PRIMITIVE_LIST_TYPE(double)
REALM_PRIMITIVE_LIST_TYPE(StringData)
REALM_PRIMITIVE_LIST_TYPE(BinaryData)
REALM_PRIMITIVE_LIST_TYPE(Timestamp)
REALM_PRIMITIVE_LIST_TYPE(ObjKey)
REALM_PRIMITIVE_LIST_TYPE(ObjectId)
REALM_PRIMITIVE_LIST_TYPE(Decimal)
REALM_PRIMITIVE_LIST_TYPE(UUID)
REALM_PRIMITIVE_LIST_TYPE(Mixed)
REALM_PRIMITIVE_LIST_TYPE(util::Optional<bool>)
REALM_PRIMITIVE_LIST_TYPE(util::Optional<int64_t>)
REALM_PRIMITIVE_LIST_TYPE(util::Optional<float>)
REALM_PRIMITIVE_LIST_TYPE(util::Optional<double>)
REALM_PRIMITIVE_LIST_TYPE(util::Optional<ObjectId>)
REALM_PRIMITIVE_LIST_TYPE(util::Optional<UUID>)

#undef REALM_PRIMITIVE_LIST_TYPE
} // namespace realm

namespace {
size_t hash_combine()
{
    return 0;
}
template <typename T, typename... Rest>
size_t hash_combine(const T& v, Rest... rest)
{
    size_t h = hash_combine(rest...);
    h ^= std::hash<T>()(v) + 0x9e3779b9 + (h << 6) + (h >> 2);
    return h;
}
} // namespace

namespace std {
size_t hash<List>::operator()(List const& list) const
{
    auto& impl = *list.m_list_base;
    return hash_combine(impl.get_key().value, impl.get_table()->get_key().value, impl.get_col_key().value);
}
} // namespace std
