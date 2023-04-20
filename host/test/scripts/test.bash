#!/usr/bin/env bash
/usr/bin/env python3 ./test.py \
--testing_path "/home/gns/adapchol/build/test/" \
--source_path "/home/gns/adapchol/host/" \
--mat_path "/home/gns/adapchol/mats/" \
--filter_max_n 5000 \
--max_jobs 12
