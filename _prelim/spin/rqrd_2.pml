#include "queue.pml"


active proctype Read1() {
	read(0, 1, _);
reader_ended:
	skip;
}


active proctype Read2() {
	read(1, 2, _);
reader_ended:
	skip;
}

active proctype Request() {
	int rq_head_time, rq_dur;
	int counter = 0;

	do
	::	(counter < 3) ->
		select(rq_head_time : 0 .. 3);
		select(rq_dur : 2 .. 4);
		request(rq_head_time, rq_head_time + rq_dur);
		counter++;
		
	::	break;
	od
}

active proctype Write() {
	do
	:: write_next();
	od
}

never {
accept:
	do
	::	!Read1@reader_ended && !Read2@reader_ended;
	od
}
