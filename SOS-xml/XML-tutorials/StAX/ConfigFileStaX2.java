
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.InputStream;

import javax.xml.stream.XMLEventReader;
import javax.xml.stream.XMLInputFactory;
import javax.xml.stream.XMLStreamException;
import javax.xml.stream.events.XMLEvent;

public class ConfigFileStaX2 {

    private String configFile;

    public void setFile(String configFile) {
	this.configFile = configFile;
    }

    public void readConfig() {

	try {
	    // First create a new XMLInputFactory
	    XMLInputFactory inputFactory = XMLInputFactory.newInstance();
	    // Setup a new eventReader
	    InputStream in = new FileInputStream(configFile);
	    XMLEventReader eventReader = inputFactory.createXMLEventReader(in);
	    // Read the XML document
	    while (eventReader.hasNext()) {

		XMLEvent event = eventReader.nextEvent();

		if (event.isStartElement()) {
		    if (event.asStartElement().getName().getLocalPart() == ("mode")) {
			event = eventReader.nextEvent();
			System.out.println(event.asCharacters().getData());
			continue;
		    }
		    if (event.asStartElement().getName().getLocalPart() == ("unit")) {
			event = eventReader.nextEvent();
			System.out.println(event.asCharacters().getData());
			continue;
		    }

		    if (event.asStartElement().getName().getLocalPart() == ("current")) {
			event = eventReader.nextEvent();
			System.out.println(event.asCharacters().getData());
			continue;
		    }

		    if (event.asStartElement().getName().getLocalPart() == ("values")) {
			event = eventReader.nextEvent();
			System.out.println(event.asCharacters().getData());
			continue;
		    }
		}
	    }
	} catch (FileNotFoundException e) {
	    e.printStackTrace();
	} catch (XMLStreamException e) {
	    e.printStackTrace();
	}
    }

    public static void main(String args[]) {
	ConfigFileStaX2 read = new ConfigFileStaX2();
	read.setFile("config.xml");
	read.readConfig();
    }
}



