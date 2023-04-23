#!/bin/bash

rsync -ar pl Makefile -e ssh liuzesen@10.3.51.50:adapchol > /dev/null
ssh liuzesen@10.3.51.50 "cd /home/liuzesen/adapchol && make $1"
rsync -ar -e ssh liuzesen@10.3.51.50:adapchol/build/pl ./build > /dev/null
rsync -ar -e ssh liuzesen@10.3.51.50:adapchol/pl/vitis_prj ./pl > /dev/null