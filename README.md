# pg_opentelemetry

An opentelemetry instrumentation extension for PostgreSQL.

## Disclaimers

This is currently alpha software, and as such it is bound to crash and should not be
used in production. In addition, it currently does not tie back to a primary trace,
making its usefulness a little lower than it could be.

Both of these will change, but it is important to note the current limitations.

## Usage

To use pg_opentelemetry, it must be loaded as a shared library:

postgresql.conf:

```
shared_preload_libraries = 'pg_opentelemetry'
```

There are other configuration parameters:

| parameter                                 | description            | values       | default                           |
| ----------------------------------------- | ---------------------- | ------------ | --------------------------------- |
| `pg_opentelemetry.enabled`                | Enabled/Disabled       | `on` `off`   | `off`                             |
| `pg_opentelemetry.max_queue_size`         | Maximum queue size     | `1`-`1024`   | `64`                              |
| `pg_opentelemetry.max_export_batch_size`  | Maximum batch size     | `1`-`128`    | `32`                              |
| `pg_opentelemetry.otlp_exporter`          | Type of exporter       | `log` `http` | `http`                            |
| `pg_opentelemetry.otlp_endpoint`          | HTTP endpoint          | `url`        | `http://127.0.0.1:4318/v1/traces` |
| `pg_opentelemetry.service_name`           | Service Name           | `string`     | `postgres`                        |
| `pg_opentelemetry.deployment_environment` | Deployment Environment | `string`     | `production`                      |

## Building

Currently requires:

- `libcurl`
- `libprotobuf`
