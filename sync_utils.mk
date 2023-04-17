SRC = \
	src/realm/alloc_slab.cpp \
	src/realm/alloc.cpp \
	src/realm/array_backlink.cpp \
	src/realm/array_binary.cpp \
	src/realm/array_blob.cpp \
	src/realm/array_blobs_big.cpp \
	src/realm/array_blobs_small.cpp \
	src/realm/array_decimal128.cpp \
	src/realm/array_fixed_bytes.cpp \
	src/realm/array_integer.cpp \
	src/realm/array_key.cpp \
	src/realm/array_mixed.cpp \
	src/realm/array_string_short.cpp \
	src/realm/array_string.cpp \
	src/realm/array_timestamp.cpp \
	src/realm/array_unsigned.cpp \
	src/realm/array_with_find.cpp \
	src/realm/array.cpp \
	src/realm/backup_restore.cpp \
	src/realm/bplustree.cpp \
	src/realm/chunked_binary.cpp \
	src/realm/cluster_tree.cpp \
	src/realm/cluster.cpp \
	src/realm/collection.cpp \
	src/realm/column_binary.cpp \
	src/realm/db.cpp \
	src/realm/decimal128.cpp \
	src/realm/dictionary.cpp \
	src/realm/disable_sync_to_disk.cpp \
	src/realm/error_codes.cpp \
	src/realm/exceptions.cpp \
	src/realm/global_key.cpp \
	src/realm/group_writer.cpp \
	src/realm/group.cpp \
	src/realm/history.cpp \
	src/realm/impl/copy_replication.cpp \
	src/realm/impl/output_stream.cpp \
	src/realm/impl/simulated_failure.cpp \
	src/realm/impl/transact_log.cpp \
	src/realm/index_string.cpp \
	src/realm/link_translator.cpp \
	src/realm/list.cpp \
	src/realm/mixed.cpp \
	src/realm/node.cpp \
	src/realm/obj_list.cpp \
	src/realm/obj.cpp \
	src/realm/object_converter.cpp \
	src/realm/object_id.cpp \
	src/realm/object-store/object_schema.cpp \
	src/realm/object-store/object_store.cpp \
	src/realm/object-store/schema.cpp \
	src/realm/query_engine.cpp \
	src/realm/query_expression.cpp \
	src/realm/query_value.cpp \
	src/realm/query.cpp \
	src/realm/replication.cpp \
	src/realm/set.cpp \
	src/realm/sort_descriptor.cpp \
	src/realm/spec.cpp \
	src/realm/status.cpp \
	src/realm/string_data.cpp \
	src/realm/sync/changeset_encoder.cpp \
	src/realm/sync/changeset_parser.cpp \
	src/realm/sync/changeset.cpp \
	src/realm/sync/history.cpp \
	src/realm/sync/instruction_applier.cpp \
	src/realm/sync/instruction_replication.cpp \
	src/realm/sync/instructions.cpp \
	src/realm/sync/noinst/changeset_index.cpp \
	src/realm/sync/noinst/client_history_impl.cpp \
	src/realm/sync/noinst/client_reset_operation.cpp \
	src/realm/sync/noinst/client_reset_recovery.cpp \
	src/realm/sync/noinst/client_reset.cpp \
	src/realm/sync/noinst/compact_changesets.cpp \
	src/realm/sync/noinst/migration_store.cpp \
	src/realm/sync/noinst/server/server_history.cpp \
	src/realm/sync/noinst/sync_metadata_schema.cpp \
	src/realm/sync/object_id.cpp \
	src/realm/sync/subscriptions.cpp \
	src/realm/sync/transform.cpp \
	src/realm/table_ref.cpp \
	src/realm/table_view.cpp \
	src/realm/table.cpp \
	src/realm/tokenizer.cpp \
	src/realm/transaction.cpp \
	src/realm/unicode.cpp \
	src/realm/util/backtrace.cpp \
	src/realm/util/base64.cpp \
	src/realm/util/basic_system_errors.cpp \
	src/realm/util/compression.cpp \
	src/realm/util/demangle.cpp \
	src/realm/util/encrypted_file_mapping.cpp \
	src/realm/util/fifo_helper.cpp \
	src/realm/util/file_mapper.cpp \
	src/realm/util/file.cpp \
	src/realm/util/interprocess_condvar.cpp \
	src/realm/util/interprocess_mutex.cpp \
	src/realm/util/logger.cpp \
	src/realm/util/misc_errors.cpp \
	src/realm/util/serializer.cpp \
	src/realm/util/sha_crypto.cpp \
	src/realm/util/terminate.cpp \
	src/realm/util/thread.cpp \
	src/realm/util/time.cpp \
	src/realm/util/to_string.cpp \
	src/realm/utilities.cpp \
	src/realm/uuid.cpp \
	sync_utils.cpp

