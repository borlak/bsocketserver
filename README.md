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

Have fun!
