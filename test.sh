test_case=$1
cd ~ # Set workspace
gcc -std=c99 -o ./simulate $KUN/lc3c/lc3sim.c # Compile lc3sim
chmod 777 ./simulate # Change permission
./lc3tools/lc3as $KUN/lc3c/tests/$test_case.asm # Compile test assembly code
hexdump -v -e ' "0x" 2/1 "%02X" "\n" ' $KUN/lc3c/tests/$test_case.obj | tee ./$test_case.isaprogram # Output isaprogram
cp ./$test_case.isaprogram $KUN/lc3c/tests # Copy to $KUN for debug
# echo 'go' > ./simulate $test_case.isaprogram
./simulate $test_case.isaprogram
