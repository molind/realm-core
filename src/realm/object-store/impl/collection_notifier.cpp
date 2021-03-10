////////////////////////////////////////////////////////////////////////////
//
// Copyright 2016 Realm Inc.
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

#include <realm/object-store/impl/collection_notifier.hpp>

#include <realm/object-store/impl/realm_coordinator.hpp>
#include <realm/object-store/shared_realm.hpp>

#include <realm/db.hpp>
#include <realm/list.hpp>

using namespace realm;
using namespace realm::_impl;

bool CollectionNotifier::any_related_table_was_modified(TransactionChangeInfo const& info) const noexcept
{
    // Check if any of the tables accessible from the root table were
    // actually modified. This can be false if there were only insertions, or
    // deletions which were not linked to by any row in the linking table
    auto table_modified = [&](auto& outgoing_linkset) {
        auto it = info.tables.find(outgoing_linkset.first.value);
        return it != info.tables.end() && !it->second.modifications_empty();
    };
    return any_of(begin(m_related_tables), end(m_related_tables), table_modified);
}

std::function<bool(ObjectChangeSet::ObjectKeyType)>
CollectionNotifier::get_modification_checker(TransactionChangeInfo const& info, ConstTableRef root_table)
{
    if (info.schema_changed)
        set_table(root_table);

    if (!any_related_table_was_modified(info)) {
        return [](ObjectChangeSet::ObjectKeyType) {
            return false;
        };
    }

    if (m_related_tables.size() == 1) {
        auto& object_set = info.tables.find(m_related_tables.begin()->first.value)->second;
        return [&](ObjectChangeSet::ObjectKeyType object_key) {
            return object_set.modifications_contains(object_key);
        };
    }

    return DeepChangeChecker(info, *root_table, m_related_tables);
}

void DeepChangeChecker::find_related_tables(DeepChangeChecker::RelatedTables& out, Table const& table)
{
    auto table_key = table.get_key();
    if (out.find(table_key) != out.end())
        return;

    auto [it, inserted] = out.emplace(table_key, std::vector<ColKey>{});

    Group* g = table.get_parent_group();
    auto outgoing_links = table.get_outgoing_links();
    for (auto pair : outgoing_links) {
        it->second.emplace_back(pair.first);
        auto target = g->get_table(pair.second);
        find_related_tables(out, *target);
    }
}

DeepChangeChecker::DeepChangeChecker(TransactionChangeInfo const& info, Table const& root_table,
                                     DeepChangeChecker::RelatedTables const& related_tables)
    : m_info(info)
    , m_root_table(root_table)
    , m_root_table_key(root_table.get_key().value)
    , m_root_object_changes([&] {
        auto it = info.tables.find(m_root_table_key.value);
        return it != info.tables.end() ? &it->second : nullptr;
    }())
    , m_related_tables(related_tables)
{
}

bool DeepChangeChecker::check_outgoing_links(TableKey table_key, Table const& table, ObjKey obj_key, size_t depth)
{
    auto it = m_related_tables.find(table_key);
    if (it == m_related_tables.end())
        return false;
    if (it->second.empty())
        return false;

    // Check if we're already checking if the destination of the link is
    // modified, and if not add it to the stack
    auto already_checking = [&](ColKey col) {
        auto end = m_current_path.begin() + depth;
        auto match = std::find_if(m_current_path.begin(), end, [&](auto& p) {
            return p.obj_key == obj_key && p.col_key == col;
        });
        if (match != end) {
            for (; match < end; ++match)
                match->depth_exceeded = true;
            return true;
        }
        m_current_path[depth] = {obj_key, col, false};
        return false;
    };

    const Obj obj = table.get_object(ObjKey(obj_key));
    auto linked_object_changed = [&](ColKey const& outgoing_link_column) {
        if (already_checking(outgoing_link_column))
            return false;
        if (!outgoing_link_column.is_collection()) {
            if (obj.is_null(outgoing_link_column))
                return false;
            ObjKey dst = obj.get<ObjKey>(outgoing_link_column);
            REALM_ASSERT(dst);
            return check_row(*table.get_link_target(outgoing_link_column), dst.value, depth + 1);
        }

        auto target = table.get_link_target(outgoing_link_column);
        auto collection_ptr = obj.get_collection_ptr(outgoing_link_column);
        const size_t coll_size = collection_ptr->size();
        for (size_t i = 0; i < coll_size; ++i) {
            Mixed val = collection_ptr->get_any(i);
            ObjKey k;
            if (val.is_type(type_TypedLink)) {
                auto link = val.get_link();
                REALM_ASSERT(target && link.get_table_key() == target->get_key());
                k = link.get_obj_key();
            }
            else if (val.is_type(type_Link)) {
                k = val.get<ObjKey>();
            }
            if (k) {
                if (this->check_row(*target, k.value, depth + 1)) {
                    return true;
                }
            }
        }
        return false;
    };

    return std::any_of(begin(it->second), end(it->second), linked_object_changed);
}

