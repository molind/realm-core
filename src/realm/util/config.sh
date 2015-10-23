# NOTE: THIS SCRIPT IS SUPPOSED TO RUN IN A POSIX COMPATIBLE SHELL

source="$1"
target="$2"

get_config_param()
{
    local name line value
    name="$1"
    if ! line="$(grep "^$name *=" "$source")"; then
        cat 1>&2 <<EOF
ERROR: Failed to read configuration parameter '$name'.
Maybe you need to rerun 'sh build.sh config [PREFIX]'.
EOF
        return 1
    fi
    value="$(printf "%s\n" "$line" | cut -d= -f2-)" || return 1
    value="$(printf "%s\n" "$value" | sed 's/^ *//')" || return 1
    printf "%s\n" "$value"
}

cstring_escape()
{
    local str
    str="$1"
    str="$(printf "%s\n" "$str" | sed "s/\\([\"\\]\\)/\\\\\\1/g")" || return 1
    printf "%s\n" "$str"

}

realm_version="$(get_config_param "REALM_VERSION")" || exit 1
realm_version_escaped="$(cstring_escape "$realm_version")" || exit 1

install_prefix="$(get_config_param "INSTALL_PREFIX")" || exit 1
install_prefix_escaped="$(cstring_escape "$install_prefix")" || exit 1

install_exec_prefix="$(get_config_param "INSTALL_EXEC_PREFIX")" || exit 1
install_exec_prefix_escaped="$(cstring_escape "$install_exec_prefix")" || exit 1

install_includedir="$(get_config_param "INSTALL_INCLUDEDIR")" || exit 1
install_includedir_escaped="$(cstring_escape "$install_includedir")" || exit 1

install_bindir="$(get_config_param "INSTALL_BINDIR")" || exit 1
install_bindir_escaped="$(cstring_escape "$install_bindir")" || exit 1

install_libdir="$(get_config_param "INSTALL_LIBDIR")" || exit 1
install_libdir_escaped="$(cstring_escape "$install_libdir")" || exit 1

install_libexecdir="$(get_config_param "INSTALL_LIBEXECDIR")" || exit 1
install_libexecdir_escaped="$(cstring_escape "$install_libexecdir")" || exit 1

max_bpnode_size="$(get_config_param "MAX_BPNODE_SIZE")" || exit 1
max_bpnode_size_debug="$(get_config_param "MAX_BPNODE_SIZE_DEBUG")" || exit 1

enable_alloc_set_zero="$(get_config_param "ENABLE_ALLOC_SET_ZERO")" || exit 1
if [ "$enable_alloc_set_zero" = "yes" ]; then
    enable_alloc_set_zero="1"
else
    enable_alloc_set_zero="0"
fi

enable_encryption="$(get_config_param "ENABLE_ENCRYPTION")" || exit 1
if [ "$enable_encryption" = "yes" ]; then
    enable_encryption="1"
else
    enable_encryption="0"
fi

enable_assertions="$(get_config_param "ENABLE_ASSERTIONS")" || exit 1
if [ "$enable_assertions" = "yes" ]; then
    enable_assertions="1"
else
    enable_assertions="0"
fi

cat >"$target" <<EOF
/*************************************************************************
 *
 * CAUTION:  DO NOT EDIT THIS FILE -- YOUR CHANGES WILL BE LOST!
 *
 * This file is generated by config.sh
 *
 *************************************************************************/

#define REALM_VERSION               "$realm_version_escaped"

#define REALM_INSTALL_PREFIX        "$install_prefix_escaped"
#define REALM_INSTALL_EXEC_PREFIX   "$install_exec_prefix_escaped"
#define REALM_INSTALL_INCLUDEDIR    "$install_includedir_escaped"
#define REALM_INSTALL_BINDIR        "$install_bindir_escaped"
#define REALM_INSTALL_LIBDIR        "$install_libdir_escaped"
#define REALM_INSTALL_LIBEXECDIR    "$install_libexecdir_escaped"

#ifdef REALM_DEBUG
#  define REALM_MAX_BPNODE_SIZE     $max_bpnode_size_debug
#else
#  define REALM_MAX_BPNODE_SIZE     $max_bpnode_size
#endif

#define REALM_ENABLE_ALLOC_SET_ZERO $enable_alloc_set_zero
#define REALM_ENABLE_ENCRYPTION     $enable_encryption
#define REALM_ENABLE_ASSERTIONS     $enable_assertions
EOF
