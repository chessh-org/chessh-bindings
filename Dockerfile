FROM debian:stable-slim

RUN apt-get update && apt-get upgrade -y
RUN apt-get install -y build-essential

COPY . /chessh-bindings

RUN make -C /chessh-bindings -B
RUN mv /chessh-bindings/src/chessh.h /usr/include/chessh.h && \
    mv /chessh-bindings/build/libchessh.a /usr/lib/libchessh.a
