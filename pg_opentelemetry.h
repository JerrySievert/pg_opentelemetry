#pragma once

// Store the settings for the extension.
struct otlp_settings {
  // Whether enabled or not.
  bool enabled = false;
  // Endpoint for the exporter, if there is one.
  char *otlp_endpoint = nullptr;
  // Type of exporter: LOG, HTTP currently.
  int otlp_exporter = 0;
  // Max queue size, we want this to stay just over 2x max export.
  int max_queue_size = 32;
  // Max export batch size.
  int max_export_batch_size = 10;
  // Service name.
  char *service_name;
  // Deployment environment.
  char *deployment_environment;
};

void init_tracer( );
void shutdown_tracer( );

extern otlp_settings settings;

// Exporter types.
#define NUM_EXPORTERS 2
#define EXPORTER_LOG 0
#define EXPORTER_HTTP 1
