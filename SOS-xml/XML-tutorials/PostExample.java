
import org.apache.commons.httpclient.*;
import org.apache.commons.httpclient.methods.*;

public class PostExample {
 public static void main( String[] args ) {
   String url = "http://search.yahoo.com/search";
   try {
    HttpClient client = new HttpClient();
    PostMethod method = new PostMethod( url );

	 // Configure the form parameters
	 method.addParameter( "p", "Java" );

	 // Execute the POST method
    int statusCode = client.executeMethod( method );
    if( statusCode != -1 ) {
      String contents = method.getResponseBodyAsString();
      method.releaseConnection();
      System.out.println( contents );
    }
   }
   catch( Exception e ) {
    e.printStackTrace();
   }
 }
}
