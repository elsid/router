#!/bin/bash -ex

if [[ -z "${SRC}" ]]; then
    export SRC=../../..
fi

${SRC}/scripts/run/rpg/examples.sh
examples/community_example
examples/int_if_then_example
examples/int_router_example
