test-vcd: test-vcd.o ngoustr.o pzfd.o vcd-h1.o teefd.o
	c++ -o test-vcd test-vcd.o ngoustr.o pzfd.o vcd-h1.o -L ~//open-vcdiff/open-vcdiff-read-only/.libs/ -lvcdcom -lvcdenc

