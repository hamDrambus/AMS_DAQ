# Monitoring

## Dependencies

    sudo ansible-playbook install-redis.yml --ask-become-pass
    sudo ansible-playbook install-webdeps.yml --ask-become-pass

## Run test server

    python3 metric-manager.py metrics-config.json

## Tweak the config

If no `metrics` field is provided, all available metrics are forwarded to both the active destinations.

An example of `metrics`:

    "metrics": [
      "metricssimulator-RandomMetric1-int,i",
      "metricssimulator-RandomMetric2-float,w",
      "metricssimulator-RandomMetric3-dobule,b",
      "metricssimulator-RandomMetric8-double_average,r"
    ]     

Active destinations are:

    "influxDB_settings": {
      "host" : "localhost",
      "port" : "8086",
      "name" : "DAQlingMetrics"
    },
    "redis_settings": {
      "host" : "localhost",
      "port" : "6379"
    }

Omitting either the `influxDB_settings` or `redis_settings` will disable that destination (`redis_settings` is omitted by default).
