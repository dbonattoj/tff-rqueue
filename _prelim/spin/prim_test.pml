#include "prim.pml"

int value_;
condition_variable cv_;
mutex mut_;

int received = 0;

active [4] proctype Waits() {
	mut_lock(mut_);
retry:
	if
	::	value_ == 1 -> skip;
	::	else -> cv_wait(cv_, mut_); goto retry;
	fi
	mut_unlock(mut_);
	
	printf("done\n");
	
	received++;
}

active proctype Signals() {
	mut_lock(mut_);
	value_ = 0;
	mut_unlock(mut_);
	cv_notify_all(cv_);

	mut_lock(mut_);
	value_ = 1;
	mut_unlock(mut_);
	cv_notify_one(cv_);
}
