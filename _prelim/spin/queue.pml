#include "prim.pml"

#define length 5

#define OPT_DSTEP d_step

short values[length] = -1;
short value_readers[length] = 0;

short head_time = 0;
short head = 0;
short tail = 0;
bool is_full = false;

short request_head_time = 0;
short request_tail_time = 0;

short lowest_reader_time = -1;
shared_mutex readers_mutex;

mutex state_mutex;
condition_variable request_cv;
//condition_variable readable_cv;
#define readable_cv request_cv



inline get_tail_time(tail_time) {
	d_step {
		if
		::	is_full ->
				tail_time = head_time + length;
		::	(tail >= head && !is_full) ->
				tail_time = head_time + (tail - head);
		::	else ->
				tail_time = head_time + (length - (head - tail));
		fi
	}
}


inline request(req_head_time, req_tail_time) {
	assert(req_tail_time - req_head_time <= length);
	mut_lock(state_mutex);
	request_tail_time = req_tail_time;
	request_head_time = req_head_time;
	printf("requested [%d, %d]\n", request_head_time, request_tail_time);
	mut_unlock(state_mutex);
	cv_notify_one(request_cv);
	cv_notify_all(readable_cv);
}


inline do_write(t, ind) {
	assert(value_readers[ind] == 0);
	printf("writing %d req[%d,%d]\n", t, request_head_time, request_tail_time);

progress:
	values[ind] = t;
}


inline end_write() {
	mut_lock(state_mutex);
	tail = (tail + 1) % length;
	if
	:: (tail == head) -> is_full = true;
	:: else -> skip;
	fi
	mut_unlock(state_mutex);
	printf("readable notif\n");
	cv_notify_all(readable_cv);
}


inline write_next() {
	mut_lock(state_mutex);
	
	int tail_time; get_tail_time(tail_time);

write_retry:
	skip;
	
	if
	:: (request_head_time >= head_time && request_head_time <= tail_time && tail_time >= request_tail_time) ->
		// at end of requested span; wait for new request
		cv_wait(request_cv, state_mutex);
		goto write_retry;
		
		
	:: (request_head_time >= head_time && request_head_time <= tail_time && tail_time < request_tail_time && is_full) ->
		// within requested span; buffer is full: push head forward
		bool paused_readers = false;
		if
		::	lowest_reader_time == head_time ->
			mut_unique_lock(readers_mutex);
			paused_readers = true;
			lowest_reader_time = -1;
		::	else;
		fi
		OPT_DSTEP {
			head = (tail + 1) % length;
			head_time++;
			is_full = false;
		}
		if
		::	paused_readers -> mut_unique_unlock(readers_mutex);
		::	else;
		fi

		mut_unlock(state_mutex);
		
		get_tail_time(tail_time);
		do_write(tail_time, tail);
		end_write();
		
		
	:: (request_head_time >= head_time && request_head_time <= tail_time && tail_time < request_tail_time && !is_full) ->
		// within requested span; buffer not full
		mut_unlock(state_mutex);
		do_write(tail_time, tail);
		end_write();
		
	:: else ->
		// long jump
		mut_unique_lock(readers_mutex);
		OPT_DSTEP {
			head_time = request_head_time;
			tail = head;
			lowest_reader_time = -1;
			is_full = false;
		}
		mut_unique_unlock(readers_mutex);
		mut_unlock(state_mutex);
				
		get_tail_time(tail_time);
		do_write(tail_time, tail);
		end_write();
	fi
}


inline do_read(rd_head_time, rd_tail_time, head) {		
	int i;
	int lim = rd_tail_time - rd_head_time - 1;
	
	d_step {
		for(i : 0 .. lim) {
			int ind = (head + i) % length;
			value_readers[ind]++;
		}
	}
	
progress:
	d_step {
		for(i : 0 .. lim) {
			int val = rd_head_time + i;
			int ind = (head + i) % length;
			assert(values[ind] == val);
		}
	}
	
	d_step {
		for(i : 0 .. lim) {
			int ind = (head + i) % length;
			value_readers[ind]--;
		}
	}
}

inline read_debug(rd_head_time, rd_tail_time) {
	d_step {
		int _tail_time;
		get_tail_time(_tail_time);
		printf("have[%d,%d] want[%d,%d], req[%d,%d]\n", head_time, _tail_time, rd_head_time, rd_tail_time, request_head_time, request_tail_time);
	}
}

inline end_read() {
	mut_shared_unlock(readers_mutex);
}

inline read(rd_head_time, rd_tail_time, out_tfail) {
	d_step { printf("read()"); read_debug(rd_head_time, rd_tail_time); }
	
	mut_lock(state_mutex);

read_retry:
	skip;
	
	printf("reader grab req [%d,%d]", request_head_time, request_tail_time);
	
	if
	::	(rd_head_time < request_head_time || rd_tail_time > request_tail_time) ->
		mut_unlock(state_mutex); goto read_tfail;
	::	else -> skip;
	fi

	int tail_time; get_tail_time(tail_time);

	if
	:: (rd_head_time >= head_time && rd_tail_time <= tail_time) ->
		if
		:: (lowest_reader_time == -1 || lowest_reader_time > rd_head_time) -> lowest_reader_time = rd_head_time;
		:: else -> skip;
		fi
		
		mut_shared_lock(readers_mutex);
		mut_unlock(state_mutex);
		do_read(rd_head_time, rd_tail_time, head + (rd_head_time - head_time));
		// reading ...
		end_read();
		
		out_tfail = false;
		goto read_end;
		
	:: else ->
		d_step { printf("waiting readable..."); read_debug(rd_head_time, rd_tail_time); }
		cv_wait(readable_cv, state_mutex);
		d_step { printf("waiting readable.  "); read_debug(rd_head_time, rd_tail_time); }
		
		goto read_retry;
		
	fi
	
read_tfail:
	skip;
	printf("tfail ");
	out_tfail = true;

read_end:
	d_step { printf("read end."); read_debug(rd_head_time, rd_tail_time); }
	skip;
}
