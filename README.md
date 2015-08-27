Built this initially as a "socket server", but turned into more of an HTTP server as I needed it for another
project that was handling thousands of requests/second.

After reading about the C10K problem I went with epoll for this project.  I had used select() in a number of 
other socket related projects (see my MUD repositories). 

Usage as far as I remember:<br>
1) Build the project (make)<br>
2) Run bserverd.php<br>

There are a number of configuration flags which you can see with: ./bserver -h

There is also connection profiling, which will tell you how long each part of a request is taking, from 
creating the connection, to receiving or sending data, etc.  This is a flag you enable at process startup.

Things I did in consideration of speed:<br>
1) I built a "smart buffer" object, that keeps buffers in memory and re-uses them.  It creates buffers of varying sizes and if a connection sends more data than their current buffer needs, it will be given a bigger buffer.  Once the connection closes, the buffer is kept in memory, cleared, ready to be re-used.<br>
2) Used <a href="http://www.kegel.com/c10k.html">epoll</a><br>
3) As far as the HTTP side goes, I read minimum headers, only looking for what I needed (like KEEPALIVE).<br>
4) The profiling I mentioned above, to troubleshoot what was causing any slowness<br>

My next phase for speed was to introduce threads and buffer lists -- see buffer_list.c and ideas.txt.  But I never finished this step.  Also I was considering a connection pool, as from what I recall, linux creating and destroying connections all the time was a decent part of the overhead.

Have fun!
