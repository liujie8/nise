gcc -c generic.c host.c imop.c mathop.c sift.c
ar -r libsift.a generic.o host.o imop.o mathop.o sift.o
