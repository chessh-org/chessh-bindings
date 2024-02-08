#!/bin/sh

set -e

docker buildx build . -t natechoe/chessh-bindings
