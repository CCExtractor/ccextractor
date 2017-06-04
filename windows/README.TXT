The Windows binaries are now distributed separately, please visit

http://ccextractor.sourceforge.net

For a link to the current version.




For this issue we have to explicitly specify the interface so that the windows does not end up taking up the wrong one.
For that we can use the socket library function addr.sin_addr.s_addr = inet_addr(group) and bind it.
Assuming 239.168.100.1:5000 is the IP we want


    addr.sin_addr.s_addr = inet_addr(239.168.100.1:5000); 
 
Then we will bind this IP
