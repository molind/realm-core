SRC = \
	src/realm/alloc.cpp \
	src/realm/alloc_slab.cpp \
	src/realm/array.cpp \
	src/realm/array_backlink.cpp \
	src/realm/array_binary.cpp \
	src/realm/array_blob.cpp \
	src/realm/array_blobs_big.cpp \
	src/realm/array_blobs_small.cpp \
	src/realm/array_decimal128.cpp \
	src/realm/array_integer.cpp \
	src/realm/array_key.cpp \
	src/realm/array_mixed.cpp \
	src/realm/array_object_id.cpp \
	src/realm/array_string.cpp \
	src/realm/array_string_short.cpp \
	src/realm/array_timestamp.cpp \
	src/realm/array_unsigned.cpp \
	src/realm/bplustree.cpp \
	src/realm/chunked_binary.cpp \
	src/realm/cluster.cpp \
	src/realm/column_binary.cpp \
	src/realm/db.cpp \
	src/realm/decimal128.cpp \
	src/realm/disable_sync_to_disk.cpp \
	src/realm/exceptions.cpp \
	src/realm/global_key.cpp \
	src/realm/group.cpp \
	src/realm/group_writer.cpp \
	src/realm/index_string.cpp \
	src/realm/list.cpp \
	src/realm/mixed.cpp \
	src/realm/node.cpp \
	src/realm/obj.cpp \
	src/realm/obj_list.cpp \
	src/realm/object_id.cpp \
	src/realm/query.cpp \
	src/realm/query_engine.cpp \
	src/realm/replication.cpp \
	src/realm/sort_descriptor.cpp \
	src/realm/spec.cpp \
	src/realm/string_data.cpp \
	src/realm/table.cpp \
	src/realm/table_ref.cpp \
	src/realm/table_view.cpp \
	src/realm/unicode.cpp \
	src/realm/utilities.cpp \
	src/realm/impl/output_stream.cpp \
	src/realm/impl/simulated_failure.cpp \
	src/realm/impl/transact_log.cpp \
	src/realm/util/allocator.cpp \
	src/realm/util/allocation_metrics.cpp \
	src/realm/util/backtrace.cpp \
	src/realm/util/base64.cpp \
	src/realm/util/basic_system_errors.cpp \
	src/realm/util/encrypted_file_mapping.cpp \
	src/realm/util/fifo_helper.cpp \
	src/realm/util/file.cpp \
	src/realm/util/file_mapper.cpp \
	src/realm/util/interprocess_condvar.cpp \
	src/realm/util/interprocess_mutex.cpp \
	src/realm/util/logger.cpp \
	src/realm/util/misc_errors.cpp \
	src/realm/util/serializer.cpp \
	src/realm/util/sha_crypto.cpp \
	src/realm/util/terminate.cpp \
	src/realm/util/thread.cpp \
	src/realm/util/to_string.cpp \
	src/realm/sync/changeset.cpp \
	src/realm/sync/changeset_encoder.cpp \
	src/realm/sync/changeset_parser.cpp \
	src/realm/sync/history.cpp \
	src/realm/sync/instructions.cpp \
	src/realm/sync/instruction_applier.cpp \
	src/realm/sync/instruction_replication.cpp \
	src/realm/sync/object.cpp \
	src/realm/sync/object_id.cpp \
	src/realm/sync/transform.cpp \
	src/realm/sync/noinst/changeset_index.cpp \
	src/realm/sync/noinst/client_history_impl.cpp \
	src/realm/sync/noinst/object_id_history_state.cpp \
	sync_migrator.cpp

EXTERNAL_SRC = \
	src/external/IntelRDFPMathLib20U2/LIBRARY/src/bid128.c \
	src/external/IntelRDFPMathLib20U2/LIBRARY/src/bid128_compare.c \
	src/external/IntelRDFPMathLib20U2/LIBRARY/src/bid128_div.c \
	src/external/IntelRDFPMathLib20U2/LIBRARY/src/bid128_add.c \
	src/external/IntelRDFPMathLib20U2/LIBRARY/src/bid128_fma.c \
	src/external/IntelRDFPMathLib20U2/LIBRARY/src/bid64_to_bid128.c \
	src/external/IntelRDFPMathLib20U2/LIBRARY/src/bid_convert_data.c \
	src/external/IntelRDFPMathLib20U2/LIBRARY/src/bid_decimal_data.c \
	src/external/IntelRDFPMathLib20U2/LIBRARY/src/bid_decimal_globals.c \
	src/external/IntelRDFPMathLib20U2/LIBRARY/src/bid_from_int.c \
	src/external/IntelRDFPMathLib20U2/LIBRARY/src/bid_round.c \

OBJ = $(SRC:.cpp=.o) $(EXTERNAL_SRC:.c=.o)
LIB = libsync_migrator.so
APP = sync_migrator
APP_SRC = sync_migrator_app.cpp
APP_OBJ = $(APP_SRC:.cpp=.o)

GENERATED_HEADERS = \
	genfiles/realm/util/config.h

ifeq ($(LIBS_PLATFORM),)
LIBS_PLATFORM := macos
endif

LDFLAGS_ios := $(LDFLAGS) -framework CoreFoundation 
LDFLAGS_catalyst := $(LDFLAGS) -framework CoreFoundation
LDFLAGS_macos := $(LDFLAGS) -framework CoreFoundation
LDFLAGS_android := $(LDFLAGS) -landroid -llog

LDFLAGS = ${LDFLAGS_${LIBS_PLATFORM}}

CXXFLAGS := $(CXXFLAGS) -std=c++17 -I./src -I./genfiles

.SUFFIXES: .cpp

all: $(APP)

FLAGS = -DREALM_VERSION_MAJOR=10 -DREALM_VERSION_MINOR=0 -DREALM_VERSION_PATCH=0 -DREALM_VERSION_STRING='"10.0.0-alpha"'

.cpp.o:
	$(CXX) $(FLAGS) $(CPPFLAGS) ${CXXFLAGS} -c $< -o $@

.c.o:
	$(CC) $(FLAGS) $(CPPFLAGS) ${CFLAGS} -c $< -o $@

$(OBJ): $(GENERATED_HEADERS)

$(LIB): $(OBJ)
	$(CXX) $(CXXFLAGS) -fvisibility=hidden -shared -o $(LIB) $(FLAGS) ${LDFLAGS} $(OBJ)

$(APP): $(APP_OBJ) $(LIB)
	$(CXX) $(CXXFLAGS) -o $(APP) $(APP_OBJ) $(LDFLAGS) -L. -lsync_migrator

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
	cp sync_migrator.hpp ${PREFIX}/include/realm

clean:
	rm -f $(OBJ) $(APP_OBJ) $(LIB) $(APP) $(GENERATED_HEADERS)