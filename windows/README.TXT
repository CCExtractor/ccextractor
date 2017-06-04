The Windows binaries are now distributed separately, please visit

http://ccextractor.sourceforge.net

For a link to the current version





For the windows multicast issue
Having more than 1 interfaces in windows is a problem as windows tends to pick up the wrong address.We can get all the interfaces in C and hence selecting the required interface.

C code for getting all the interfaces

'struct ifaddrs *addrs,*tmp;

getifaddrs(&addrs);
tmp = addrs;

while (tmp)
{
if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_PACKET)
printf("%s\n", tmp->ifa_name);

tmp = tmp->ifa_next;
}
freeifaddrs(addrs);`
