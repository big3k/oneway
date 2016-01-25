// SYNACKReceiver.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <pcap.h>

#include "AmHttpSocket.h"

#define	CONFIGURATION_FILE	"configuration.txt"
#define	DEVICE		"\\Device\\Packet_{27423316-888D-43F9-913D-32F98AED12B5}"
#define	FILTER		"tcp and dst host 66.183.22.247 and tcp[13]=18 and src port 80"
#define PROXY_SUBMIT_URL	"http://proxyking.servehttp.com:8080/pk/service/"

#define	IP_LENGTH	25 //at least 25 for "%d.%d.%d.%d:%d=1\r\n"

void dispatcher_handler(u_char *, const struct pcap_pkthdr *, const u_char *);

char *proxySubmitURL = NULL;

enum PORTS {PORT25 = 0, PORT80, PORT135, PORT1080, PORT1813, PORT3128, PORT8080, PORTNUM};
char *ports[] = {"25", "80", "135", "1080", "1813", "3128", "8080", "wrongport"};

long nNumOfIP[PORTNUM];
long nTotalNumOfIP = 0;
CStdioFile *f[PORTNUM];
CString alldata[PORTNUM];
time_t  lastResponse[PORTNUM];
char* data[PORTNUM];
int numofupload[PORTNUM];
int numofuploadall = 0;

void SYNACKReceive() ;
UINT UploadData(LPVOID data);

BOOL save_to_file_flag = TRUE;
BOOL upload_flag = FALSE;
BOOL overwrite_flag = FALSE;

int max_upload = 100;

void init() {
//	printf("Port num is %d\n", PORTNUM); 
	for (int i = 0; i < PORTNUM; i++) nNumOfIP[i] = 0;
	for (i = 0; i < PORTNUM; i++) lastResponse[i] = 0;
	for (i = 0; i < PORTNUM; i++) data[i] = 0;
	for (i = 0; i < PORTNUM; i++) numofupload[i] = 0;
}
void printUsage() {
		printf("Usage: SYNACKReceiver -f (no|overwrite|append) -u (true|false) -n numer_of_upload_before_exit\n");
		printf("\t-f no: do not save to file.\n");
		printf("\t-f overwrite: save to file and overwrite previous result.\n");
		printf("\t-f append: save to file and append the result to previous files.\n");
		printf("\t-u true: upload result to web server. The web server is specified in configuration.txt.\n");
		printf("\t-u false: do not upload result to web server.\n");
		printf("\t-n numer of uploads before the program exits.\n");
		
}

int getIndex (int port) {
	if (port == 1080) {
		return PORT1080;
	} else if (port == 1813) {
		return PORT1813;
	} else if (port == 25) {
		return PORT25;
	} else if (port == 80) {
		return PORT80;
	} else if (port == 3128) {
		return PORT3128;
	} else if (port == 8080) {
		return PORT8080;
	} else if (port == 135) {
		return PORT135;
	} else  {
//		printf ("port %d is not supported.", port);
		return PORTNUM;
	} 
}

void openFile(int index) {
	char *output = new char[20];
//	printf("Port is %s\n", ports[index]);
	wsprintf(output, "%s.txt", ports[index]);
//	printf("Started opening file ......\n");

	f[index] = new CStdioFile( output, CFile::modeCreate | CFile::modeWrite | CFile::modeNoTruncate);
	f[index]->SeekToEnd();
	CTime now = CTime::GetCurrentTime();
	CString s = now.Format( "%A, %B %d, %Y" );
	f[index]->WriteString("#------- ");
	f[index]->WriteString(s);
	f[index]->WriteString(" -------\n");

	f[index]->Flush();
}

void openFiles() {
	for (int i = 0; i < PORTNUM; i++) {
		openFile(i);
	}
}

