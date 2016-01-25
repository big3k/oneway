
import javax.xml.stream.*;
import javax.xml.stream.events.*;
import javax.xml.stream.XMLInputFactory;
import java.io.*;

public class XMLParser{

public void parseXMLDocument(){
try{
XMLInputFactory inputFactory=XMLInputFactory.newInstance();
InputStream input=new FileInputStream(new File("catalog.xml"));
XMLStreamReader  xmlStreamReader=inputFactory.createXMLStreamReader(input);

while(xmlStreamReader.hasNext()){
int event=xmlStreamReader.next();

if(event==XMLStreamConstants.START_DOCUMENT){
System.out.println("Event Type:START_DOCUMENT");
System.out.println("Document Encoding:"+xmlStreamReader.getEncoding());
System.out.println("XML Version:"+xmlStreamReader.getVersion());
}

if(event==XMLStreamConstants.START_ELEMENT){
System.out.println("Event Type: START_ELEMENT");

System.out.println("Element Prefix:"+xmlStreamReader.getPrefix());
System.out.println("Element Local Name:"+xmlStreamReader.getLocalName());
System.out.println("Namespace URI:"+xmlStreamReader.getNamespaceURI());

for(int i=0; i<xmlStreamReader.getAttributeCount();i++){

System.out.println("Attribute Prefix:"+xmlStreamReader.getAttributePrefix(i));
System.out.println("Attribute Namespace:"+xmlStreamReader.getAttributeNamespace(i));
System.out.println("Attribute Local Name:"+xmlStreamReader.getAttributeLocalName(i));
System.out.println("Attribute Value:"+xmlStreamReader.getAttributeValue(i));
}

}
if(event==XMLStreamConstants.ATTRIBUTE){
System.out.println("Event Type:ATTRIBUTE");
}

if(event==XMLStreamConstants.CHARACTERS){
System.out.println("Event Type: CHARACTERS");
System.out.println("Text:"+xmlStreamReader.getText());
}
if(event==XMLStreamConstants.COMMENT){
System.out.println("Event Type:COMMENT");
System.out.println("Comment Text:"+xmlStreamReader.getText());
}

if(event==XMLStreamConstants.END_DOCUMENT){
System.out.println("Event Type:END_DOCUMENT");
}
if(event==XMLStreamConstants.END_ELEMENT){
System.out.println("Event Type: END_ELEMENT");
}

if(event==XMLStreamConstants.NAMESPACE){
System.out.println("Event Type:NAMESPACE");
}

if(event==XMLStreamConstants.PROCESSING_INSTRUCTION){
System.out.println("Event Type: PROCESSING_INSTRUCTION");

System.out.println("PI Target:"+xmlStreamReader.getPITarget());
System.out.println("PI Data:"+xmlStreamReader.getPIData());
}
if(event==XMLStreamConstants.SPACE){
System.out.println("Event Type: SPACE");
System.out.println("Text:"+xmlStreamReader.getText());

}
}
}catch(FactoryConfigurationError e)

  {System.out.println("FactoryConfigurationError"+e.getMessage());}

catch(XMLStreamException e)
{System.out.println("XMLStreamException"+e.getMessage());}

catch(IOException e)
{System.out.println("IOException"+e.getMessage());}

}

public static void main(String[] argv){

XMLParser xmlParser=new XMLParser();
xmlParser.parseXMLDocument();

}
}



