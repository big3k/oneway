gcc  -I/usr/include -I../WpdPack/Include -g -O2 -Wall -mno-cygwin -c win-pa2.c 
gcc -static -g -O2 -L../WpdPack/Lib -mno-cygwin -o win-pa2.exe win-pa2.o -lwpcap -lpacket -lws2_32 -liphlpapi
strip win-pa2.exe
exit


gcc  -I../usr/include -I../usr/include/libnet -I../usr/include/libnet/win32 -I../../WpdPack/Include -g -O2 -Wall -mno-cygwin -c pcap_list_dev.c 
gcc -static -g -O2 -L../../WpdPack/Lib -mno-cygwin -o pcap_list_dev.exe pcap_list_dev.o -lwpcap -lpacket -lws2_32 -liphlpapi
exit

gcc  -o adv_pcap.exe  -I../usr/include -I../usr/include/libnet/win32 -I../../WpdPack/Include -L../../WpdPack/Lib -mno-cygwin  adv_pcap.c -lwpcap -lpacket -lws2_32 -liphlpapi

exit

gcc  -I../usr/include -I../usr/include/libnet -I../usr/include/libnet/win32 -I../../WpdPack/Include -g -O2 -Wall -mno-cygwin -c sendarp.c 

gcc -static -g -O2 -L../../WpdPack/Lib -mno-cygwin -o sendarp.exe sendarp.o -lwpcap -lpacket -lws2_32 -liphlpapi

exit 



gcc -I../usr/include -I../usr/include/libnet -I../usr/include/libnet/win32 -I../../WpdPack/Include -g -O2 -Wall -mno-cygwin -c win-pa.c

gcc -static -g -O2 -L../../WpdPack/Lib -mno-cygwin -o win-pa.exe win-pa.o ../lib/libnet.a -lwpcap -lpacket -lws2_32 -liphlpapi

gcc -I../usr/include -I../usr/include/libnet -I../usr/include/libnet/win32 -I../../WpdPack/Include -g -O2 -Wall -mno-cygwin -c get_addr.c

gcc -static -g -O2 -L../../WpdPack/Lib -mno-cygwin -o get_addr.exe get_addr.o ../lib/libnet.a -lwpcap -lpacket -lws2_32 -liphlpapi

