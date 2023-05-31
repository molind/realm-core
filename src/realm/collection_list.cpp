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

#include <realm/collection_list.hpp>
#include <realm/obj.hpp>
#include <realm/table.hpp>
#include <realm/group.hpp>
#include <realm/cluster.hpp>
#include <realm/list.hpp>
#include "realm/array_string.hpp"
#include "realm/array_integer.hpp"

namespace realm {

/****************************** CollectionList *******************************/

CollectionList::CollectionList(std::shared_ptr<CollectionParent> parent, ColKey col_key, Index index,
                               CollectionType coll_type)
    : CollectionParent(parent->get_level() + 1)
    , m_owned_parent(parent)
    , m_parent(m_owned_parent.get())
    , m_index(index)
    , m_alloc(&get_table()->get_alloc())
    , m_col_key(col_key)
    , m_top(*m_alloc)
    , m_refs(*m_alloc)
    , m_coll_type(coll_type)
{
    m_top.set_parent(this, 0);
    m_refs.set_parent(&m_top, 1);
}

CollectionList::CollectionList(CollectionParent* obj, ColKey col_key)
    : m_parent(obj)
    , m_alloc(&get_table()->get_alloc())
    , m_col_key(col_key)
    , m_top(*m_alloc)
    , m_refs(*m_alloc)
    , m_coll_type(get_table()->get_nested_column_type(col_key, 0))
{
    m_top.set_parent(this, 0);
    m_refs.set_parent(&m_top, 1);
}

CollectionList::~CollectionList() {}

bool CollectionList::init_from_parent(bool allow_create) const
{
    auto ref = m_parent->get_collection_ref(m_index, m_coll_type);
    if ((ref || allow_create) && !m_keys) {
        switch (m_coll_type) {
            case CollectionType::Dictionary: {
                m_keys.reset(new BPlusTree<StringData>(*m_alloc));
                break;
            }
            case CollectionType::List: {
                m_keys.reset(new BPlusTree<Int>(*m_alloc));
                break;
            }
            default:
                break;
        }
        m_keys->set_parent(&m_top, 0);
    }
    if (ref) {
        m_top.init_from_ref(ref);
        m_keys->init_from_parent();
        m_refs.init_from_parent();
        // All is well
        return true;
    }

    if (!allow_create) {
        m_top.detach();
        return false;
    }

    m_top.create(Array::type_HasRefs, false, 2, 0);
    m_keys->create();
    m_refs.create();
    try {
        m_top.update_parent();
    }
    catch (const StaleAccessor& e) {
        m_top.destroy_deep();
        throw e;
    }

    return true;
}

Mixed CollectionList::get_any(size_t ndx) const
{
    auto sz = size();
    if (ndx >= sz) {
        throw OutOfBounds("CollectionList::get_collection_ptr()", ndx, sz);
    }

    ref_type ref = m_refs.get(ndx);
    return Mixed(ref, get_table()->get_collection_type(m_col_key, m_level));
}

void CollectionList::ensure_created()
{
    bool changed = m_parent->update_if_needed(); // Throws if the object does not exist.
    auto content_version = m_alloc->get_content_version();

    if (changed || content_version != m_content_version || !m_top.is_attached()) {
        bool attached = init_from_parent(true);
        m_content_version = m_alloc->get_content_version();
        REALM_ASSERT(attached);
    }
}

UpdateStatus CollectionList::update_if_needed_with_status() const noexcept
{
    UpdateStatus status = m_parent ? m_parent->update_if_needed_with_status() : UpdateStatus::Detached;

    if (status != UpdateStatus::Detached) {
        auto content_version = m_alloc->get_content_version();
        if (content_version != m_content_version) {
            m_content_version = content_version;
            status = UpdateStatus::Updated;
        }
    }
    switch (status) {
        case UpdateStatus::Detached: {
            m_top.detach();
            return UpdateStatus::Detached;
        }
        case UpdateStatus::NoChange:
            if (m_top.is_attached()) {
                return UpdateStatus::NoChange;
            }
            // The tree has not been initialized yet for this accessor, so
            // perform lazy initialization by treating it as an update.
            [[fallthrough]];
        case UpdateStatus::Updated: {
            bool attached = init_from_parent(false);
            m_content_version = m_alloc->get_content_version();
            return attached ? UpdateStatus::Updated : UpdateStatus::Detached;
        }
    }
    REALM_UNREACHABLE();
}

bool CollectionList::update_if_needed() const
{
    auto status = update_if_needed_with_status();
    if (status == UpdateStatus::Detached) {
        throw StaleAccessor("CollectionList no longer exists");
    }
    return status == UpdateStatus::Updated;
}

auto CollectionList::get_path() const noexcept -> FullPath
{
    auto path = m_parent->get_path();
    m_parent->add_index(path.path_from_top, m_index);
    return path;
}

Path CollectionList::get_short_path() const noexcept
{
    auto path = m_parent->get_short_path();
    m_parent->add_index(path, m_index);
    return path;
}

StablePath CollectionList::get_stable_path() const noexcept
{
    auto path = m_parent->get_stable_path();
    path.push_back(m_index);
    return path;
}

void CollectionList::add_index(Path& path, Index index) const noexcept
{
    if (m_coll_type == CollectionType::List) {
        auto int_keys = static_cast<BPlusTree<Int>*>(m_keys.get());
        auto ndx = int_keys->find_first(mpark::get<int64_t>(index));
        REALM_ASSERT(ndx != realm::not_found);
        path.emplace_back(ndx);
    }
    else {
        path.emplace_back(mpark::get<std::string>(index));
    }
}


ref_type CollectionList::get_child_ref(size_t) const noexcept
{
    return m_parent->get_collection_ref(m_col_key, m_coll_type);
}

void CollectionList::update_child_ref(size_t, ref_type ref)
{
    m_parent->set_collection_ref(m_index, ref, m_coll_type);
}

void CollectionList::insert_collection(const PathElement& index, CollectionType)
{
    REALM_ASSERT(m_level <= get_table()->get_nesting_levels(m_col_key));
    ensure_created();
    size_t ndx;
    if (m_coll_type == CollectionType::List) {
        ndx = index.get_ndx();
        auto int_keys = static_cast<BPlusTree<Int>*>(m_keys.get());
        int64_t key = generate_key(size());
        while (int_keys->find_first(key) != realm::not_found) {
            key++;
        }
        int_keys->insert(ndx, key);
        m_refs.insert(ndx, 0);
    }
    else {
        auto key = index.get_key();
        auto string_keys = static_cast<BPlusTree<String>*>(m_keys.get());
        StringData actual;
        IteratorAdapter help(string_keys);
        auto it = std::lower_bound(help.begin(), help.end(), key);
        if (it.index() < string_keys->size()) {
            actual = *it;
        }
        if (actual != key) {
            string_keys->insert(it.index(), key);
            m_refs.insert(it.index(), 0);
        }
    }

    bump_content_version();
}

CollectionBasePtr CollectionList::get_collection(const PathElement& path_element) const
{
    REALM_ASSERT(get_table()->get_nesting_levels(m_col_key) == m_level);
    Index index = get_index(path_element);
    CollectionBasePtr coll = CollectionParent::get_collection_ptr(m_col_key);
    coll->set_owner(const_cast<CollectionList*>(this)->shared_from_this(), index);
    return coll;
}

auto CollectionList::get_index(const PathElement& path_element) const -> Index
{
    auto sz = size();
    if (path_element.is_ndx()) {
        size_t ndx = path_element.get_ndx();
        if (ndx >= sz) {
            throw OutOfBounds("CollectionList::get_collection...()", ndx, sz);
        }
        if (m_coll_type == CollectionType::List) {
            auto int_keys = static_cast<BPlusTree<Int>*>(m_keys.get());
            return int_keys->get(ndx);
        }
        else {
            auto string_keys = static_cast<BPlusTree<String>*>(m_keys.get());
            return std::string(string_keys->get(ndx));
        }
    }
    else {
        REALM_ASSERT(m_coll_type == CollectionType::Dictionary);
        auto key = path_element.get_key();
        auto string_keys = static_cast<BPlusTree<String>*>(m_keys.get());
        IteratorAdapter help(string_keys);
        auto it = std::lower_bound(help.begin(), help.end(), key);
        if (it == help.end() || *it != key) {
            throw KeyNotFound("CollectionList::get_collection_list");
        }
        return std::string(string_keys->get(it.index()));
    }
}

CollectionListPtr CollectionList::get_collection_list(const PathElement& path_element) const
{
    REALM_ASSERT(get_table()->get_nesting_levels(m_col_key) > m_level);
    Index index = get_index(path_element);
    auto coll_type = get_table()->get_nested_column_type(m_col_key, m_level);
    return CollectionList::create(const_cast<CollectionList*>(this)->shared_from_this(), m_col_key, index, coll_type);
}

void CollectionList::remove(size_t ndx)
{
    update();
    REALM_ASSERT(m_coll_type == CollectionType::List);
    auto int_keys = static_cast<BPlusTree<Int>*>(m_keys.get());
    const auto sz = int_keys->size();
    if (ndx >= sz) {
        throw OutOfBounds("CollectionList::remove", ndx, sz);
    }

    if (m_col_key.get_type() == col_type_LinkList || m_col_key.get_type() == col_type_Link) {
        std::vector<ObjKey> keys;
        auto origin_table = m_parent->get_table().unchecked_ptr();
        auto origin_key = m_parent->get_object().get_key();
        CascadeState state(CascadeState::Mode::Strong, origin_table->get_parent_group());

        get_all_keys(origin_table->get_nesting_levels(m_col_key) - m_level, keys);
        Cluster::remove_backlinks(origin_table, origin_key, m_col_key, keys, state);
        origin_table->remove_recursive(state);
    }

    int_keys->erase(ndx);
    auto ref = m_refs.get(ndx);
    Array::destroy_deep(ref, *m_alloc);
    m_refs.erase(ndx);

    bump_content_version();
}

void CollectionList::remove(StringData key)
{
    update();
    REALM_ASSERT(m_coll_type == CollectionType::Dictionary);
    auto string_keys = static_cast<BPlusTree<String>*>(m_keys.get());
    IteratorAdapter help(string_keys);
    auto it = std::lower_bound(help.begin(), help.end(), key);
    if (it.index() >= string_keys->size() || *it != key) {
        throw KeyNotFound("CollectionList::remove");
    }
    const auto index = it.index();
    string_keys->erase(index);
    auto ref = m_refs.get(index);
    Array::destroy_deep(ref, *m_alloc);
    m_refs.erase(index);

    bump_content_version();
}

ref_type CollectionList::get_collection_ref(Index index, CollectionType) const noexcept
{
    size_t ndx;
    if (m_coll_type == CollectionType::List) {
        auto int_keys = static_cast<BPlusTree<Int>*>(m_keys.get());
        ndx = int_keys->find_first(mpark::get<int64_t>(index));
    }
    else {
        auto string_keys = static_cast<BPlusTree<String>*>(m_keys.get());
        ndx = string_keys->find_first(StringData(mpark::get<std::string>(index)));
    }
    return ndx == realm::not_found ? 0 : m_refs.get(ndx);
}

void CollectionList::set_collection_ref(Index index, ref_type ref, CollectionType)
{
    size_t ndx;
    if (m_coll_type == CollectionType::List) {
        auto int_keys = static_cast<BPlusTree<Int>*>(m_keys.get());
        ndx = int_keys->find_first(mpark::get<int64_t>(index));
    }
    else {
        auto string_keys = static_cast<BPlusTree<String>*>(m_keys.get());
        ndx = string_keys->find_first(StringData(mpark::get<std::string>(index)));
    }
    if (ndx == realm::not_found) {
        throw StaleAccessor("Collection has been deleted");
    }
    m_refs.set(ndx, ref);
}

auto CollectionList::get_index(size_t ndx) const noexcept -> Index
{
    if (m_coll_type == CollectionType::List) {
        auto int_keys = static_cast<BPlusTree<Int>*>(m_keys.get());
        return int_keys->get(ndx);
    }
    else {
        auto string_keys = static_cast<BPlusTree<String>*>(m_keys.get());
        return string_keys->get(ndx);
    }
    return {};
}

void CollectionList::get_all_keys(size_t levels, std::vector<ObjKey>& keys) const
{
    if (!update()) {
        return;
    }
    for (size_t i = 0; i < size(); i++) {
        if (levels > 0) {
            get_collection_list(i)->get_all_keys(levels - 1, keys);
        }
        else {
            auto ref = m_refs.get(i);
            if (m_col_key.is_dictionary()) {
                Array top(*m_alloc);
                top.init_from_ref(ref);
                BPlusTree<Mixed> values(*m_alloc);
                values.set_parent(&top, 1);
                values.init_from_parent();
                for (size_t n = 0; n < values.size(); n++) {
                    Mixed value = values.get(n);
                    if (value.is_type(type_TypedLink)) {
                        keys.push_back(value.get<ObjKey>());
                    }
                }
            }
            else {
                BPlusTree<ObjKey> links(*m_alloc);
                links.init_from_ref(ref);
                if (links.size() > 0) {
                    auto vec = links.get_all();
                    std::move(vec.begin(), vec.end(), std::back_inserter(keys));
                }
            }
        }
    }
}

void CollectionList::to_json(std::ostream& out, size_t link_depth, JSONOutputMode output_mode,
                             util::FunctionRef<void(const Mixed&)> fn) const
{
    bool is_leaf = m_level == get_table()->get_nesting_levels(m_col_key);
    bool is_dictionary = m_coll_type == CollectionType::Dictionary;
    auto sz = size();
    auto string_keys = static_cast<BPlusTree<String>*>(m_keys.get());

    bool print_close = false;
    if (output_mode == output_mode_xjson_plus && is_dictionary) {
        out << "{ \"$dictionary\": ";
        print_close = true;
    }
    out << (is_dictionary ? "{" : "[");
    for (size_t i = 0; i < sz; i++) {
        if (i > 0)
            out << ",";
        if (is_dictionary) {
            out << Mixed(string_keys->get(i)) << ":";
        }
        if (is_leaf) {
            get_collection(i)->to_json(out, link_depth, output_mode, fn);
        }
        else {
            get_collection_list(i)->to_json(out, link_depth, output_mode, fn);
        }
    }
    out << (is_dictionary ? "}" : "]");
    if (print_close) {
        out << " }";
    }
}

} // namespace realm
