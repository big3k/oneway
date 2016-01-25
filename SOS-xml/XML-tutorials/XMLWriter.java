
import javax.xml.stream.*;
import javax.xml.stream.events.*;
import javax.xml.stream.XMLOutputFactory;
import java.io.*;

public class XMLWriter{

public void generateXMLDocument(){
try{

XMLOutputFactory outputFactory=XMLOutputFactory.newInstance();
FileWriter output=new FileWriter(new File("catalog.xml"));

XMLStreamWriter xmlStreamWriter=outputFactory.createXMLStreamWriter(output);
xmlStreamWriter.writeStartDocument("UTF-8","1.0");
xmlStreamWriter.writeComment("A OReilly Journal Catalog");
xmlStreamWriter.writeProcessingInstruction("catalog","journal='OReilly'");
xmlStreamWriter.writeStartElement("journal","catalog","http://OnJava.com/Journal");
xmlStreamWriter.writeNamespace("journal","http://OnJava.com/Journal");
xmlStreamWriter.writeNamespace("xsi","http://www.w3.org/2001/XMLSchema-instance");
xmlStreamWriter.writeAttribute("xsi:noNamespaceSchemaLocation","file://c:/Schemas/catalog.xsd");
xmlStreamWriter.writeAttribute("publisher","OReilly");

xmlStreamWriter.writeStartElement("journal","journal","http://OnJava.com/Journal");
xmlStreamWriter.writeAttribute("date","January 2004");
xmlStreamWriter.writeAttribute("title","ONJava");
xmlStreamWriter.writeStartElement("article");
xmlStreamWriter.writeStartElement("title");
xmlStreamWriter.writeCharacters("Data Binding with XMLBeans");
xmlStreamWriter.writeEndElement();
xmlStreamWriter.writeStartElement("author");
xmlStreamWriter.writeCharacters("Daniel Steinberg");
xmlStreamWriter.writeEndElement();
xmlStreamWriter.writeEndElement();
xmlStreamWriter.writeEndElement();

xmlStreamWriter.writeStartElement("journal","journal","http://OnJava.com/Journal");
xmlStreamWriter.writeAttribute("date","July 2004");
xmlStreamWriter.writeAttribute("title","java.net");
xmlStreamWriter.writeStartElement("article");
xmlStreamWriter.writeStartElement("title");
xmlStreamWriter.writeCharacters("Hibernate: A Developer's Notebook");
xmlStreamWriter.writeEndElement();
xmlStreamWriter.writeStartElement("author");
xmlStreamWriter.writeCharacters("James Elliott");
xmlStreamWriter.writeEndElement();
xmlStreamWriter.writeEndElement();
xmlStreamWriter.writeEndElement();
xmlStreamWriter.writeEndElement();

xmlStreamWriter.flush();
xmlStreamWriter.close();

}catch(FactoryConfigurationError e){}
catch(XMLStreamException e){}
catch(IOException e){}

}
public static void main(String[] argv){

XMLWriter xmlWriter=new XMLWriter();
xmlWriter.generateXMLDocument();
}
}

