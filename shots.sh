#! /bin/bash

export COCONFIG=$(pwd)/doc/build/config.vistle.doc.xml

export VWP=tiny.vwp
export OUTDIR=/Users/ma/git/vistle.github.io/docs/example/cfd

export COVER_TERMINATE_SESSION=1

function snapshot {
    local w="$1"

    echo "Workflow: $w"
    WORKFLOW=$w TASK=result vistle -b snapshot-workflow.py
    WORKFLOW=$w TASK=workflow vistle -g snapshot-workflow.py
}

if [ -z "$*" ]; then
    for w in workflow/tutorial/*.vsl; do
        snapshot "$w"
    done
else
    for w in "$@"; do
        snapshot "$w"
    done
fi
