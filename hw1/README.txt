Jonathan: wrq
Qianting: rrq 
We both worked on some of the utility function that help to execute some of the more granular actions.
Implemented the timeout with alarm() and EINTR, where when alarm() reached its time, it would call alarm_handler thus interrupting with the recvfrom, getting a EINTR.