bool DeepChangeChecker::check_row(Table const& table, ObjKeyType key, size_t depth)
{
    // Arbitrary upper limit on the maximum depth to search
    if (depth >= m_current_path.size()) {
        // Don't mark any of the intermediate rows checked along the path as
        // not modified, as a search starting from them might hit a modification
        for (size_t i = 0; i < m_current_path.size(); ++i)
            m_current_path[i].depth_exceeded = true;
        return false;
    }

    TableKey table_key = table.get_key();
    if (depth > 0) {
        auto it = m_info.tables.find(table_key.value);
        if (it != m_info.tables.end() && it->second.modifications_contains(key))
            return true;
    }
    auto& not_modified = m_not_modified[table_key.value];
    auto it = not_modified.find(key);
    if (it != not_modified.end())
        return false;

    bool ret = check_outgoing_links(table_key, table, ObjKey(key), depth);
    if (!ret && (depth == 0 || !m_current_path[depth - 1].depth_exceeded))
        not_modified.insert(key);
    return ret;
}

bool DeepChangeChecker::operator()(ObjKeyType key)
{
    if (m_root_object_changes && m_root_object_changes->modifications_contains(key))
        return true;
    return check_row(m_root_table, key, 0);
}

CollectionNotifier::CollectionNotifier(std::shared_ptr<Realm> realm)
    : m_realm(std::move(realm))
    , m_sg_version(Realm::Internal::get_transaction(*m_realm).get_version_of_current_transaction())
{
}

CollectionNotifier::~CollectionNotifier()
{
    // Need to do this explicitly to ensure m_realm is destroyed with the mutex
    // held to avoid potential double-deletion
    unregister();
}

void CollectionNotifier::release_data() noexcept
{
    m_sg = nullptr;
}

uint64_t CollectionNotifier::add_callback(CollectionChangeCallback callback)
{
    m_realm->verify_thread();

    util::CheckedLockGuard lock(m_callback_mutex);
    auto token = m_next_token++;
    m_callbacks.push_back({std::move(callback), {}, {}, token, false, false});
    if (m_callback_index == npos) { // Don't need to wake up if we're already sending notifications
        Realm::Internal::get_coordinator(*m_realm).wake_up_notifier_worker();
        m_have_callbacks = true;
    }
    return token;
}

void CollectionNotifier::remove_callback(uint64_t token)
{
    // the callback needs to be destroyed after releasing the lock as destroying
    // it could cause user code to be called
    Callback old;
    {
        util::CheckedLockGuard lock(m_callback_mutex);
        auto it = find_callback(token);
        if (it == end(m_callbacks)) {
            return;
        }

        size_t idx = distance(begin(m_callbacks), it);
        if (m_callback_index != npos) {
            if (m_callback_index >= idx)
                --m_callback_index;
        }
        --m_callback_count;

        old = std::move(*it);
        m_callbacks.erase(it);

        m_have_callbacks = !m_callbacks.empty();
    }
}

