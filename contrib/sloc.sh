#! /bin/bash

sloc \
    -i '^lib/vistle/|^app/|^module/' \
    -e '^lib/vistle/config/covconfig/detail/toml|^app/gui/icons|^module/univiz|^module/general/Calc/exprtk|^module/read/ReadIewMatlabCsvExports/rapidcsv/|^module/read/ReadCsv/fast-cpp-csv-parser|^module/geometry/DelaunayTriangulator/tetgen|^module/map/IsoSurface/cccl|^module/read/ReadSubzoneTecplot/tecio' \
    "$@" \
    .
