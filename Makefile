.PHONY: dep deps/opentelemetry-cpp format

EXT_VERSION = 0.1.0
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)

PG_CPPFLAGS := -std=c++14 -Ideps/opentelemetry-cpp/exporters/ostream/include -Ideps/opentelemetry-cpp/sdk/include -Ideps/opentelemetry-cpp/api/include -Ideps/opentelemetry-cpp/exporters/otlp/include
PG_LDFLAGS := -std=c++14 -L./deps/lib \
   -lopentelemetry_common -lopentelemetry_proto \
	 -lopentelemetry_otlp_recordable -lopentelemetry_resources \
	 -lopentelemetry_http_client_curl -lopentelemetry_exporter_otlp_http \
	 -lopentelemetry_exporter_otlp_http_client -lopentelemetry_trace \
	 -lopentelemetry_exporter_ostream_span \
	 -lprotobuf -lcurl -lc++
SRCS = pg_opentelemetry.cpp instrumentation.cpp
OBJS = pg_opentelemetry.o  instrumentation.o
MODULE_big = pg_opentelemetry
EXTENSION = pg_opentelemetry
DATA = pg_opentelemetry.control

REGRESS = init-extension json jsonb types context

# Platform detection
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	PG_LDFLAGS += -L/opt/homebrew/lib
endif
ifeq ($(UNAME_S),Linux)
	PG_LDFLAGS += -L/usr/local/lib
endif

all: dep %--$(EXT_VERSION).sql

clean:
	rm -rf deps

include $(PGXS)

pg_opentelemetry.cpp: pg_opentelemetry.h
instrumentation.cpp: pg_opentelemetry.h

dep: deps/lib/libopentelemetry_trace.a

deps/opentelemetry-cpp:
	mkdir -p deps
	git submodule update --init --recursive

deps/lib/libopentelemetry_trace.a: deps/opentelemetry-cpp
	cd deps/opentelemetry-cpp && \
	git submodule update --init --recursive && \
	mkdir -p build && \
	cd build && \
	cmake -DBUILD_TESTING=OFF -DWITH_OTLP=ON -DWITH_OTLP_HTTP=ON -DWITH_OTLP_GRPC=OFF .. && \
	cmake --build . --target all
	mkdir -p deps/lib && \
	cp deps/opentelemetry-cpp/build/sdk/src/trace/libopentelemetry_trace.a deps/lib/ && \
	cp deps/opentelemetry-cpp/build/ext/src/http/client/curl/libopentelemetry_http_client_curl.a deps/lib/ && \
	cp deps/opentelemetry-cpp/build/exporters/ostream/libopentelemetry_exporter_ostream_span.a  deps/lib/ && \
	cp deps/opentelemetry-cpp/build/sdk/src/resource/libopentelemetry_resources.a deps/lib/ && \
	cp deps/opentelemetry-cpp/build/sdk/src/trace/libopentelemetry_trace.a deps/lib/ && \
	cp deps/opentelemetry-cpp/build/sdk/src/common/libopentelemetry_common.a deps/lib/ && \
	cp deps/opentelemetry-cpp/build/exporters/otlp/libopentelemetry_exporter_otlp_http.a deps/lib/ && \
	cp deps/opentelemetry-cpp/build/exporters/otlp/libopentelemetry_exporter_otlp_http_client.a deps/lib/ && \
	cp deps/opentelemetry-cpp/build/exporters/otlp/libopentelemetry_otlp_recordable.a deps/lib/ && \
	cp deps/opentelemetry-cpp/build/exporters/otlp/libopentelemetry_exporter_otlp_http.a deps/lib/ && \
	cp deps/opentelemetry-cpp/build/exporters/otlp/libopentelemetry_exporter_otlp_http_client.a deps/lib/ && \
	cp deps/opentelemetry-cpp/build/sdk/src/common/libopentelemetry_common.a deps/lib/ && \
	cp deps/opentelemetry-cpp/build/libopentelemetry_proto.a deps/lib

format:
	clang-format -i $(SRCS)

%--$(EXT_VERSION).sql: pg_opentelemetry.sql
	cp pg_opentelemetry.sql pg_opentelemetry--$(EXT_VERSION).sql