void CollectionNotifier::suppress_next_notification(uint64_t token)
{
    {
        std::lock_guard<std::mutex> lock(m_realm_mutex);
        REALM_ASSERT(m_realm);
        m_realm->verify_thread();
        m_realm->verify_in_write();
    }

    util::CheckedLockGuard lock(m_callback_mutex);
    auto it = find_callback(token);
    if (it != end(m_callbacks)) {
        it->skip_next = true;
    }
}

std::vector<CollectionNotifier::Callback>::iterator CollectionNotifier::find_callback(uint64_t token)
{
    REALM_ASSERT(m_error || m_callbacks.size() > 0);

    auto it = find_if(begin(m_callbacks), end(m_callbacks), [=](const auto& c) {
        return c.token == token;
    });
    // We should only fail to find the callback if it was removed due to an error
    REALM_ASSERT(m_error || it != end(m_callbacks));
    return it;
}

void CollectionNotifier::unregister() noexcept
{
    std::lock_guard<std::mutex> lock(m_realm_mutex);
    m_realm = nullptr;
}

bool CollectionNotifier::is_alive() const noexcept
{
    std::lock_guard<std::mutex> lock(m_realm_mutex);
    return m_realm != nullptr;
}

std::unique_lock<std::mutex> CollectionNotifier::lock_target()
{
    return std::unique_lock<std::mutex>{m_realm_mutex};
}

void CollectionNotifier::set_table(ConstTableRef table)
{
    m_related_tables.clear();
    DeepChangeChecker::find_related_tables(m_related_tables, *table);
}

void CollectionNotifier::add_required_change_info(TransactionChangeInfo& info)
{
    if (!do_add_required_change_info(info) || m_related_tables.empty()) {
        return;
    }

    info.tables.reserve(m_related_tables.size());
    for (auto& tbl : m_related_tables)
        info.tables[tbl.first.value];
}

void CollectionNotifier::prepare_handover()
{
    REALM_ASSERT(m_sg);
    m_sg_version = m_sg->get_version_of_current_transaction();
    do_prepare_handover(*m_sg);
    add_changes(std::move(m_change));
    m_change = {};
    REALM_ASSERT(m_change.empty());
    m_has_run = true;

#ifdef REALM_DEBUG
    util::CheckedLockGuard lock(m_callback_mutex);
    for (auto& callback : m_callbacks)
        REALM_ASSERT(!callback.skip_next);
#endif
}

void CollectionNotifier::before_advance()
{
    for_each_callback([&](auto& lock, auto& callback) {
        if (callback.changes_to_deliver.empty()) {
            return;
        }

        auto changes = callback.changes_to_deliver;
        // acquire a local reference to the callback so that removing the
        // callback from within it can't result in a dangling pointer
        auto cb = callback.fn;
        lock.unlock_unchecked();
        cb.before(changes);
    });
}

void CollectionNotifier::after_advance()
{
    for_each_callback([&](auto& lock, auto& callback) {
        if (callback.initial_delivered && callback.changes_to_deliver.empty()) {
            return;
        }
        callback.initial_delivered = true;

        auto changes = std::move(callback.changes_to_deliver);
        // acquire a local reference to the callback so that removing the
        // callback from within it can't result in a dangling pointer
        auto cb = callback.fn;
        lock.unlock_unchecked();
        cb.after(changes);
    });
}

void CollectionNotifier::deliver_error(std::exception_ptr error)
{
    // Don't complain about double-unregistering callbacks
    m_error = true;

    m_callback_count = m_callbacks.size();
    for_each_callback([this, &error](auto& lock, auto& callback) {
        // acquire a local reference to the callback so that removing the
        // callback from within it can't result in a dangling pointer
        auto cb = std::move(callback.fn);
        auto token = callback.token;
        lock.unlock_unchecked();
        cb.error(error);

        // We never want to call the callback again after this, so just remove it
        this->remove_callback(token);
    });
}

bool CollectionNotifier::is_for_realm(Realm& realm) const noexcept
{
    std::lock_guard<std::mutex> lock(m_realm_mutex);
    return m_realm.get() == &realm;
}

