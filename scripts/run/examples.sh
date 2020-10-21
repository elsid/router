#!/bin/bash -ex

if [[ -z "${SRC}" ]]; then
    export SRC=../../..
fi

${SRC}/scripts/run/rpg/examples.sh
examples/community_example