int main(int argc, char* argv[])
{
	if (argc != 7) {
		printUsage();
		return 0;
	} else {

		for (int i = 1; i < 7; i += 2) {
			if (strcmp(argv[i], "-f") == 0) {
				if (!strcmp(argv[i + 1], "no")) {
					save_to_file_flag = FALSE;
				} else if (!strcmp(argv[i + 1], "overwrite")) {
					save_to_file_flag = TRUE;
					overwrite_flag = TRUE;
				} else if (!strcmp(argv[i + 1], "append")) {
					save_to_file_flag = TRUE;
					overwrite_flag = FALSE;
				} else {
					printUsage();
					return 0;
				}
			} else
			if (strcmp(argv[i], "-u") == 0) {
				if (!strcmp(argv[i + 1], "true")) {
					upload_flag = TRUE;
				} else if (!strcmp(argv[i + 1], "false")) {
					upload_flag = FALSE;
				} else {
					printUsage();
					return 0;
				}
			} else
			if (strcmp(argv[i], "-n") == 0) {
				max_upload = atoi(argv[i + 1]);
			} else {
				printUsage();
				return 0;
			}
		}
	}

	printf("\nsave to file is %s\n", save_to_file_flag ? "true" : "false");
	printf("overwrite file is %s\n", overwrite_flag ? "true" : "false");
	printf("upload is %s\n", upload_flag ? "true" : "false");
	printf("upload number is %d\n\n", max_upload);

	init();
//parse arguments
//prepare output files
//	printf("Started opening files ......\n");
	if (save_to_file_flag) 
		openFiles();
//	printf("Finished opening files ......\n");

	SYNACKReceive() ;
	return 0;
}

void SYNACKReceive() 
{
	pcap_t *fp;
	char error[PCAP_ERRBUF_SIZE];
	char *device = NULL;
	char *filter=NULL;
	int i=0;
	struct bpf_program fcode;
	bpf_u_int32 SubNet,NetMask;
  

	//check if there is a file for configuration
	CStdioFile conf;
	
	if( !conf.Open( CONFIGURATION_FILE, CFile::modeRead | CFile::typeText ) ) 
	{
		//if not configuration file, use default configuration
		printf( "Use default configuration\n");
		device = new char[strlen(device)];
		wsprintf(device, "%s", DEVICE);
		device = new char[strlen(filter)];
		wsprintf (filter, "%s", DEVICE);
		proxySubmitURL = new char[strlen(PROXY_SUBMIT_URL)];
		wsprintf (proxySubmitURL, "%s", PROXY_SUBMIT_URL);
		
	}
	else
	{
		//use configuration file.
		printf( "Use configuration file\n\n");
		CString s;
		device = new char[100];
		conf.ReadString(s);
		wsprintf(device, "%s%", s);

		filter = new char[100];
		conf.ReadString(s);
		wsprintf(filter, "%s%", s);

		proxySubmitURL = new char[100];
		conf.ReadString(s);
		wsprintf(proxySubmitURL, "%s", s);

		conf.Close();
	}
	{
		//print out message
		printf( "Device is %s\n", device);
		printf( "Filter is %s\n", filter);
		printf( "Proxy submit url is %s\n\n", proxySubmitURL);
	}

	//open a capture from the network
	if (device != NULL)
	{
		if ( (fp= pcap_open_live(device, 1514, 1, 20, error) ) == NULL)
		{
			printf( "Unable to open the adapter. Please check configuration.txt\n");
			return;
		}
	}
	if(filter!=NULL)
	{
	  
		//obtain the subnet
		if(device!=NULL)
		{
			if(pcap_lookupnet(device, &SubNet, &NetMask, error)<0)
			{
				printf("Unable to obtain the netmask\n");
				return;
			}
		}
		else NetMask=0xffffff; //If reading from file, we suppose to be in a C class network
	  
		//compile the filter
		if(pcap_compile(fp, &fcode, filter, 1, NetMask)<0)
		{
			printf("Error compiling filter: wrong syntax. Please check configuration.txt\n");
			return;
		}
	  
		//set the filter
		if(pcap_setfilter(fp, &fcode)<0)
		{
			printf("Error setting the filter\n");
			return;
		}
	  
	}
	printf("Started capturing ......\n");
  
	//start the capture
	while (TRUE)
	{
		pcap_loop(fp, -1, dispatcher_handler, (unsigned char *)NULL);
	}

/*	
	while (TRUE)
	{
		pcap_dispatch(fp, -1, dispatcher_handler, (unsigned char *)f);
		wsprintf(msg, "%d", nNumOfIP);
		pIPNum->SetWindowText(msg);
	}
*/
  
	pcap_close(fp);
}

