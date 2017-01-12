typedef mutex {
	bool locked;
};

typedef shared_mutex {
	unsigned shared_locked:2;
	bool unique_locked;
};


inline mut_lock(mut) {
	atomic {
		(mut.locked == false) -> mut.locked = true;
	}
}

inline mut_unlock(mut) {
	assert(mut.locked);
	mut.locked = false;
}


inline mut_unique_lock(mut) {
	atomic {
		(mut.shared_locked == 0 && !mut.unique_locked) -> mut.unique_locked = true;
	}
}

inline mut_unique_unlock(mut) {
	mut.unique_locked = false;
}


inline mut_shared_lock(mut) {
	atomic {
		!mut.unique_locked -> mut.shared_locked++;
	}
}

inline mut_shared_unlock(mut) {
	mut.shared_locked--;
}


mtype { cv_notif_none, cv_notif_one, cv_notif_all }


typedef condition_variable {
	unsigned waiting:4;
	unsigned should_waiting:4;
};


inline cv_wait(cv, mut) {
	assert(mut.locked);
	atomic {
		(cv.waiting == cv.should_waiting);
		mut.locked = false;
		cv.should_waiting++;
		cv.waiting++;
	}
	atomic {
		(cv.waiting > cv.should_waiting);
		cv.waiting--;
		(mut.locked == false);
		mut.locked = true;
	}
}

inline cv_notify_one(cv) {
	d_step {
		if
		::	cv.should_waiting >= 1 -> cv.should_waiting--;
		::	cv.should_waiting == 0 -> skip;
		fi
	}
}

inline cv_notify_all(cv) {
	cv.should_waiting = 0;
}
