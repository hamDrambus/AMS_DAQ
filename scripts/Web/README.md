# Web

## Dependencies

    pip3 install Flask
    pip3 install Flask-Restful
    pip3 install redis
    sudo yum install redis
    sudo systemctl start redis
    sudo systemctl enable redis

## Run test server

    python3 rest.py

Go to URL:

    http://<host>:5000/metrics

Post data with curl with the available metrics (datarate and cpuutil):

    curl -d '{"value":5}' -H "Content-Type: application/json" -X POST http://<host>:5000/add/datarate

To run the full metrics example:

1. run the metrics publisher:

       ./build/bin/test_metrics

2. run test server:

       cd scripts/Web
       python3 rest.py

3. run metrics subscriber:

       cd scripts/Monitoring/
       python3 metrics_sub.py

Go to URL:

    http://<host>:5000/graph