void handleoneIP(const char* ip, int index) 
{
//	printf("Index is %d \n", index);

	nTotalNumOfIP++;
	nNumOfIP[index]++;
	printf("%10d%10d\t%s", nNumOfIP[index], nTotalNumOfIP, ip);
	if (save_to_file_flag) 	{	
//		printf("saving IP ... \n");
		f[index]->WriteString(ip);
		f[index]->Flush ();
//		printf("IP saved... \n");
	} else {
//		printf("IP not saved\n");
	}
	if (upload_flag) { 
//		printf("upload is true\n");
		alldata[index] += ip;

		if (lastResponse[index] == 0) {
			lastResponse[index] = time(NULL);
//			printf("First response\n");
		} else if (((time(NULL) - lastResponse[index] > 180)  
				|| ((nNumOfIP[index] % 500 == 0) && (time(NULL) - lastResponse[index] > 60)))
			&& (alldata[index].GetLength() > 10)){
//			printf("Time to upload\n");
			lastResponse[index] = time(NULL);
//upload data
//clean the data used by last time 
			(alldata[index]) = "&candidate=" + (alldata[index]);
			(alldata[index]) = ports[index] + (alldata[index]);
			(alldata[index]) = "port=" + (alldata[index]);
			if (data[index] != NULL) {
				delete data[index];
				data[index] = NULL;
			}

			(data[index]) = strdup(alldata[index]);
			alldata[index].Empty();
			if (strlen(data[index]) > 10) {
//				printf("Start uploading \n");
//				printf("---------------------------------------- \n");
//				printf("%s\n", data[index]);
//				printf("---------------------------------------- \n");
				AfxBeginThread(UploadData, (LPVOID)(data[index]));
				numofupload[index]++;
			}
			//UploadData((LPVOID)data);

		} else {
//			printf("Following response \n");
		}
	} else {
//		printf("Upload is false\n");
	}
}

//Callback function called by libpcap for every incoming packet
void dispatcher_handler(u_char *fileNOTINUSE,
						const struct pcap_pkthdr *header, const u_char *pkt_data)
{
	unsigned char ip1, ip2, ip3, ip4; //source ip address
	unsigned short port = 11; //port
	unsigned char ihl = 5; //header length

	char *ip = new char[IP_LENGTH];

	ip1 = pkt_data[26];
	ip2 = pkt_data[27];
	ip3 = pkt_data[28];
	ip4 = pkt_data[29];

	ihl = pkt_data[14] & 15;

	port = pkt_data[ihl * 4 + 14] * 256 + pkt_data[ihl * 4 + 14 + 1];
	
	wsprintf(ip, "%d.%d.%d.%d:%d\r\n", ip1, ip2, ip3, ip4, port);
//	printf("%s\n", ip);
	int index = getIndex(port);
	if (index < PORTNUM) {
		handleoneIP(ip, index);
	}
	delete ip;
}

// use this function to upload data (ip) to server
UINT UploadData(LPVOID notusedata)
{

	CAmHttpSocket http;
	CString dataStr;
	dataStr = "service=NewProxyCandidate&action=submit&";
	dataStr += (char*)notusedata;

	char *s = http.GetPage(_T(proxySubmitURL), 
		true, dataStr);

	CTime now = CTime::GetCurrentTime();
	CString time = now.Format( "%B %d, %H:%M:%S" );

	printf("----------------%d Data uploaded at: %s -------------\r\n", 
		(numofuploadall + 1), time);
	delete s;
	dataStr.Empty();

	if (numofuploadall++ >= max_upload) {
		exit(0);
	}
	return 0;


}



