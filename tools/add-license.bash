#!/bin/bash

# https://github.com/osterman/copyright-header

copyright-header \
    --license GPL3 \
    --copyright-software immer \
    --copyright-software-description "immutable data structures for C++" \
    --copyright-holder "Juan Pedro Bolivar Puente" \
    --copyright-year 2016 \
    -a . \
    -o .
