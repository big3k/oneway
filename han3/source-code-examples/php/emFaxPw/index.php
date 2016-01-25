<?php
// oneway added this
session_name("SQMSESSID");
session_start();

if (!isset($_SESSION["user_is_logged_in"])) {
// kick them out
  print "User not authorized\n";
  die();
} else {
// end of oneway's edition
?>
<html>
<head>
</head>
<body>

<h1>Instruction for Sending Fax via Email </h1>

<p>
Instructions:

<p>
1.
Type in the To: line of your email in this format: <br>
   1+areacode+localfaxnumber@bid.servequake.com <br>
   example: 12023334567@bid.servequake.com <br>

<p>
2.
Type in the Subject: line with pass code:
<img src="http://faxbeer.no-ip.info/Fax/emFaxPw/randpwd.php">

<p>
3.
Attach your content file that you need to send to your recipient. You can
only attach one file for each fax. You can embed a cover page as your
first page in the attached content file.

<p>
A confirmation email will be sent to you informing you of the result.
There could be extensive delay if there are many pending faxes in the
system.

<p>
Note: <br>
a. Supported file formats: PDF, TIF, DOC <br>
b. Use plain format email, not html or other formats. <br>
c. Supports only US numbers. <br>

</body>
</html>

<?php
}
?>

