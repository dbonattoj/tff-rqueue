#include "queue.pml"

#define SA_past 1
#define SA_future 0
#define SB_past 0
#define SB_future 1
#define AM_past 0
#define AM_future 1
#define BM_past 1
#define BM_future 0

#define M_prefetch (length-3)
#define A_prefetch 1
#define B_prefetch 0

mtype { read_msg, did_read_msg }
chan A_chan = [0] of { mtype, int, int };
chan B_chan = [0] of { mtype, int, int };


inline flow_trunc(x) {
	x = (x >= 0 -> x : 0);
}

inline flow_propagate_request(rq_head_time, rq_tail_time) {
	// M (queue)
	int SM_past, SM_future, M_rq_head_time;
	d_step {
		SM_past = ( (SA_past + AM_past > SB_past + BM_past) -> SA_past + AM_past : SB_past + BM_past );
		SM_future = ( (SA_future + AM_future > SB_future + BM_future) -> SA_future + AM_future : SB_future + BM_future );
		M_rq_head_time = rq_head_time - SM_past;
		M_rq_head_time = (M_rq_head_time >= 0 -> M_rq_head_time : 0);
	}
	request(M_rq_head_time, rq_tail_time + SM_future + M_prefetch);
}


proctype M() {
accept:
	do
	:: write_next();
	od
}

proctype Async_node(chan rd_chan; int input_past; int input_future; int prefetch) {
	int rd_head_time = -1, rd_tail_time = -1, rd_tail_time_max = -1;

loop:
	do
	::	int M_rd_head_time, M_rd_tail_time;
		bool is_prefetch;
		
		if
		::	rd_chan?read_msg(rd_head_time,rd_tail_time) ->
			d_step {
				printf("node: req\n");
				M_rd_head_time = rd_head_time - input_past;
				M_rd_head_time = ( M_rd_head_time >= 0 -> M_rd_head_time : 0 );
				M_rd_tail_time = rd_tail_time + input_future;
				rd_tail_time_max = rd_tail_time + prefetch;
				is_prefetch = false;
			}			
			
		::	(rd_head_time != -1 && rd_tail_time < rd_tail_time_max) ->
			d_step {
				printf("node: pref\n");
				rd_head_time++;
				rd_tail_time++;
				M_rd_head_time = rd_head_time - input_past;
				M_rd_head_time = ( M_rd_head_time >= 0 -> M_rd_head_time : 0 );
				M_rd_tail_time = rd_tail_time + input_future;
				is_prefetch = true;
			}
		fi;
	
		bool tfail;
try_read:
		read(M_rd_head_time, M_rd_tail_time, tfail);
		if
		:: tfail && !is_prefetch -> goto try_read;
		:: tfail && is_prefetch -> rd_tail_time_max = rd_tail_time; goto loop;
		:: else -> skip
		fi
		
		if
		::	!is_prefetch -> rd_chan!did_read_msg(-1,-1);
		::	else -> skip
		fi
	od
}


proctype Sink() {
	int counter = 0;
	int t = -1;
	
	int A_rd_head_time, A_rd_tail_time;
	int B_rd_head_time, B_rd_tail_time;
	
	do
	::	(counter < 3) ->
		int dur = 1;
		if
		::	printf("short seq\n"); t++; dur = 3; // sequential read
		::	printf("seek back\n"); t = 0; // seek back
		::	printf("seek fwd\n"); t = t + 2; // seek forward
		fi;
	
		int i = 0;
		for(i : 1 .. dur) {
			d_step {
				A_rd_head_time = t - SA_past;
				A_rd_tail_time = t + SA_future + 1;
				A_rd_head_time = ( A_rd_head_time >= 0 -> A_rd_head_time : 0 );
				B_rd_head_time = t - SB_past;
				B_rd_tail_time = t + SB_future + 1;
				B_rd_head_time = ( B_rd_head_time >= 0 -> B_rd_head_time : 0 );
			}
			
			flow_propagate_request(t, t + 1);
	
			A_chan!read_msg(A_rd_head_time, A_rd_tail_time);
			A_chan?did_read_msg(_,_);
			B_chan!read_msg(B_rd_head_time, B_rd_tail_time);
			B_chan?did_read_msg(_,_);
			
			t++;
		}
		
		counter++;
	
	::	else -> break;
	od
sink_ended:
	skip;
}


init {
	run M();
	run Async_node(A_chan, AM_past, AM_future, A_prefetch);
	run Async_node(B_chan, BM_past, BM_future, B_prefetch);
	run Sink();
}


never {
accept:
	do
	:: !Sink@sink_ended;
	od
}
