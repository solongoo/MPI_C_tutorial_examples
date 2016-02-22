# Author: Solongo Munkhjargaj

all: roller_coaster prime_numbers stable_marriage prime_numbers_mw
roller_coaster:
	mpicc -o roller_coaster roller_coaster.c
prime_numbers:
	mpicc -o prime_numbers prime_numbers.c
prime_numbers_mw:
	mpicc -o prime_numbers_mw prime_numbers_mw.c
stable_marriage:
	mpicc -o stable_marriage stable_marriage.c
rc: roller_coaster
	mpirun -np 5 -hostfile hostfile ./roller_coaster 1 1
pn: prime_numbers
	mpirun -np 5 -hostfile hostfile ./prime_numbers 20
pn_mw: prime_numbers_mw
	mpirun -np 5 -hostfile hostfile ./prime_numbers_mw 20
sm: stable_marriage
	mpirun -np 21 -hostfile hostfile ./stable_marriage
clean:
	rm roller_coaster
	rm prime_numbers
	rm stable_marriage
	rm prime_numbers_mw
