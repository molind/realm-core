// swift-tools-version:5.5

import PackageDescription
import Foundation

let versionStr = "14.10.3"
let versionPieces = versionStr.split(separator: "-")
let versionCompontents = versionPieces[0].split(separator: ".")
let versionExtra = versionPieces.count > 1 ? versionPieces[1] : ""

var cxxSettings: [CXXSetting] = [
    .headerSearchPath("."),
    .define("REALM_DEBUG", .when(configuration: .debug)),
    .define("REALM_NO_CONFIG"),
    .define("REALM_INSTALL_LIBEXECDIR", to: ""),
    .define("REALM_ENABLE_ASSERTIONS", to: "1"),
    .define("REALM_ENABLE_ENCRYPTION", to: "1"),
    .define("REALM_ENABLE_SYNC", to: "1"),
    .define("REALM_ENABLE_GEOSPATIAL", to: "1"),
    .define("REALM_APP_SERVICES", to: "1"),

    .define("REALM_VERSION_MAJOR", to: String(versionCompontents[0])),
    .define("REALM_VERSION_MINOR", to: String(versionCompontents[1])),
    .define("REALM_VERSION_PATCH", to: String(versionCompontents[2])),
    .define("REALM_VERSION_EXTRA", to: "\"\(versionExtra)\""),
    .define("REALM_VERSION_STRING", to: "\"\(versionStr)\""),
    .define("REALM_HAVE_SECURE_TRANSPORT", to: "1", .when(platforms: [.macOS, .macCatalyst, .iOS, .tvOS, .watchOS]))
]

let syncServerSources: [String] =  [
    "realm/sync/noinst/server",
]

let syncExcludes: [String] = [
    // Server files
    "realm/sync/noinst/server/crypto_server_openssl.cpp",
    "realm/sync/noinst/server/crypto_server_stub.cpp",

    // CLI Tools
    "realm/sync/tools",
]

let notSyncServerSources: [String] = [
    "realm/alloc.cpp",
    "realm/alloc_slab.cpp",
    "realm/array.cpp",
    "realm/array_with_find.cpp",
    "realm/array_backlink.cpp",
    "realm/array_binary.cpp",
    "realm/array_blob.cpp",
    "realm/array_blobs_big.cpp",
    "realm/array_blobs_small.cpp",
    "realm/array_decimal128.cpp",
    "realm/array_fixed_bytes.cpp",
    "realm/array_integer.cpp",
    "realm/array_key.cpp",
    "realm/array_mixed.cpp",
    "realm/array_string.cpp",
    "realm/array_string_short.cpp",
    "realm/array_timestamp.cpp",
    "realm/array_unsigned.cpp",
    "realm/backup_restore.cpp",
    "realm/bplustree.cpp",
    "realm/chunked_binary.cpp",
    "realm/cluster.cpp",
    "realm/cluster_tree.cpp",
    "realm/collection.cpp",
    "realm/collection_parent.cpp",
    "realm/column_binary.cpp",
    "realm/db.cpp",
    "realm/decimal128.cpp",
    "realm/dictionary.cpp",
    "realm/disable_sync_to_disk.cpp",
    "realm/error_codes.cpp",
    "realm/exceptions.cpp",
    "realm/geospatial.cpp",
    "realm/global_key.cpp",
    "realm/group.cpp",
    "realm/group_writer.cpp",
    "realm/history.cpp",
    "realm/impl",
    "realm/index_string.cpp",
    "realm/link_translator.cpp",
    "realm/list.cpp",
    "realm/mixed.cpp",
    "realm/node.cpp",
    "realm/obj.cpp",
    "realm/obj_list.cpp",
    "realm/object_converter.cpp",
    "realm/object_id.cpp",
    "realm/query.cpp",
    "realm/query_engine.cpp",
    "realm/query_expression.cpp",
    "realm/query_value.cpp",
    "realm/replication.cpp",
    "realm/set.cpp",
    "realm/sort_descriptor.cpp",
    "realm/spec.cpp",
    "realm/status.cpp",
    "realm/string_data.cpp",
    "realm/sync/changeset.cpp",
    "realm/sync/changeset_encoder.cpp",
    "realm/sync/changeset_parser.cpp",
    "realm/sync/client.cpp",
    "realm/sync/config.cpp",
    "realm/sync/history.cpp",
    "realm/sync/instruction_applier.cpp",
    "realm/sync/instruction_replication.cpp",
    "realm/sync/instructions.cpp",
    "realm/sync/network/default_socket.cpp",
    "realm/sync/network/http.cpp",
    "realm/sync/network/network.cpp",
    "realm/sync/network/network_ssl.cpp",
    "realm/sync/network/websocket.cpp",
    "realm/sync/noinst/changeset_index.cpp",
    "realm/sync/noinst/client_history_impl.cpp",
    "realm/sync/noinst/client_impl_base.cpp",
    "realm/sync/noinst/client_reset.cpp",
    "realm/sync/noinst/client_reset_operation.cpp",
    "realm/sync/noinst/client_reset_recovery.cpp",
    "realm/sync/noinst/compact_changesets.cpp",
    "realm/sync/noinst/migration_store.cpp",
    "realm/sync/noinst/pending_bootstrap_store.cpp",
    "realm/sync/noinst/pending_reset_store.cpp",
    "realm/sync/noinst/protocol_codec.cpp",
    "realm/sync/noinst/sync_metadata_schema.cpp",
    "realm/sync/noinst/sync_schema_migration.cpp",
    "realm/sync/object_id.cpp",
    "realm/sync/protocol.cpp",
    "realm/sync/subscriptions.cpp",
    "realm/sync/transform.cpp",
    "realm/table.cpp",
    "realm/table_ref.cpp",
    "realm/table_view.cpp",
    "realm/tokenizer.cpp",
    "realm/to_json.cpp",
    "realm/transaction.cpp",
    "realm/unicode.cpp",
    "realm/util",
    "realm/utilities.cpp",
    "realm/uuid.cpp",
    "realm/version.cpp",
] + syncExcludes

