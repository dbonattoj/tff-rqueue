#include "queue.pml"

active proctype Reader() {
	int rd_head_time = 0;
	int counter = 0;
	do		
	::	(counter <= 0) ->
		int dur = 1;
		if
		//::	printf("long seq\n"); rd_head_time = rd_head_time + 1; dur = length + length/2; // long sequential read
		::	printf("short seq\n"); rd_head_time = rd_head_time + 1; // short sequential read
		//::	printf("seek back\n"); rd_head_time = 0; // seek back
		//::	printf("seek fwd\n"); rd_head_time = rd_head_time + 2; // seek forward
		fi;
		int j;
		for(j : 1 .. dur) {
			request(rd_head_time, rd_head_time + length);
			read(rd_head_time, rd_head_time + 1, _);
			rd_head_time++;
		}
		counter++;
		
	:: else -> break;
	od;
reader_ended:
	skip;
}

active proctype Writer() {
accept:
	do
	:: write_next()
	od
}



never reader_stuck {
accept:
	do
	:: !Reader[0]@reader_ended
	od
}