bool CollectionNotifier::package_for_delivery()
{
    if (!prepare_to_deliver())
        return false;
    util::CheckedLockGuard lock(m_callback_mutex);
    for (auto& callback : m_callbacks) {
        callback.changes_to_deliver = std::move(callback.accumulated_changes).finalize();
        callback.accumulated_changes = {};
    }
    m_callback_count = m_callbacks.size();
    return true;
}

template <typename Fn>
void CollectionNotifier::for_each_callback(Fn&& fn)
{
    util::CheckedUniqueLock callback_lock(m_callback_mutex);
    REALM_ASSERT_DEBUG(m_callback_count <= m_callbacks.size());
    for (++m_callback_index; m_callback_index < m_callback_count; ++m_callback_index) {
        fn(callback_lock, m_callbacks[m_callback_index]);
        if (!callback_lock.owns_lock())
            callback_lock.lock_unchecked();
    }

    m_callback_index = npos;
}

void CollectionNotifier::attach_to(std::shared_ptr<Transaction> sg)
{
    do_attach_to(*sg);
    m_sg = std::move(sg);
}

Transaction& CollectionNotifier::source_shared_group()
{
    return Realm::Internal::get_transaction(*m_realm);
}

void CollectionNotifier::report_collection_root_is_deleted()
{
    if (!m_has_delivered_root_deletion_event) {
        m_change.collection_root_was_deleted = true;
        m_has_delivered_root_deletion_event = true;
    }
}

void CollectionNotifier::add_changes(CollectionChangeBuilder change)
{
    util::CheckedLockGuard lock(m_callback_mutex);
    for (auto& callback : m_callbacks) {
        if (callback.skip_next) {
            REALM_ASSERT_DEBUG(callback.accumulated_changes.empty());
            callback.skip_next = false;
        }
        else {
            if (&callback == &m_callbacks.back())
                callback.accumulated_changes.merge(std::move(change));
            else
                callback.accumulated_changes.merge(CollectionChangeBuilder(change));
        }
    }
}

NotifierPackage::NotifierPackage(std::exception_ptr error, std::vector<std::shared_ptr<CollectionNotifier>> notifiers,
                                 RealmCoordinator* coordinator)
    : m_notifiers(std::move(notifiers))
    , m_coordinator(coordinator)
    , m_error(std::move(error))
{
}

// Clang TSE seems to not like returning a unique_lock from a function
void NotifierPackage::package_and_wait(util::Optional<VersionID::version_type> target_version)
    NO_THREAD_SAFETY_ANALYSIS
{
    if (!m_coordinator || m_error || !*this)
        return;

    auto lock = m_coordinator->wait_for_notifiers([&] {
        if (!target_version)
            return true;
        return std::all_of(begin(m_notifiers), end(m_notifiers), [&](auto const& n) {
            return !n->have_callbacks() || (n->has_run() && n->version().version >= *target_version);
        });
    });

    // Package the notifiers for delivery and remove any which don't have anything to deliver
    auto package = [&](auto& notifier) {
        if (notifier->has_run() && notifier->package_for_delivery()) {
            m_version = notifier->version();
            return false;
        }
        return true;
    };
    m_notifiers.erase(std::remove_if(begin(m_notifiers), end(m_notifiers), package), end(m_notifiers));
    if (m_version && target_version && m_version->version < *target_version) {
        m_notifiers.clear();
        m_version = util::none;
    }
    REALM_ASSERT(m_version || m_notifiers.empty());

    m_coordinator = nullptr;
}

void NotifierPackage::before_advance()
{
    if (m_error)
        return;
    for (auto& notifier : m_notifiers)
        notifier->before_advance();
}

void NotifierPackage::after_advance()
{
    if (m_error) {
        for (auto& notifier : m_notifiers)
            notifier->deliver_error(m_error);
        return;
    }
    for (auto& notifier : m_notifiers)
        notifier->after_advance();
}

void NotifierPackage::add_notifier(std::shared_ptr<CollectionNotifier> notifier)
{
    m_notifiers.push_back(notifier);
    m_coordinator->register_notifier(notifier);
}
