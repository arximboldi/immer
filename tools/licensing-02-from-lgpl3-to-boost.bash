
#!/bin/bash

# https://github.com/osterman/copyright-header

copyright-header \
    --license LGPL3 \
    --copyright-software immer \
    --copyright-software-description "immutable data structures for C++" \
    --copyright-holder "Juan Pedro Bolivar Puente" \
    --copyright-year 2016 \
    --copyright-year 2017 \
    -r . \
    -o .

copyright-header \
    --license-file Boost \
    --copyright-software immer \
    --copyright-software-description "immutable data structures for C++" \
    --copyright-holder "Juan Pedro Bolivar Puente" \
    --copyright-year 2016 \
    --copyright-year 2017 \
    --copyright-year 2018 \
    -a . \
    -o .
