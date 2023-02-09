#include <chrono>

#include "opentelemetry/exporters/ostream/span_exporter_factory.h"
#include "opentelemetry/exporters/otlp/otlp_http_exporter_factory.h"
#include "opentelemetry/exporters/otlp/otlp_http_exporter_options.h"
#include "opentelemetry/sdk/trace/batch_span_processor_factory.h"
#include "opentelemetry/sdk/trace/tracer_provider_factory.h"
#include "opentelemetry/trace/provider.h"

#include "pg_opentelemetry.h"

namespace trace_api      = opentelemetry::trace;
namespace resource       = opentelemetry::sdk::resource;
namespace trace_sdk      = opentelemetry::sdk::trace;
namespace trace_exporter = opentelemetry::exporter::trace;
namespace nostd          = opentelemetry::nostd;
namespace otlp           = opentelemetry::exporter::otlp;

// Settings storage.
otlp_settings settings;

// Initialize the tracer with the configuration parameters.
void init_tracer( ) {
  // resources and attributes for the traces.
  trace_sdk::BatchSpanProcessorOptions options{ };
  options.max_queue_size        = settings.max_queue_size;
  options.schedule_delay_millis = std::chrono::milliseconds(3000);
  options.max_export_batch_size = settings.max_export_batch_size;

  resource::ResourceAttributes attributes = {
    { "service", "pg_opentelemetry" },
    { "version", "0.1.0" },
    { "service.name", settings.service_name },
    { "deployment.environment", settings.deployment_environment }
  };

  auto resource = resource::Resource::Create(attributes);

  // http exporter.
  if (settings.otlp_exporter == EXPORTER_HTTP) {
    otlp::OtlpHttpExporterOptions opts;
    opts.url = settings.otlp_endpoint;

    auto exporter = otlp::OtlpHttpExporterFactory::Create(opts);

    auto processor = trace_sdk::BatchSpanProcessorFactory::Create(
        std::move(exporter), options);

    std::shared_ptr<opentelemetry::trace::TracerProvider> provider =
        trace_sdk::TracerProviderFactory::Create(std::move(processor),
                                                 resource);

    // set the global trace provider.
    trace_api::Provider::SetTracerProvider(provider);
  } else {
    // log exporter.
    auto exporter = trace_exporter::OStreamSpanExporterFactory::Create( );

    auto processor = trace_sdk::BatchSpanProcessorFactory::Create(
        std::move(exporter), options);

    std::shared_ptr<opentelemetry::trace::TracerProvider> provider =
        trace_sdk::TracerProviderFactory::Create(std::move(processor),
                                                 resource);

    // set the global trace provider.
    trace_api::Provider::SetTracerProvider(provider);
  }
}

// Shut down the tracer.
void shutdown_tracer( ) {
  std::shared_ptr<opentelemetry::trace::TracerProvider> none;
  trace_api::Provider::SetTracerProvider(none);
}
