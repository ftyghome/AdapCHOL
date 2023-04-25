# Begin of parameters ---
KV260_HOSTNAME = kria-lzs

# End of parameters ---

MKFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
CURRENT_DIR := $(dir $(realpath $(lastword $(MAKEFILE_LIST))))

.DEFAULT_GOAL := all
dir_guard=@mkdir -p $(@D)
PL_DIR = $(CURRENT_DIR)/pl
PS_SRC_DIR = $(CURRENT_DIR)/ps_src
PRODUCT_DIR = $(CURRENT_DIR)/build/pl/product
XO_FILE = $(PRODUCT_DIR)/xo/kernel.xo
XCLBIN_FILE = $(PRODUCT_DIR)/xclbin/kernel.xclbin
V++_COMMAND_PRE = v++ -t hw --platform kv260_custom
HLS_PROJECT_DIR = $(PL_DIR)/vitis_prj/adapchol_hls
SOLUTION_NAME = solution1

KERNEL_VERILOG = $(HLS_PROJECT_DIR)/$(SOLUTION_NAME)/syn/verilog/krnl_proc_col.v


PL_DEPS = $(PL_DIR)/src/krnl_col.cpp $(PL_DIR)/include/krnl_col.h ${PL_DIR}/include/common_def.h

transfer_ps_test:
	scp mats/mytest.txt ubuntu@$(KV260_HOSTNAME):mytest.txt

transfer_ps:
	ssh -t ubuntu@kria-lzs "/bin/bash -c \"mkdir -p /tmp/adapchol/build/host/debug && mkdir -p /tmp/adapchol/build/host/rel && mkdir -p /tmp/adapchol/build/host/reldbg\""
	rsync -azr ./host -e ssh ubuntu@kria-lzs:/tmp/adapchol

compile_ps: transfer_ps
	ssh -t ubuntu@kria-lzs "/bin/bash -c \"cd /tmp/adapchol/build/host/debug && cmake ../../../host && make -j4\""

compile_ps_rel: transfer_ps
	ssh -t ubuntu@kria-lzs "/bin/bash -c \"cd /tmp/adapchol/build/host/rel && cmake -DCMAKE_BUILD_TYPE=Release ../../../host && make -j4\""

compile_ps_reldbg: transfer_ps
	ssh -t ubuntu@kria-lzs "/bin/bash -c \"cd /tmp/adapchol/build/host/reldbg && cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ../../../host && make -j4\""


run_ps: compile_ps
	ssh ubuntu@kria-lzs "/tmp/adapchol/build/host/test_host result.txt csparse_result.txt /dev/null /dev/null /dev/null" < /home/gns/adapchol/mats/mytest.txt

run_ps_big: compile_ps
	ssh ubuntu@kria-lzs "/tmp/adapchol/build/host/test_host result.txt csparse_result.txt /dev/null /dev/null /dev/null" < /home/gns/adapchol/mats/bcsstm22/bcsstm22_input.txt

$(KERNEL_VERILOG):$(PL_DEPS)
	cd pl/vitis_prj && CPATH="/usr/include/x86_64-linux-gnu" vitis_hls ../tcls/csynth_design.tcl

csynth: $(KERNEL_VERILOG)

$(XO_FILE): $(KERNEL_VERILOG)
	cd pl/vitis_prj && CPATH="/usr/include/x86_64-linux-gnu" vitis_hls ../tcls/export_design.tcl -tclargs $(XO_FILE)

export_design: $(XO_FILE)

$(XCLBIN_FILE): $(XO_FILE)
	$(dir_guard)
	mkdir -p $(@D)/tmp; \
	mkdir -p $(@D)/log; \
	mkdir -p $(@D)/report; \
	$(V++_COMMAND_PRE) \
	--temp_dir $(@D)/tmp \
	--log_dir $(@D)/log \
	--report_dir $(@D)/report \
	--optimize 3 \
	--link $(XO_FILE) -o $(@)

get_xclbin: $(XCLBIN_FILE)

transfer_pl: $(XCLBIN_FILE)
	scp build/pl/product/xclbin/kernel.xclbin ubuntu@$(KV260_HOSTNAME):/lib/firmware/xilinx/adapchol/binary_container_1.bin

load_pl:
	ssh ubuntu@$(KV260_HOSTNAME) "sudo xmutil unloadapp && sudo xmutil loadapp adapchol"

clean:
	rm -rf $(PRODUCT_DIR)
	rm -rf $(HLS_PROJECT_DIR)/$(SOLUTION_NAME)/csim
	rm -rf $(HLS_PROJECT_DIR)/$(SOLUTION_NAME)/impl
	rm -rf $(HLS_PROJECT_DIR)/$(SOLUTION_NAME)/syn
	rm -rf .Xil
	rm *.log

print_dir:
	echo $(CURRENT_DIR)
	echo $(XO_FILE)
	echo $(XCLBIN_FILE)
	echo $(KERNEL_VERILOG)