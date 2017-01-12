#include "queue.pml"


active proctype Read() {
	read(1, 4, _);
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
	::	!Read@reader_ended;
	od
}