let bidExcludes: [String] = [
    "bid128_acos.c",
    "bid128_acosh.c",
    "bid128_asin.c",
    "bid128_asinh.c",
    "bid128_atan.c",
    "bid128_atan2.c",
    "bid128_atanh.c",
    "bid128_cbrt.c",
    "bid128_cos.c",
    "bid128_cosh.c",
    "bid128_erf.c",
    "bid128_erfc.c",
    "bid128_exp.c",
    "bid128_exp10.c",
    "bid128_exp2.c",
    "bid128_expm1.c",
    "bid128_fdimd.c",
    "bid128_fmod.c",
    "bid128_frexp.c",
    "bid128_hypot.c",
    "bid128_ldexp.c",
    "bid128_lgamma.c",
    "bid128_llquantexpd.c",
    "bid128_llrintd.c",
    "bid128_llround.c",
    "bid128_log.c",
    "bid128_log10.c",
    "bid128_log1p.c",
    "bid128_log2.c",
    "bid128_logb.c",
    "bid128_logbd.c",
    "bid128_lrintd.c",
    "bid128_lround.c",
    "bid128_minmax.c",
    "bid128_modf.c",
    "bid128_nearbyintd.c",
    "bid128_next.c",
    "bid128_nexttowardd.c",
    "bid128_noncomp.c",
    "bid128_pow.c",
    "bid128_quantexpd.c",
    "bid128_quantumd.c",
    "bid128_rem.c",
    "bid128_round_integral.c",
    "bid128_scalb.c",
    "bid128_scalbl.c",
    "bid128_sin.c",
    "bid128_sinh.c",
    "bid128_sqrt.c",
    "bid128_tan.c",
    "bid128_tanh.c",
    "bid128_tgamma.c",
    "bid128_to_int16.c",
    "bid128_to_int32.c",
    "bid128_to_int8.c",
    "bid128_to_uint16.c",
    "bid128_to_uint32.c",
    "bid128_to_uint64.c",
    "bid128_to_uint8.c",
    "bid32_acos.c",
    "bid32_acosh.c",
    "bid32_add.c",
    "bid32_asin.c",
    "bid32_asinh.c",
    "bid32_atan.c",
    "bid32_atan2.c",
    "bid32_atanh.c",
    "bid32_cbrt.c",
    "bid32_compare.c",
    "bid32_cos.c",
    "bid32_cosh.c",
    "bid32_div.c",
    "bid32_erf.c",
    "bid32_erfc.c",
    "bid32_exp.c",
    "bid32_exp10.c",
    "bid32_exp2.c",
    "bid32_expm1.c",
    "bid32_fdimd.c",
    "bid32_fma.c",
    "bid32_fmod.c",
    "bid32_frexp.c",
    "bid32_hypot.c",
    "bid32_ldexp.c",
    "bid32_lgamma.c",
    "bid32_llquantexpd.c",
    "bid32_llrintd.c",
    "bid32_llround.c",
    "bid32_log.c",
    "bid32_log10.c",
    "bid32_log1p.c",
    "bid32_log2.c",
    "bid32_logb.c",
    "bid32_logbd.c",
    "bid32_lrintd.c",
    "bid32_lround.c",
    "bid32_minmax.c",
    "bid32_modf.c",
    "bid32_mul.c",
    "bid32_nearbyintd.c",
    "bid32_next.c",
    "bid32_nexttowardd.c",
    "bid32_noncomp.c",
    "bid32_pow.c",
    "bid32_quantexpd.c",
    "bid32_quantize.c",
    "bid32_quantumd.c",
    "bid32_rem.c",
    "bid32_round_integral.c",
    "bid32_scalb.c",
    "bid32_scalbl.c",
    "bid32_sin.c",
    "bid32_sinh.c",
    "bid32_sqrt.c",
    "bid32_string.c",
    "bid32_sub.c",
    "bid32_tan.c",
    "bid32_tanh.c",
    "bid32_tgamma.c",
    "bid32_to_bid64.c",
    "bid32_to_int16.c",
    "bid32_to_int32.c",
    "bid32_to_int64.c",
    "bid32_to_int8.c",
    "bid32_to_uint16.c",
    "bid32_to_uint32.c",
    "bid32_to_uint64.c",
    "bid32_to_uint8.c",
    "bid64_acos.c",
    "bid64_acosh.c",
    "bid64_add.c",
    "bid64_asin.c",
    "bid64_asinh.c",
    "bid64_atan.c",
    "bid64_atan2.c",
    "bid64_atanh.c",
    "bid64_cbrt.c",
    "bid64_compare.c",
    "bid64_cos.c",
    "bid64_cosh.c",
    "bid64_div.c",
    "bid64_erf.c",
    "bid64_erfc.c",
    "bid64_exp.c",
    "bid64_exp10.c",
    "bid64_exp2.c",
    "bid64_expm1.c",
    "bid64_fdimd.c",
    "bid64_fma.c",
    "bid64_fmod.c",
    "bid64_frexp.c",
    "bid64_hypot.c",
    "bid64_ldexp.c",
    "bid64_lgamma.c",
    "bid64_llquantexpd.c",
    "bid64_llrintd.c",
    "bid64_llround.c",
    "bid64_log.c",
    "bid64_log10.c",
    "bid64_log1p.c",
    "bid64_log2.c",
    "bid64_logb.c",
    "bid64_logbd.c",
    "bid64_lrintd.c",
    "bid64_lround.c",
    "bid64_minmax.c",
    "bid64_modf.c",
    "bid64_mul.c",
    "bid64_nearbyintd.c",
    "bid64_next.c",
    "bid64_nexttowardd.c",
    "bid64_noncomp.c",
    "bid64_pow.c",
    "bid64_quantexpd.c",
    "bid64_quantize.c",
    "bid64_quantumd.c",
    "bid64_rem.c",
    "bid64_round_integral.c",
    "bid64_scalb.c",
    "bid64_scalbl.c",
    "bid64_sin.c",
    "bid64_sinh.c",
    "bid64_sqrt.c",
    "bid64_string.c",
    "bid64_tan.c",
    "bid64_tanh.c",
    "bid64_tgamma.c",
    "bid64_to_int16.c",
    "bid64_to_int32.c",
    "bid64_to_int64.c",
    "bid64_to_int8.c",
    "bid64_to_uint16.c",
    "bid64_to_uint32.c",
    "bid64_to_uint64.c",
    "bid64_to_uint8.c",
    "bid_dpd.c",
    "bid_feclearexcept.c",
    "bid_fegetexceptflag.c",
    "bid_feraiseexcept.c",
    "bid_fesetexceptflag.c",
    "bid_fetestexcept.c",
    "bid_flag_operations.c",
    "strtod128.c",
    "strtod32.c",
    "strtod64.c",
    "wcstod128.c",
    "wcstod32.c",
    "wcstod64.c",
]

