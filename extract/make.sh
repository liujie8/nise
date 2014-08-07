gcc -c generic.c 
gcc -c sift.c
gcc -c host.c
gcc -c mathop.c
gcc -c imop.c
gcc -c extract.cpp -I../include

gcc  -o ../bin/extract generic.o mathop.o sift.o imop.o host.o extract.o -L../lib -lfreeimage -L/usr/local/lib -lopencv_core -lopencv_highgui -lboost_program_options -lboost_thread  -lstdc++
