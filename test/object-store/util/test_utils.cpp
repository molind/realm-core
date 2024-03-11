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

#include <util/test_utils.hpp>

#include <realm/string_data.hpp>
#include <realm/object-store/impl/realm_coordinator.hpp>
#include <realm/util/base64.hpp>
#include <realm/util/demangle.hpp>
#include <realm/util/file.hpp>

#include <external/json/json.hpp>

#include <iostream>
#include <random>
#include <sys/stat.h>

#ifndef _WIN32
#include <unistd.h>
#include <sys/types.h>
#endif

#if REALM_PLATFORM_APPLE
#include <sys/mount.h>
#include <sys/param.h>
#endif

namespace realm {

bool ExceptionMatcher<void>::match(Exception const& ex) const
{
    return ex.code() == m_code && ex.what() == m_message;
}

std::string ExceptionMatcher<void>::describe() const
{
    return util::format("Exception(%1, \"%2\")", ErrorCodes::error_string(m_code), m_message);
}

bool OutOfBoundsMatcher::match(OutOfBounds const& ex) const
{
    return ex.code() == ErrorCodes::OutOfBounds && ex.index == m_index && ex.size == m_size && ex.what() == m_message;
}

std::string OutOfBoundsMatcher::describe() const
{
    return util::format("OutOfBounds(index=%1, size=%2, \"%3\")", m_index, m_size, m_message);
}

bool LogicErrorMatcher::match(LogicError const& ex) const
{
    return ex.code() == m_code;
}

std::string LogicErrorMatcher::describe() const
{
    return util::format("LogicError(%1)", ErrorCodes::error_string(m_code));
}

std::ostream& operator<<(std::ostream& os, const Exception& e)
{
    os << util::get_type_name(e) << "(" << e.code_string() << ", \"" << e.what() << "\")";
    return os;
}

bool create_dummy_realm(std::string path, std::shared_ptr<Realm>* out)
{
    Realm::Config config;
    config.path = path;
    try {
        auto realm = _impl::RealmCoordinator::get_coordinator(path)->get_realm(config, none);
        REQUIRE_REALM_EXISTS(path);
        if (out) {
            *out = std::move(realm);
        }
        return true;
    }
    catch (std::exception&) {
        return false;
    }
}

std::vector<char> make_test_encryption_key(const char start)
{
    std::vector<char> vector;
    vector.reserve(64);
    for (int i = 0; i < 64; i++) {
        vector.emplace_back((start + i) % 128);
    }
    return vector;
}

// FIXME: Catch2 limitation on old compilers (currently our android CI)
// https://github.com/catchorg/Catch2/blob/master/docs/limitations.md#clangg----skipping-leaf-sections-after-an-exception
void catch2_ensure_section_run_workaround(bool did_run_a_section, std::string section_name,
                                          util::FunctionRef<void()> func)
{
    if (did_run_a_section) {
        func();
    }
    else {
        std::cout << "Skipping test section '" << section_name << "' on this run." << std::endl;
    }
}

std::string encode_fake_jwt(const std::string& in, util::Optional<int64_t> exp, util::Optional<int64_t> iat)
{
    // by default make a valid expiry time so that the sync session pre check
    // doesn't trigger a token refresh on first open
    using namespace std::chrono_literals;
    if (!exp) {
        std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
        exp = std::chrono::system_clock::to_time_t(now + 60min);
    }
    if (!iat) {
        std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
        iat = std::chrono::system_clock::to_time_t(now - 1s);
    }

    std::string unencoded_prefix = nlohmann::json({"alg", "HS256"}).dump();
    std::string unencoded_body =
        nlohmann::json(
            {{"user_data", {{"token", in}}}, {"exp", *exp}, {"iat", *iat}, {"access", {"download", "upload"}}})
            .dump();

    std::string encoded_prefix, encoded_body;
    encoded_prefix.resize(util::base64_encoded_size(unencoded_prefix.size()));
    encoded_body.resize(util::base64_encoded_size(unencoded_body.size()));
    util::base64_encode(unencoded_prefix, encoded_prefix);
    util::base64_encode(unencoded_body, encoded_body);
    std::string suffix = "Et9HFtf9R3GEMA0IICOfFMVXY7kkTX1wr4qCyhIf58U";
    return encoded_prefix + "." + encoded_body + "." + suffix;
}

std::string random_string(std::string::size_type length)
{
    static auto& chrs = "abcdefghijklmnopqrstuvwxyz"
                        "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    thread_local static std::mt19937 rg{std::random_device{}()};
    thread_local static std::uniform_int_distribution<std::string::size_type> pick(0, sizeof(chrs) - 2);
    std::string s;
    s.reserve(length);
    while (length--)
        s += chrs[pick(rg)];
    return s;
}

int64_t random_int()
{
    thread_local std::mt19937_64 rng(std::random_device{}());
    return rng();
}

static bool file_is_on_exfat(const std::string& path)
{
#if REALM_PLATFORM_APPLE
    if (path.empty())
        return false;

    struct statfs fsbuf;
    int ret = statfs(path.c_str(), &fsbuf);
    REALM_ASSERT_RELEASE(ret == 0);
    // The documentation and headers helpfully don't list any of the values of
    // f_type or provide constants for them
    return fsbuf.f_type == 28 /* exFAT */;
#else
    static_cast<void>(path);
    return false;
#endif
}

bool chmod_supported(const std::string& path)
{
#ifndef _WIN32
    if (getuid() == 0) {
        return false; // running as root
    }
    if (file_is_on_exfat(path)) {
        return false;
    }
    return true;
#else
    static_cast<void>(path);
    return false;
#endif
}

int get_permissions(const std::string& path)
{
    int perms = 0;
#ifndef _WIN32
    REALM_ASSERT(!path.empty());
    struct stat statbuf;
    int ret = ::stat(path.c_str(), &statbuf);
    REALM_ASSERT_EX(ret == 0, ret, errno);
    perms = statbuf.st_mode;
#else
    static_cast<void>(path);
#endif
    return perms;
}

void chmod(const std::string& path, int permissions)
{
#ifndef _WIN32
    int ret = ::chmod(path.c_str(), permissions);
    REALM_ASSERT_EX(ret == 0, ret, errno);
#else
    static_cast<void>(path);
    static_cast<void>(permissions);
#endif
}

} // namespace realm