let platforms: [SupportedPlatform] = [
    .macOS(.v10_13),
    .iOS(.v12),
    .tvOS(.v12),
    .watchOS(.v4)
]

let package = Package(
    name: "RealmDatabase",
    platforms: platforms,
    products: [
        .library(
            name: "RealmCore",
            targets: ["RealmCoreResources"]),
        .library(
            name: "RealmQueryParser",
            targets: ["RealmQueryParser"]),
        .library(
            name: "RealmCapi",
            targets: ["Capi"]),
        .library(
            name: "RealmFFI",
            targets: ["RealmFFI"]),
    ],
    targets: [
        .target(
            name: "Bid",
            path: "src/external/IntelRDFPMathLib20U2/LIBRARY/src",
            exclude: bidExcludes,
            publicHeadersPath: "."
        ),
        .target(
            name: "s2geometry",
            path: "src/external/s2",
            exclude: [
                "s2cellunion.cc",
                "s2regioncoverer.cc",
                "s2regionintersection.cc",
                "s2regionunion.cc"
            ],
            publicHeadersPath: ".",
            cxxSettings: ([
                .headerSearchPath(".."),
                .headerSearchPath("../.."),
            ] + cxxSettings) as [CXXSetting]),
        .target(
            name: "RealmCore",
            dependencies: ["Bid", "s2geometry"],
            path: "src",
            exclude: ([
                "CMakeLists.txt",
                "external",
                "realm/CMakeLists.txt",
                "realm/exec",
                "realm/object-store/CMakeLists.txt",
                "realm/object-store/c_api",
                "realm/object-store/impl/epoll",
                "realm/object-store/impl/generic",
                "realm/object-store/impl/windows",
                "realm/object-store/impl/emscripten",
                "realm/object-store/sync/impl/emscripten",
                "realm/parser",
                "realm/sync/CMakeLists.txt",
                "realm/tools",
                "realm/util/config.h.in",
                "realm/version_numbers.hpp.in",
                "spm",
                "swift",
                "win32",
            ] + syncExcludes + syncServerSources) as [String],
            publicHeadersPath: ".",
            cxxSettings: ([
                .headerSearchPath("external"),
            ] + cxxSettings) as [CXXSetting],
            linkerSettings: [
                .linkedLibrary("compression"),
                .linkedLibrary("z"),
                .linkedFramework("Foundation", .when(platforms: [.macOS, .iOS, .tvOS, .watchOS, .macCatalyst])),
                .linkedFramework("Security", .when(platforms: [.macOS, .iOS, .tvOS, .watchOS, .macCatalyst])),
            ]),
        // Adding resources to a target makes command line swift build (but not
        // xcodebuild) force-include Foundation.h without properly excluding
        // non-objc c++ files. Adding the resources in a separate target which
        // depends on RealmCore works around this. This target needs a single
        // source file (dummy.mm) to work around a different bug in xcodebuild
        // (but not swift build) that makes empty targets not work.
        .target(
            name: "RealmCoreResources",
            dependencies: ["RealmCore"],
            path: "src/spm",
            resources: [
                .copy("PrivacyInfo.xcprivacy")
            ],
            publicHeadersPath: "."),
        .target(
            name: "RealmQueryParser",
            dependencies: ["RealmCore"],
            path: "src/realm/parser",
            exclude: [
                "CMakeLists.txt",
                "query_bison.yy",
                "query_flex.ll",
            ],
            publicHeadersPath: ".",
            cxxSettings: [
                .headerSearchPath("realm/parser/generated")
            ] + cxxSettings),
        .target(
            name: "SyncServer",
            dependencies: ["RealmCore"],
            path: "src",
            exclude: ([
                "CMakeLists.txt",
                "external",
                "realm/CMakeLists.txt",
                "realm/exec",
                "realm/object-store",
                "realm/parser",
                "realm/sync/CMakeLists.txt",
                "realm/sync/noinst/server/CMakeLists.txt",
                "realm/tools",
                "realm/util/config.h.in",
                "realm/version_numbers.hpp.in",
                "swift",
                "win32",
            ] + notSyncServerSources) as [String],
            sources: ["realm/sync"],
            publicHeadersPath: "realm/sync/impl", // hack
            cxxSettings: cxxSettings),
        .target(
            name: "Capi",
            dependencies: ["RealmCore", "RealmQueryParser"],
            path: "src/realm/object-store/c_api",
            exclude: [
                "CMakeLists.txt",
                "realm.c",
            ],
            publicHeadersPath: ".",
            cxxSettings: (cxxSettings) as [CXXSetting]),
        .target(
            name: "RealmFFI",
            dependencies: ["Capi"],
            path: "src/swift",
            cxxSettings: (cxxSettings) as [CXXSetting]),
        .target(
            name: "Catch2Generated",
            path: "test/external/generated",
            // this file was manually generated with catch v3.0.1
            // and should be regenerated when catch is upgraded
            resources: [.copy("catch2/catch_user_config.hpp")],
            publicHeadersPath: "."),
        .target(
            name: "Catch2",
            dependencies: ["Catch2Generated"],
            path: "test/external/catch/src",
            exclude: [
                "CMakeLists.txt",
                "catch2/catch_user_config.hpp.in",
                "catch2/internal/catch_main.cpp"
                ],
            publicHeadersPath: ".",
            cxxSettings: ([
                .headerSearchPath("catch2"),
                .define("CATCH_CONFIG_NO_CPP17_UNCAUGHT_EXCEPTIONS")
            ] + cxxSettings) as [CXXSetting]),
        .target(
            name: "CoreTestUtils",
            dependencies: ["RealmCore"],
            path: "test/util",
            exclude: [
                "CMakeLists.txt"
            ],
            publicHeadersPath: ".",
            cxxSettings: (cxxSettings) as [CXXSetting]),
        .target(
            name: "ObjectStoreTestUtils",
            dependencies: ["RealmCore", "SyncServer", "Catch2", "CoreTestUtils"],
            path: "test/object-store/util",
            publicHeadersPath: ".",
            cxxSettings: ([
                .headerSearchPath(".."),
                .define("_LIBCPP_DISABLE_AVAILABILITY")
            ] + cxxSettings) as [CXXSetting]),
        .executableTarget(
            name: "ObjectStoreTests",
            dependencies: ["RealmQueryParser", "ObjectStoreTestUtils"],
            path: "test/object-store",
            exclude: [
                "CMakeLists.txt",
                "backup.cpp",
                "benchmarks",
                "c_api",
                "geospatial.cpp",
                "query.json",
                "sync-metadata-v4.realm",
                "sync-metadata-v5.realm",
                "test_backup-olden-and-golden.realm",
                "util",
            ],
            cxxSettings: ([
                .headerSearchPath("."),
                .headerSearchPath(".."),
                .define("_LIBCPP_DISABLE_AVAILABILITY")
            ] + cxxSettings) as [CXXSetting]),
        .executableTarget(
            name: "CapiTests",
            dependencies: ["Capi", "ObjectStoreTestUtils"],
            path: "test/object-store/c_api",
            cxxSettings: ([
                .headerSearchPath("../"),
            ] + cxxSettings) as [CXXSetting]),
    ],
    cxxLanguageStandard: .cxx20
)
