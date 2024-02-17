### Created by Dr. Benjamin Richards (b.richards@qmul.ac.uk)

### Download base image from repo
FROM anniesoft/toolanalysis:base

### Run the following commands as super user (root):
USER root

Run cd ToolAnalysis; bash -c "source Setup.sh && git checkout -- . && make update && make clean && make && make"

### Open terminal
CMD ["/bin/bash"]

