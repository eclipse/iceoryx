// Copyright (c) 2019 by Robert Bosch GmbH. All rights reserved.
// Copyright (c) 2021 by Apex.AI Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0
#ifndef IOX_UTILS_POSIX_WRAPPER_ACCESS_CONTROL_HPP
#define IOX_UTILS_POSIX_WRAPPER_ACCESS_CONTROL_HPP

#include "iceoryx_utils/cxx/expected.hpp"
#include "iceoryx_utils/cxx/string.hpp"
#include "iceoryx_utils/cxx/vector.hpp"
#include "iceoryx_utils/platform/acl.hpp"

#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <type_traits>

namespace iox
{
namespace posix
{
/// @brief abstraction class for the management of access control lists (ACLs).
///
/// ACLs allow to define fine-grained access rights for files. In addition to the standard access rights, which can only
/// distinguish between user/group/others, ACLs can be used to give specific access rights to named users and groups.
/// ACL is part of the posix specification. The AccessController class is used to store ACL permission entries and
/// provides a way to write those entries to a file. A permission entry can be seen as a combination of an access
/// Category, a Permission and an optional name (used to identify specific users and groups.)
class AccessController
{
  public:
    using string_t = cxx::string<100>;

    enum class AccessControllerError : uint8_t
    {
        INVALID_STATE,
        COULD_NOT_ALLOCATE_NEW_ACL,
    };

    /// @brief maximum number of permission entries the AccessController can store
    static constexpr int32_t MaxNumOfPermissions = 20;

/// @brief identifier for a permission entry (user, group, others, ...)
#if defined(QNX) || defined(QNX__) || defined(__QNX__)
    enum class Category : std::underlying_type<acl_tag_t>::type
#else
    enum class Category : acl_tag_t
#endif
    {
        USER = ACL_USER_OBJ,
        /// a specific user must be identified by a name
        SPECIFIC_USER = ACL_USER,
        GROUP = ACL_GROUP_OBJ,
        /// a specific group must be identified by a name
        SPECIFIC_GROUP = ACL_GROUP,
        OTHERS = ACL_OTHER,
    };

/// @brief access right for a permission entry
#if defined(QNX) || defined(QNX__) || defined(__QNX__)
    enum class Permission : std::underlying_type<acl_perm_t>::type
#else
    enum class Permission : acl_perm_t
#endif
    {
        READ = ACL_READ,
        WRITE = ACL_WRITE,
        READWRITE = Permission::READ | Permission::WRITE,
        NONE = 0
    };

    /// @brief define and store a specific permission entry to be used by writePermissionsToFile.
    /// @param[f_id] id of the user or group. For Category::SPECIFIC_USER or Category::SPECIFIC_GROUP the id is
    /// required. Otherwise writing the permission entry to a file will fail. For the default user/group/others
    /// categories the id is ignored and can therefore be left empty. Do not forget to add permissions of the standard
    /// user/group/others categories before writing to a file.
    /// @param[f_permission] Permissions which should be applied to the category.
    /// @param[f_id] The group or user id - depending on the category. For Category::USER, Category::GROUP and
    ///   Category::OTHER the f_id is not required for everything else a valid group or user id is mandatory.
    bool addPermissionEntry(const Category f_category, const Permission f_permission, const uint32_t f_id = -1u);

    /// @brief just like addPermissionEntry(Category, Permission, int) but using a name instead of an id.
    bool addPermissionEntry(const Category f_category, const Permission f_permission, const string_t& f_name);

    /// @brief Write permission entries stored by the AccessController to a file identified by a file descriptor.
    /// @param[f_fileDescriptor] identifier for a file (can be regular file, shared memory file, message queue file...
    /// everything is a file).
    /// @return true if succesful. If false, you can assume that the file has not been touched at all.
    bool writePermissionsToFile(const int32_t f_fileDescriptor) const;

  private:
    using smartAclPointer_t = std::unique_ptr<std::remove_pointer<acl_t>::type, std::function<void(acl_t)>>;

    struct PermissionEntry
    {
        unsigned int m_category;
        Permission m_permission;
        unsigned int m_id;
    };

    cxx::vector<PermissionEntry, MaxNumOfPermissions> m_permissions;

    cxx::expected<smartAclPointer_t, AccessControllerError> createACL(const int32_t f_numEntries) const;
    bool createACLEntry(const acl_t f_ACL, const PermissionEntry& f_entry) const;
    bool addAclPermission(acl_permset_t f_permset, acl_perm_t f_perm) const;

    bool m_useACLMask{false};
};
} // namespace posix
} // namespace iox

#endif // IOX_UTILS_POSIX_WRAPPER_ACCESS_CONTROL_HPP
