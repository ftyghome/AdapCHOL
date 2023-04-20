transfer:
	ssh -t ubuntu@kria-lzs "/bin/bash -c \"mkdir -p /tmp/adapchol/build/host\""
	rsync -azr ./host -e ssh ubuntu@kria-lzs:/tmp/adapchol
compile: transfer
	ssh -t ubuntu@kria-lzs "/bin/bash -c \"cd /tmp/adapchol/build/host && cmake ../../host && make -j4\""
run_host: compile
	ssh ubuntu@kria-lzs "/tmp/adapchol/build/host/test" < /home/gns/adapchol/build/test/result/Journals/Journals_input.txt