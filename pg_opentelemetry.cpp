extern "C" {
#include <postgres.h>

#include <executor/executor.h>
#include <fmgr.h>
#include <utils/elog.h>
#include <utils/guc.h>

static void executor_start_hook(QueryDesc *query_desc, int eflags);
static void executor_end_hook(QueryDesc *query_desc);
PGDLLEXPORT void _PG_init(void);
PGDLLEXPORT void _PG _fini(void);

PG_MODULE_MAGIC;
}

#include "opentelemetry/trace/provider.h"
#include <string>

#include "pg_opentelemetry.h"

namespace trace = opentelemetry::trace;

bool init = false;

// Previous hooks for storage.
static ExecutorStart_hook_type prev_executor_start_hook = NULL;
static ExecutorEnd_hook_type prev_executor_end_hook     = NULL;

void executor_start_hook(QueryDesc *query_desc, int eflags) {
  if (prev_executor_start_hook) {
    prev_executor_start_hook(query_desc, eflags);
  } else {
    standard_ExecutorStart(query_desc, eflags);
  }

  // only "do" something if the extension is enabled.
  if (!settings.enabled) {
    return;
  }

  // don't initialize the tracer until the first call for this backend.
  if (!init) {
    init_tracer( );
    init = true;
  }

  auto provider = trace::Provider::GetTracerProvider( );

  auto span = provider->GetTracer("pg_opentelemetry")
                  ->StartSpan("query", { { "query", query_desc->sourceText } });
}

void executor_end_hook(QueryDesc *query_desc) {
  if (prev_executor_end_hook) {
    prev_executor_end_hook(query_desc);
  } else {
    standard_ExecutorEnd(query_desc);
  }

  // only "do" something if the extension is enabled.
  if (!settings.enabled) {
    return;
  }

  auto provider = trace::Provider::GetTracerProvider( );

  auto span = provider->GetTracer("pg_opentelemetry")->GetCurrentSpan( );
  span->End( );
}

void _PG_init(void) {
  // set up the start hook.
  prev_executor_start_hook = ExecutorStart_hook;
  ExecutorStart_hook       = executor_start_hook;

  // set up the end hook.
  prev_executor_end_hook = ExecutorEnd_hook;
  ExecutorEnd_hook       = executor_end_hook;

  // set up the GUC variables.
  DefineCustomBoolVariable(
      "pg_opentelemetry.enabled",
      "Whether pg_opentelemetry trace gathering is enabled or not", nullptr,
      &settings.enabled, false, PGC_USERSET, 0, nullptr, nullptr, nullptr);

  DefineCustomIntVariable("pg_opentelemetry.max_queue_size",
                          "Maximum queue size before exporting traces", nullptr,
                          &settings.max_queue_size, 64, 1, 1024, PGC_USERSET, 0,
                          nullptr, nullptr, nullptr);

  DefineCustomIntVariable("pg_opentelemetry.max_export_batch_size",
                          "Maximum number of traces to export at once", nullptr,
                          &settings.max_export_batch_size, 32, 1, 128,
                          PGC_USERSET, 0, nullptr, nullptr, nullptr);

  struct config_enum_entry exporters_enum[ NUM_EXPORTERS ] = {
    { "log", EXPORTER_LOG, false }, { "http", EXPORTER_HTTP, false }
  };

  DefineCustomEnumVariable(
      "pg_opentelemetry.otlp_exporter", "Type of exporter to use (log, http)",
      nullptr, &settings.otlp_exporter, 0, exporters_enum, PGC_USERSET,
      EXPORTER_LOG, nullptr, nullptr, nullptr);

  DefineCustomStringVariable(
      "pg_opentelemetry.otlp_endpoint",
      "Endpoint to send the traces to (currently http only)", nullptr,
      &settings.otlp_endpoint, "http://127.0.0.1:4318/v1/traces", PGC_USERSET,
      0, nullptr, nullptr, nullptr);

  DefineCustomStringVariable("pg_opentelemetry.service_name",
                             "Service name to set in the trace", nullptr,
                             &settings.service_name, "postgres", PGC_USERSET, 0,
                             nullptr, nullptr, nullptr);

  DefineCustomStringVariable("pg_opentelemetry.deployment_environment",
                             "Deployment environment to set in the trace",
                             nullptr, &settings.deployment_environment,
                             "production", PGC_USERSET, 0, nullptr, nullptr,
                             nullptr);
}

void _PG_fini(void) {
  // Remove our hooks.
  ExecutorStart_hook = prev_executor_start_hook;
  ExecutorEnd_hook   = prev_executor_end_hook;

  // Shutdown the tracer if it is running.
  if (settings.enabled) {
    shutdown_tracer( );
  }
}
