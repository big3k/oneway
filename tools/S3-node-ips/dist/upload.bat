rem The first parameter "fgdns" is the S3 bucket name.

java -cp classes;lib\aws-java-sdk-1.1.0.jar;lib\commons-codec-1.3.jar;lib\commons-httpclient-3.0.1.jar;lib\commons-io-1.4.jar;lib\commons-logging-1.1.1.jar Uploader fgdns upload.properties

echo off

echo /////////////////////////////////////////////////////////////////////////////
echo // You can access https://s3.amazonaws.com/fgdns/1
echo // https://s3.amazonaws.com/fgdns/2 and 
echo // https://s3.amazonaws.com/fgdns/4.
echo //
echo // You can modify upload.properties to upload more files.
echo /////////////////////////////////////////////////////////////////////////////