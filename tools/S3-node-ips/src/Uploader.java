

/*
 * Copyright 2010 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */
import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.io.Writer;
import java.util.Properties;
import java.util.Set;
import java.util.UUID;

import com.amazonaws.AmazonClientException;
import com.amazonaws.AmazonServiceException;
import com.amazonaws.auth.PropertiesCredentials;
import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.AmazonS3Client;
import com.amazonaws.services.s3.model.AccessControlList;
import com.amazonaws.services.s3.model.GetObjectRequest;
import com.amazonaws.services.s3.model.Grantee;
import com.amazonaws.services.s3.model.GroupGrantee;
import com.amazonaws.services.s3.model.Permission;
import com.amazonaws.services.s3.model.ListObjectsRequest;
import com.amazonaws.services.s3.model.PutObjectRequest;
import com.amazonaws.services.s3.model.Bucket;
import com.amazonaws.services.s3.model.S3Object;
import com.amazonaws.services.s3.model.ObjectListing;
import com.amazonaws.services.s3.model.S3ObjectSummary;

/**
 * This sample demonstrates how to make basic requests to Amazon S3 using the
 * AWS SDK for Java.
 * <p>
 * <b>Prerequisites:</b> You must have a valid Amazon Web Services developer
 * account, and be signed up to use Amazon S3. For more information on Amazon
 * S3, see http://aws.amazon.com/s3.
 * <p>
 * <b>Important:</b> Be sure to fill in your AWS access credentials in the
 * AwsCredentials.properties file before you try to run this sample.
 * http://aws.amazon.com/security-credentials
 */
public class Uploader {

	public static void main(String[] args) throws IOException {
		if (args.length != 2) {
			System.out.println("Usage: bucket_name properties_file");
			
			System.out.println("Please specify the bucket name and the properties file to process.");
			System.out.println("The properties file contains the key for S3 as the key and absolute file path as the value.");
			System.exit(0);
		}
		/*
		 * Important: Be sure to fill in your AWS access credentials in the
		 * AwsCredentials.properties file before you try to run this sample.
		 * http://aws.amazon.com/security-credentials
		 */
		AmazonS3 s3 = new AmazonS3Client(
				new PropertiesCredentials(Uploader.class
						.getResourceAsStream("AwsCredentials.properties")));

		String bucketName = args[0];

		System.out.println("===========================================");
		System.out.println("Getting Started with Amazon S3");
		System.out.println("===========================================\n");

		Properties files = new Properties();
		InputStream in = new FileInputStream(new File(args[1]));
		files.load(in);
		in.close();

		Set keys = files.keySet();
		for (Object key : keys) {
			uploadFile(s3, bucketName, (String) key,
					new File(files.getProperty((String) key)));
		}

	}

	private static void uploadFile(AmazonS3 s3, String bucketName, String key,
			File file) {
		if (!file.exists()) {
			System.out.println("File does not exist: " + file.getAbsolutePath());
			return;
		}
		/*
		 * Upload an object to your bucket - You can easily upload a file to S3,
		 * or upload directly an InputStream if you know the length of the data
		 * in the stream. You can also specify your own metadata when uploading
		 * to S3, which allows you set a variety of options like content-type
		 * and content-encoding, plus additional metadata specific to your
		 * applications.
		 */
		try {
			System.out.println(file.getAbsolutePath() + " ---> " + key + "\n");
			s3.putObject(new PutObjectRequest(bucketName, key, file));
			// Change permissions. Grant all users the read permission.
			AccessControlList acl = s3.getObjectAcl(bucketName, key);
			Permission permission = Permission.Read;
			Grantee grantee = GroupGrantee.AllUsers;
			acl.grantPermission(grantee, permission);
			s3.setObjectAcl(bucketName, key, acl);
		} catch (AmazonServiceException ase) {
			System.out
					.println("Caught an AmazonServiceException, which means your request made it "
							+ "to Amazon S3, but was rejected with an error response for some reason.");
			System.out.println("Error Message:    " + ase.getMessage());
			System.out.println("HTTP Status Code: " + ase.getStatusCode());
			System.out.println("AWS Error Code:   " + ase.getErrorCode());
			System.out.println("Error Type:       " + ase.getErrorType());
			System.out.println("Request ID:       " + ase.getRequestId());
		} catch (AmazonClientException ace) {
			System.out
					.println("Caught an AmazonClientException, which means the client encountered "
							+ "a serious internal problem while trying to communicate with S3, "
							+ "such as not being able to access the network.");
			System.out.println("Error Message: " + ace.getMessage());
		}

	}
}
