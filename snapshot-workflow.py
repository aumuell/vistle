import os
import time
from coGRMsg import *

doworkflow = False
doresult = False

workflow = os.getenv("WORKFLOW")
vwp = os.getenv("VWP")
outdir = os.getenv("OUTDIR")
task = os.getenv("TASK")

if task == "workflow":
    doworkflow = True
if task == "result":
    doresult = True

directory, filename = os.path.split(workflow)
basename, extension = os.path.splitext(filename)

print(f"workflow={workflow}, vwp={vwp}, dir={directory}, outdir={outdir}, file={basename}, ext={extension}")
workflowimg = basename + "-workflow.png"
resultimg = basename + "-vis.png"
if outdir:
    workflowimg = outdir + "/" + workflowimg
    resultimg = outdir + "/" + resultimg

source(workflow)
barrier()
setLoadedFile(workflow) #neded to reset the execution counter to prevent save dialog
setStatus("Workflow loaded: "+workflow)

if doresult:
    compute()
    barrier()
    cover = findFirstModule("COVER")
    if cover > 0:
        sendCoverMessage(coGRSetViewpointFile(vwp, 0))
        sendCoverMessage(coGRShowViewpointMsg(0))
        sendCoverMessage(coGRColorBarPluginMsg(coGRColorBarPluginMsg.ShowColormap))
        time.sleep(3) #needed until we can wait for cover to finish rendering

        snapshotCover(cover, resultimg)
        print(f"Visualization snapshot taken in {resultimg}")

if doworkflow:
    time.sleep(3) #wait until modules are started
    snapshotGui(workflowimg)
    print(f"Workflow snapshot taken in {workflowimg}")

time.sleep(1) #needed until we can wait for cover to finish rendering
vistle.quit()
