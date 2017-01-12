#include "queue.pml"

#define SA_past 1
#define SA_future 0
#define AM_past 0
#define AM_future 1

#define M_prefetch 1
#define A_prefetch 1

mtype { read_msg, did_read_msg }
chan A_chan = [0] of { mtype, int, int };


inline flow_propagate_request(rq_head_time, rq_tail_time) {
	// M (queue)
	int SM_past, SM_future, M_rq_head_time;
	d_step {
		SM_past = SA_past + AM_past;
		SM_future = SA_future + AM_future;
		M_rq_head_time = rq_head_time - SM_past;
		M_rq_head_time = (M_rq_head_time >= 0 -> M_rq_head_time : 0);
	}
	//request(M_rq_head_time, rq_tail_time + SM_future + M_prefetch);
	request(M_rq_head_time, M_rq_head_time + length)
}


active proctype M() {
accept:
	do
	:: write_next();
	od
}


active proctype A() {
	int rd_head_time = -1, rd_tail_time = -1, rd_tail_time_max = -1;

loop:
	do
	::	int M_rd_head_time, M_rd_tail_time;
		bool is_prefetch;
		
		if
		::	A_chan?read_msg(rd_head_time,rd_tail_time) ->
			d_step {
				printf("A: req\n");
				rd_tail_time_max = rd_tail_time + A_prefetch;
				is_prefetch = false;
			}			
			
		::	(rd_head_time != -1 && rd_tail_time < rd_tail_time_max) ->
			d_step {
				printf("A: pref\n");
				rd_head_time++;
				rd_tail_time++;
				is_prefetch = true;
			}
		fi;
	
		bool tfail;
		d_step {
			M_rd_head_time = rd_head_time - AM_past;
			M_rd_head_time = ( M_rd_head_time >= 0 -> M_rd_head_time : 0 );
			M_rd_tail_time = rd_tail_time + AM_future;
		}

try_read:
		read(M_rd_head_time, M_rd_tail_time, tfail);
		if
		:: tfail && !is_prefetch -> goto try_read;
		:: tfail && is_prefetch -> rd_tail_time_max = rd_tail_time; goto loop;
		:: else -> skip
		fi

		
		if
		::	!is_prefetch -> A_chan!did_read_msg(-1,-1);
		::	else -> skip
		fi
	od
}


active proctype S() {
	int counter = 0;
	int t = -1;
	
	int A_rd_head_time, A_rd_tail_time;
	
	do
	::	(counter < 3) ->
		int dur = 1;
		if
		::	printf("seq read\n"); skip; // sequential read
		::	printf("seek back\n"); t = 0; // seek back
		::	printf("seek fwd\n"); t++; // seek forward
		fi;
	
		int i = 0;
		for(i : 1 .. dur) {
			d_step {
				A_rd_head_time = t - SA_past;
				A_rd_tail_time = t + SA_future + 1;
				A_rd_head_time = ( A_rd_head_time >= 0 -> A_rd_head_time : 0 );
			}
			
			flow_propagate_request(t, t + 1);
	
			A_chan!read_msg(A_rd_head_time, A_rd_tail_time);
			A_chan?did_read_msg(_,_);
			
			t++;
		}
		
		counter++;
	
	::	else -> break;
	od
sink_ended:
	skip;
}


never {
accept:
	do
	:: !S@sink_ended;
	od
}
