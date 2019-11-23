#!/bin/bash -ex

DIR=$(mktemp -d)

function run_example() {
    NAME=${1:?}
    if [[ "${2:x}" ]]; then
        INPUT=${2}
    else
        INPUT=${NAME}
    fi
    if [[ "${3:x}" ]]; then
        OUTPUT=${3}
    else
        OUTPUT=${INPUT}
    fi
    if [[ -f examples/rpg/${NAME} ]]; then
        examples/rpg/${NAME}_example < ${SRC:?}/examples/rpg/input/${INPUT}.txt > ${DIR:?}/${NAME}.txt
        diff ${SRC:?}/examples/rpg/output/${OUTPUT}.txt ${DIR:?}/${NAME}.txt
    fi
}

run_example if_then single_argument
run_example if_then_2 single_argument_2
run_example if_then_arguments multi_arguments
run_example virtual single_argument
run_example virtual_arguments multi_arguments multi_arguments_2
run_example virtual_arguments_state multi_arguments multi_arguments_2
run_example meta multi_arguments multi_arguments_2
run_example meta_2 multi_arguments multi_arguments_2
run_example meta_2_implicit_conversion multi_arguments multi_arguments_2
run_example meta_2_implicit_conversion_and_runtime_name multi_arguments multi_arguments_2
run_example meta_2_implicit_conversion_and_return multi_arguments multi_arguments_3
run_example meta_2_implicit_conversion_and_return_and_explicit_argument multi_arguments_2 multi_arguments_4