EXTERNAL_SRC = \
	src/external/IntelRDFPMathLib20U2/LIBRARY/src/bid_convert_data.c \
	src/external/IntelRDFPMathLib20U2/LIBRARY/src/bid_decimal_data.c \
	src/external/IntelRDFPMathLib20U2/LIBRARY/src/bid_decimal_globals.c \
	src/external/IntelRDFPMathLib20U2/LIBRARY/src/bid_from_int.c \
	src/external/IntelRDFPMathLib20U2/LIBRARY/src/bid_round.c \
	src/external/IntelRDFPMathLib20U2/LIBRARY/src/bid128_2_str_tables.c \
	src/external/IntelRDFPMathLib20U2/LIBRARY/src/bid128_add.c \
	src/external/IntelRDFPMathLib20U2/LIBRARY/src/bid128_compare.c \
	src/external/IntelRDFPMathLib20U2/LIBRARY/src/bid128_div.c \
	src/external/IntelRDFPMathLib20U2/LIBRARY/src/bid128_fma.c \
	src/external/IntelRDFPMathLib20U2/LIBRARY/src/bid128_mul.c \
	src/external/IntelRDFPMathLib20U2/LIBRARY/src/bid128_quantize.c \
	src/external/IntelRDFPMathLib20U2/LIBRARY/src/bid128_string.c \
	src/external/IntelRDFPMathLib20U2/LIBRARY/src/bid128_to_int64.c \
	src/external/IntelRDFPMathLib20U2/LIBRARY/src/bid128.c \
	src/external/IntelRDFPMathLib20U2/LIBRARY/src/bid64_to_bid128.c \

OBJ = $(SRC:.cpp=.o) $(EXTERNAL_SRC:.c=.o)
LIB = libsync_utils.so
APP = sync_utils
APP_SRC = sync_utils_app.cpp
APP_OBJ = $(APP_SRC:.cpp=.o)

GENERATED_HEADERS = \
	genfiles/realm/util/config.h

ifeq ($(LIBS_PLATFORM),)
LIBS_PLATFORM := macos
endif

LDFLAGS_ios := $(LDFLAGS) -framework CoreFoundation
LDFLAGS_catalyst := $(LDFLAGS) -framework CoreFoundation
LDFLAGS_macos := $(LDFLAGS) -framework CoreFoundation -lz -lcompression
LDFLAGS_android := $(LDFLAGS) -landroid -llog -lcrypto

LDFLAGS = ${LDFLAGS_${LIBS_PLATFORM}}

CXXFLAGS := $(CXXFLAGS) -std=c++17 -I./src -I./genfiles

.SUFFIXES: .cpp

all: $(APP)

FLAGS = -DREALM_VERSION_MAJOR=13 -DREALM_VERSION_MINOR=8 -DREALM_VERSION_PATCH=0 -DREALM_VERSION_STRING='"13.8.0-alpha"' -DREALM_DEBUG -g -O0

.cpp.o:
	$(CXX) $(FLAGS) $(CPPFLAGS) ${CXXFLAGS} -c $< -o $@

.c.o:
	$(CC) $(FLAGS) $(CPPFLAGS) ${CFLAGS} -c $< -o $@

$(OBJ): $(GENERATED_HEADERS)

$(LIB): $(OBJ)
	$(CXX) $(CXXFLAGS) -fvisibility=hidden -shared -o $(LIB) $(FLAGS) ${LDFLAGS} $(OBJ)

$(APP): $(APP_OBJ) $(LIB)
	$(CXX) $(CXXFLAGS) -o $(APP) $(APP_OBJ) $(LDFLAGS) -L. -lsync_utils

genfiles/realm/util/config.h:
	mkdir -p genfiles/realm/util
	touch genfiles/realm/util/config.h

.PHONY: clean install all

ifndef PREFIX
PREFIX = /usr/local
endif

install: $(LIB)
	mkdir -p ${PREFIX}/lib ${PREFIX}/include/realm
	cp $(LIB) ${PREFIX}/lib
	cp sync_utils.hpp ${PREFIX}/include/realm

clean:
	rm -f $(OBJ) $(APP_OBJ) $(LIB) $(APP) $(GENERATED_HEADERS)
