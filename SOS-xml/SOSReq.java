
// Parse XML using StAX
import javax.xml.stream.*;
import javax.xml.stream.events.*;
import javax.xml.stream.XMLInputFactory;
import java.io.*;
import java.util.*;
import java.text.*;

import org.apache.commons.httpclient.*;
import org.apache.commons.httpclient.methods.*;

public class SOSReq {
 public static void main( String[] args ) {
   if (args.length != 2 ) {
     System.err.println("Usage: java SOSReq time outfile\r\nExample: java SOSReq 2006-10-30T03:00:00Z /tmp/sm.dat\r\n"); 
     System.exit(-1); 
   } 
   String url = "http://192.239.84.150:18080/LISW/SOS";
   String xmldata =
	"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n" +
	"<sos:GetObservation xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\r\n" +
 	"  xsi:schemaLocation=\"http://www.opengis.net/sos/1.0 http://schemas.opengis.net/sos/1.0.0/sosAll.xsd\"\r\n" +
 	"  xmlns:sos=\"http://www.opengis.net/sos/1.0\"\r\n" +
 	"  xmlns:om=\"http://www.opengis.net/om/1.0\"\r\n" +
 	"  xmlns:ogc=\"http://www.opengis.net/ogc\"\r\n" +
 	"  xmlns:gml=\"http://www.opengis.net/gml\"\r\n" +
 	"  service=\"SOS\" version=\"1.0.0\" srsName=\"urn:ogc:def:crs:EPSG:4326\"> \r\n" +
	"<sos:offering>LISv5</sos:offering>\r\n" +
	"<sos:eventTime>\r\n" +
  	"  <ogc:TM_During>\r\n" +
    	"    <ogc:PropertyName>om:samplingTime</ogc:PropertyName>\r\n" +
    	"    <gml:TimePeriod>\r\n" +
       	"      <gml:beginPosition>"+ args[0] + "</gml:beginPosition>\r\n" +
       	"      <gml:endPosition>" + args[0] + "</gml:endPosition>\r\n" +
    	"    </gml:TimePeriod>\r\n" +
  	"  </ogc:TM_During>\r\n" +
	"</sos:eventTime>\r\n" +
	"<sos:observedProperty>urn:ogc:def:property:CREW:SoilMoist</sos:observedProperty>\r\n" +
	"<sos:featureOfInterest>\r\n" +
	"  <ogc:BBOX>\r\n" +
        "    <ogc:PropertyName>gml:location</ogc:PropertyName>\r\n" +
        "      <gml:Envelope srsName=\"urn:ogc:def:crs:EPSG:4326\">\r\n" +
        "        <gml:lowerCorner>25.875 -124.875</gml:lowerCorner>\r\n" +
        "        <gml:upperCorner>52.875 -67.875</gml:upperCorner>\r\n" +
        "      </gml:Envelope>\r\n" +
        "    </ogc:BBOX>\r\n" +
	"</sos:featureOfInterest>\r\n" +
	"<sos:responseFormat>text/xml;subtype=&quot;om/1.0.0&quot;</sos:responseFormat>\r\n" +
	"<sos:resultModel>om:Observation</sos:resultModel>\r\n" +
	"<sos:responseMode>inline</sos:responseMode>\r\n" +
	"</sos:GetObservation>\r\n"; 


   try {
    HttpClient client = new HttpClient();
    PostMethod method = new PostMethod( url );

    System.out.println("-------------------Request-----------------\r\n");
    System.out.println(xmldata);

    //method.setRequestBody(xmldata);
    method.addParameter("body", xmldata);

	 // Execute the POST method
    int statusCode = client.executeMethod( method );
    if( statusCode != -1 ) {
      String contents = method.getResponseBodyAsString();
      System.out.println("-------------------Response-----------------\r\n");
      System.out.println( contents );
      InputStream xmlresp; 
      xmlresp = method.getResponseBodyAsStream();
      saveData(parseXML(xmlresp), args[0], args[1]); 
      
      method.releaseConnection();
//      getData(args[0], args[1]);   // short-circuit for now YDT
    }
   } catch( Exception e ) {
    e.printStackTrace();
   }

 } //main 

 public static String parseXML(InputStream xml) { 
 String rawdata = ""; 
 try {
   XMLInputFactory inputFactory=XMLInputFactory.newInstance();
   XMLEventReader xmlEventReader=inputFactory.createXMLEventReader(xml);

   while(xmlEventReader.hasNext()){

      XMLEvent xmlEvent = xmlEventReader.nextEvent();
            if (xmlEvent.isStartElement()){
                if(xmlEvent.asStartElement().getName().getLocalPart() == ("values") ) { 
                  xmlEvent = xmlEventReader.nextEvent();
                  rawdata = xmlEvent.asCharacters().getData(); 
                  continue; 
                 }
             }

   }
   xmlEventReader.close(); 
   //System.out.print("Element data: " + rawdata + "End"); 

 } catch(FactoryConfigurationError e) {
   System.out.println("FactoryConfigurationError"+e.getMessage());
 } catch(XMLStreamException e) {
   System.out.println("XMLStreamException"+e.getMessage());
 } //try

 return rawdata; 

 }  //mehtod 


 public static void saveData(String rawdata, String dtime, String outfile) {
  try {
  /*
   DateFormat dfm = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss'Z'");
   Date ddate = dfm.parse(dtime); 
   System.out.println("Result: " + dfm.format(ddate));  
   dfm = new SimpleDateFormat("yyyy'/'yyyyMMdd'/'yyyyMMddHHmm'.d01.gs4r'");
   System.out.println("Result2: " + fpath + dfm.format(ddate));  
  */

   PrintWriter outputStream = 
                new PrintWriter(new FileWriter(outfile));

   String[] lines = rawdata.trim().replaceAll("@@@@", "@@").split("@@"); 
   System.out.println ("Decoding the data...");
   int nlines = lines.length; 
   if (nlines > 70) { nlines=nlines-1; }  // cut off the last line
   for (int i=0; i < nlines; i++) {
    System.out.println(i +  lines[i]); 
    String[] words = lines[i].split(" ");  
    outputStream.println (words[0] + " " + words[7] + " " + words[8] 
                          + " " + words[9]);
   }
   
   outputStream.close(); 
  } catch( Exception e ) { e.printStackTrace(); }

 } // method 



 public static void getData(String dtime, String outfile) {
  try {
   String fpath="/home/hongbo/src/MPI_LIS/LIS5.0BETA_2008_Feb_Su/OUT/LISW/EXPclm/CLM/"; 
   DateFormat dfm = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss'Z'");
   Date ddate = dfm.parse(dtime); 
   System.out.println("Result: " + dfm.format(ddate));  
   dfm = new SimpleDateFormat("yyyy'/'yyyyMMdd'/'yyyyMMddHHmm'.d01.gs4r'");
   System.out.println("Result2: " + fpath + dfm.format(ddate));  
   
   RandomAccessFile infile = new RandomAccessFile(fpath + dfm.format(ddate), "r"); 
   int reclen= 229*109*4 + 8;  //fortran seq access
   byte[] data = new byte[reclen]; 
   // SoilMoist1 is 24th record 
   infile.seek( reclen * 23 ); 
   infile.read(data); 
   infile.close(); 

   RandomAccessFile outf = new RandomAccessFile(outfile, "rw"); 
   outf.write(data); 
   outf.close(); 
  
  } catch( Exception e ) {
    e.printStackTrace();
  }


 }  //mehtod




}
