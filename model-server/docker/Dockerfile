# In order for this file to work, you should put compiled model_server binary
# in this very folder.

FROM ubuntu
COPY model_server /usr/local/bin/model_server
ENV PORT 8500
CMD model_server --zookeeper_hosts="$ZOOKEEPER_HOSTS" --zookeeper_base="$ZOOKEEPER_BASE" --port="$PORT"
EXPOSE 8500
