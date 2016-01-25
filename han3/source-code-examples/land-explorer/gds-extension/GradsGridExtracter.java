package org.iges.grads.server;

import java.io.*;
import java.text.*;
import java.util.*;

import dods.dap.*;

import org.iges.util.Range;

import org.iges.anagram.*;

/** Extracts and caches metadata for a gridded GrADS dataset. */
public class GradsGridExtracter
    extends GradsExtracter {

    protected void writeSubsetInfo() 
	throws AnagramException {
	
	writeDim("lon");
	writeDim("lat");
	writeDim("lev");
	writeDim("time");
	if (gradsInfo.isDirectSubset()) {
	    saveDimsForDirectSubset();
	}
    }

    protected void load()
	throws AnagramException {

	if (debug()) log.debug(this, "loading " + 
			       gradsInfo.getDescriptorFile());
	
	String name = data.getName();
	BufferedReader r;
	try {
	    r = new BufferedReader
		(new FileReader
		    (gradsInfo.getDescriptorFile()));
	} catch (FileNotFoundException fnfe) {
	    throw new AnagramException("extraction failed for " + 
				       data.getName(), 
				       fnfe);
	}

	if (debug()) log.debug(this, "parsing " + gradsInfo.getDescriptorFile());

	String customTitle = gradsInfo.getTitle();
	
	// parsing code for CTL files
	this.variableList = new ArrayList();
	this.levelCountList = new ArrayList();
	this.descriptionList = new ArrayList();
	this.dimValues = new HashMap();
	this.minValues = new Hashtable();
	this.maxValues = new Hashtable();


	boolean bigEndian = false;	
	if (gradsInfo.isDirectSubset()) {
	    this.unsortedVariableList = new ArrayList();
	    this.unsortedLevelCountList = new ArrayList();
	}

	// begin parsing
	try {
	    boolean inVarSection = false;
	    String line;
	    String original;
	    
	    while ((original = r.readLine()) != null) {
		
		original = original.trim();
		line = original.toLowerCase();
		
		// tokenize line
		StringTokenizer st = new StringTokenizer(line, " ");
		
		try {
		    if (gradsInfo.isDirectSubset() &&
			(line.startsWith("dtype") 
			 || line.startsWith("fileheader")
			 || line.startsWith("xyheader")
			 || line.startsWith("theader"))) {
			throw new AnagramException
			    ("direct subsetting cannot be enabled if " +
			     "DTYPE, FILEHEADER, XYHEADER, or THEADER " + 
			     "are present");
		    }				
			
		    if (inVarSection) {
			if (line.startsWith("endvars")) {
			    break; // done parsing
			} 
			
			// parse a variable
			String variable = st.nextToken();

			// handle new "LongVarName=>gradsname" syntax
			// used in dtype netcdf
			int arrowIndex = variable.indexOf("=>");
			if (arrowIndex > 0) {
			    variable = variable.substring(arrowIndex + 2);
			}

			Long levelCount = Long.valueOf(st.nextToken());

			// ignore units info except for direct subset
			String units = st.nextToken(); 
			if (gradsInfo.isDirectSubset()
			    && !units.equals("99")) {
			    throw new AnagramException
				("units set to " + units + " for " + 
				 variable + "; must be '99' to enable " + 
				 " direct subsetting");
			}
			    

			// parse remainder of line as description
			// note - case of description is lost. 
			String description = "";
			while (st.hasMoreTokens()) {
			    description += st.nextToken() + " ";
			}
			// if this is the first variable with multiple levels
			// put it first in the list. 
			// the GrADS client needs to have a multi-level
			// variable be the first one if there is a mixture
			if (levelCount.longValue() > 0 && 
			    !this.gotLevels) {
			    this.variableList.add(0, variable);
			    this.levelCountList.add(0, levelCount);
			    this.descriptionList.add(0, description);
			    this.gotLevels = true;
			} else {
			    this.variableList.add(variable);
			    this.levelCountList.add(levelCount);
			    this.descriptionList.add(description);
			}
		
			if (gradsInfo.isDirectSubset()) {
			    this.unsortedVariableList.add(variable);
			    this.unsortedLevelCountList.add(levelCount);
			}
	
		    } else { // not in var section, look for general metadata
			String label = st.nextToken();

			if (gradsInfo.isDirectSubset() && 
			    label.equals("options")) {
			    while (st.hasMoreTokens()) {
				if (! st.nextToken().equals("big_endian")) {
				    throw new AnagramException
					("only 'big_endian' is allowed on "
					 + "OPTIONS line when direct " +
					 "subsetting is enabled");
				} else {
				    bigEndian = true;
				}
			    }
			}
			
			
			if (label.equals("title")) {
			    title = "";
			    while (st.hasMoreTokens()) {
				title += st.nextToken() + " ";
			    }
			    title = title.trim();
			} else if (label.equals("undef")) {
			    this.missingData = 
				Double.valueOf(st.nextToken()).doubleValue();
			    
			} else if (label.equals("xdef")) {
			    this.xSize = 
				Integer.valueOf(st.nextToken()).intValue();
			    dimValues.put("lon", loadDimValues("lon", xSize));
			    
			} else if (label.equals("ydef")) {
			    this.ySize = 
				Integer.valueOf(st.nextToken()).intValue();
			    dimValues.put("lat", loadDimValues("lat", ySize));
			    
			} else if (label.equals("zdef")) {
			    this.zSize = 
				Integer.valueOf(st.nextToken()).intValue();
			    dimValues.put("lev", loadDimValues("lev", zSize));
			    
			} else if (label.equals("tdef")) {
			    this.tSize = 
				Integer.valueOf(st.nextToken()).intValue();
			    dimValues.put("time", loadDimValues("time", tSize));
			    
			} else if (label.equals("vars")) {
			    inVarSection = true;
			}
		    } 
		} catch (NoSuchElementException nsee) {
		} 
		
	    } // end parsing loop
	    
	    if (gradsInfo.isDirectSubset() && !bigEndian) {
		throw new AnagramException
		    ("direct subsetting can only be enabled for " +
		     "big-endian data; 'OPTIONS big_endian' not found " +
		     "in CTL file");
	    }

	    if (customTitle != null) {
		title = customTitle;
	    }
	    
	    if (title.equals("")) {
		title = "no description provided";
	    }
	    
	    // put dataset name in title
	    //		title = "(dods:" + name + ") " + title;
	    
	    
	} catch (IOException ioe) {
	    throw new AnagramException("error parsing metadata for " + name);
	} finally {
	    try {
		r.close();
	    } catch (IOException ioe) {}
	}
    }
    
    

    /** Invokes GrADS to print a complete list of values for a given
     *  dimension, then parses the values into an array
     * @param dim one of "lat, "lon", "lev", "time"
     */ 
    protected double[] loadDimValues(String dim, int size) 
	throws AnagramException {
	
	if (debug()) log.debug(this, "loading " + size + " values for " + 
			       dim + " in " + data);

	double[] values = new double[size];
	File tempFile = new File(storagePrefix + "." + dim + ".output");
	Task task = tasker.task(gradsInfo.getGradsBinaryType(),
				"dimension", new String[] {
				    tempFile.getAbsolutePath(),
				    gradsInfo.getGradsArgument(),
				    dim,
				    "1",
				    String.valueOf(size) 
				});

	task.run();
	    
	// Decode and read data into array
	BufferedReader dataStream = null;
	try {
	    dataStream = new BufferedReader(new FileReader(tempFile));
	    String line; // Each line contains one value
	    for (int i = 0; i < size; i++) {
		line = dataStream.readLine(); 
		if (dim.equals("time")) {
		    values[i] = convertGradsDateToCOARDS(line);
		} else {
		    values[i] = Double.valueOf(line).doubleValue();
		}
		if (i == 0) {
		    minValues.put(dim, line);
		} 
		if (i == (size - 1)) {
		    maxValues.put(dim, line);
		}
	    }
	    
	} catch (IOException ioe) {
	    throw new AnagramException("not enough " + dim + " data in " + 
				       tempFile.getAbsolutePath());
	} catch (NumberFormatException nfe) {
	    throw new AnagramException("bad data for " + dim + " in " + 
				       tempFile.getAbsolutePath());
	} finally {
	    // Clean up
	    try {
		dataStream.close();
	    } catch (Exception e) {}
	    tempFile.delete();
	}

	return values;
    }


    /** Writes an array of coordinate data to a temporary storage
     *  file for use by the subsetting modules
     * @param dim one of "lat, "lon", "lev", "time"
     */ 
    protected void writeDim(String dim)
	throws AnagramException {
	
	File dimStorage = new File(storagePrefix + "." + dim);

	if (debug()) log.debug(this, "writing values for " + 
			       dim + " to " + dimStorage.getAbsolutePath());

	try {
	    DataOutputStream out = new DataOutputStream
		(new BufferedOutputStream
		    (new FileOutputStream
			(dimStorage)));
	    
	    double[] values = (double[])dimValues.get(dim);

	    for (int i = 0; i < values.length; i++) {
		out.writeDouble(values[i]);
	    }

	    out.close();

	} catch (IOException ioe) {
	    throw new AnagramException("error writing data for " + dim);
	}
	
    }
    
    /** Creates an in-memory object for use by the direct-subsetting 
     *  feature, which reads directly from IEEE binary datafiles instead
     *  of invoking GrADS. To make this possible it is necessary to 
     *  save the dimension sizes, and an ordered list of variable names
     *  with the number of vertical levels for each variable, so that
     *  byte offsets can be calculated properly. 
     */
    protected void saveDimsForDirectSubset() {
	if (debug()) log.debug(this, "putting dims in GradsDataInfo: \n" +
			       "\tx=" + xSize + ", y=" + ySize + 
			       ", z=" + zSize + ", t=" + tSize +"\n" + 
			       "\tvars=" + unsortedVariableList + "\n" +
			       "\tlevs=" + unsortedLevelCountList);
	gradsInfo.setCTL(xSize, ySize, zSize, tSize,
			 unsortedVariableList,
			 unsortedLevelCountList);
	server.getCatalog().saveCatalogToStore();
    }

	

    public void writeDAS() 
	throws AnagramException {
	
	String dasStorage = storagePrefix + ".das";
	if (debug()) log.debug(this, "writing das to " + dasStorage);

	FileWriter output;
	try {
	    output = new FileWriter(dasStorage);
	} catch (IOException ioe) {
	    throw new AnagramException("error writing das for " + 
				       data.getName());
	}
	PrintWriter das = new PrintWriter(output);
	
	// write global metadata including bounds
	das.println("Attributes {\n" +
		    "    NC_GLOBAL {\n" +
		    "        String title \"" + title + "\";\n" +
		    "        String Conventions \"COARDS\";\n" +
		    "        String history \"" + getHistoryString() + "\";");
	if (gradsInfo.getDocURL() != null) {
	    das.println("        String documentation \"" + 
			gradsInfo.getDocURL() + "\";");
	}
	printDODSAttributes(das, "");
	das.println("    }\n" +
		    "    lat {\n" +
		    "        String units \"degrees_north\";\n" +
		    "        String long_name \"latitude\";\n" +
		    "        Float64 minimum " + this.minValues.get("lat") + ";\n" +
		    "        Float64 maximum " +  this.maxValues.get("lat") + ";\n");
	printDODSAttributes(das, "lat");
	das.println("    }\n" +
		    "    lon {\n" +
		    "        String units \"degrees_east\";\n" +
		    "        String long_name \"longitude\";\n" +
		    "        Float64 minimum " + this.minValues.get("lon") + ";\n" +
		    "        Float64 maximum " + this.maxValues.get("lon") + ";\n");
	printDODSAttributes(das, "lon");
	das.println("    }\n" +
		    "    time {\n" +
		    "        String units \"days since 1-1-1 00:00:0.0\";\n" +
		    "        String long_name \"Time\";\n" +
		    "        String minimum \"" + this.minValues.get("time") + "\";\n" +
			"        String maximum \"" + this.maxValues.get("time") + "\";\n");
	printDODSAttributes(das, "time");
	das.println("    }");
	if (this.gotLevels) {
	    das.println(
			"    lev {\n" +
			"        String units \"millibar\";\n" +
			"        String long_name \"altitude\";\n" +
			"        Float64 minimum " + this.minValues.get("lev") + ";\n" +
			"        Float64 maximum " + this.maxValues.get("lev") + ";\n");
	printDODSAttributes(das, "lev");
	das.println("    }");
	    }
	
	// write metadata for each variable
	for (int i = 0; i < this.variableList.size(); i++) {
	    String name = (String)this.variableList.get(i);
	    das.println(
			"    " + name + " {\n" +
			"        Float32 _FillValue " + this.missingData + ";\n" +
			"        Float32 missing_value " + this.missingData + ";\n" +
			"        String long_name \"" + this.descriptionList.get(i) + "\";\n");
	    printDODSAttributes(das, name);
	    das.println("    }");
	}
	das.println("}");
	das.close();
	
	try {
	    DAS finalDAS = new DAS();
	    finalDAS.parse(new BufferedInputStream
			   (new FileInputStream(dasStorage)));
	    
	    File userDASFile = 
		((GradsDataInfo)data.getToolInfo()).getUserDAS();
	    try {
		if (userDASFile != null && userDASFile.exists()) {
		    finalDAS.parse(new BufferedInputStream
				       (new FileInputStream(userDASFile)));
		}
	    } catch (FileNotFoundException e) {
		throw new AnagramException("user DAS not found " +  
					   userDASFile);
		
	    } catch (Exception e) {
		throw new AnagramException("error parsing user DAS", 
					   e);
		
	    }
	    output = new FileWriter(dasStorage);
	    das = new PrintWriter(output);
	    finalDAS.print(das);
	} catch (IOException ioe) {
	    throw new AnagramException("error writing das for " + 
				       data.getName());
	} catch (Exception e) {
	    throw new AnagramException("error in DAS formatting; ", 
				       e);
	    
	} finally {
	    das.close();
	}
    }
    
    
    
    public void writeDDS() 
	throws AnagramException {
	
	String ddsStorage = storagePrefix + ".dds";
	if (debug()) log.debug(this, "writing dds to " + ddsStorage);

	FileWriter output;
	try {
	    output = new FileWriter(ddsStorage);
	} catch (IOException ioe) {
	    throw new AnagramException("error writing dds for " + 
				       data.getName());
	}
	
	PrintWriter dds = new PrintWriter(output);
	
	dds.println("Dataset {");
	// write metadata for each variable
	for (int i = 0; i < this.variableList.size(); i++) { 
	    if (((Long)this.levelCountList.get(i)).longValue() > 0) {
		dds.println("  Grid {\n" + 
			    "    ARRAY:\n" + 
			    "      Float32 " + this.variableList.get(i) +
			    "[time = "+ this.tSize + 
			    "][lev = " + this.zSize + 
			    "][lat = " + this.ySize + 
			    "][lon = " + this.xSize + "];\n" + 
			    "    MAPS:\n" + 
			    "      Float64 time[time = " + this.tSize + "];\n" + 
			    "      Float64 lev[lev = " + this.zSize + "];\n" + 
			    "      Float64 lat[lat = " + this.ySize + "];\n" + 
			    "      Float64 lon[lon = " + this.xSize + "];\n" + 
			    "  } " + this.variableList.get(i) + ";");
	    } else { 
		dds.println("  Grid {\n" + 
			    "    ARRAY:\n" + 
			    "      Float32 " + this.variableList.get(i) +
			    "[time = "+ this.tSize + 
			    "][lat = " + this.ySize + 
			    "][lon = " + this.xSize + "];\n" + 
			    "    MAPS:\n" + 
			    "      Float64 time[time = " + this.tSize + "];\n" + 
			    "      Float64 lat[lat = " + this.ySize + "];\n" + 
			    "      Float64 lon[lon = " + this.xSize + "];\n" + 
			    "  } " + this.variableList.get(i) + ";");
	    }
	}
	
	
	// write global dimension information. needed for netCDF client. 
	if (this.gotLevels) {
	    dds.println("  Float64 time[time = " + this.tSize + "];\n" + 
			"  Float64 lev[lev = " + this.zSize + "];\n" + 
			"  Float64 lat[lat = " + this.ySize + "];\n" + 
			"  Float64 lon[lon = " + this.xSize + "];"); 
	} else {
	    dds.println("  Float64 time[time = " + this.tSize + "];\n" + 
			"  Float64 lat[lat = " + this.ySize + "];\n" + 
			"  Float64 lon[lon = " + this.xSize + "];"); 
	}
	
	dds.println("} " + gradsInfo.getDODSName() + ";");
	dds.close();
	
    }
    
    public void writeWebSummary()
	throws AnagramException {

	String infoStorage = storagePrefix + ".info";
	if (debug()) log.debug(this, "writing web info to " + infoStorage);

	FileWriter output;
	try {
	    output = new FileWriter(infoStorage);
	} catch (IOException ioe) {
	    throw new AnagramException("error writing web info for " + 
				       data.getName());
	}

	PrintWriter info = new PrintWriter(output);

	info.print("<table>\n");
	info.print("   <tbody>\n");
	info.print("     <tr>\n");
	info.print("       <td valign=\"Bottom\" colspan=\"2\">\n");
	info.print("<b>Description:</b><br>\n");
	info.print("       </td>\n");
	info.print("       <td valign=\"Bottom\" colspan=\"2\">\n");
	info.print(data.getDescription());
	info.print("<br>\n");
	info.print("       </td>\n");
	info.print("     </tr>\n");
	info.print("     <tr>\n");
	info.print("       <td valign=\"Bottom\" colspan=\"2\">\n");
	info.print("<b>Documentation:</b>\n");
	info.print("       </td>\n");
	info.print("       <td valign=\"Bottom\" colspan=\"2\">");
	if (gradsInfo.getDocURL() != null) {
	    info.print("<a href=\"\n");
	    info.print(gradsInfo.getDocURL());
	    info.print("\">\n");
	    info.print(gradsInfo.getDocURL());
	} else {
	    info.print("none provided");
	}
	info.print("</a>\n");
	info.print("       <br>\n");
	info.print("       </td>\n");
	info.print("     </tr>\n");

	printDim(info, "lon", "Longitude", "&deg;E", "&deg;", xSize);
	printDim(info, "lat", "Latitude", "&deg;N", "&deg;", ySize);
	printDim(info, "lev", "Altitude", "", "", zSize);
	printDim(info, "time", "Time", "", "", tSize);


	info.print("     <tr>\n");
	info.print("       <td valign=\"Bottom\" colspan=\"2\"><b>Variables:</b><br>\n");
	info.print("       </td>\n");
	info.print("       <td colspan=\"4\"><b>(total of ");
	info.print(variableList.size());
	info.print(")</b><br>\n");
	info.print("       </td>\n");
	info.print("     </tr>\n");


/** Yudong Tian: added link to visualization cgi. 4/13/2004 **/
	Iterator varIt = variableList.iterator();
	Iterator descIt = descriptionList.iterator();
	while (varIt.hasNext()) {
	    info.print("     <tr>\n");
	    info.print("       <td valign=\"Bottom\">&nbsp;<br>\n");
	    info.print("       </td>\n");
	    info.print("       <td><b>");
            String lvar = (String) varIt.next();
	    info.print(lvar); 
	    info.print("</b><br>\n");
	    info.print("       </td>\n");
	    info.print("       <td colspan=\"4\">");
	    info.print(descIt.next());
	    info.print("<br>\n");
	    info.print("       </td>\n");
	    info.print("       <td colspan=\"4\">");
	    info.print("<a href=\"http://lisdata.gsfc.nasa.gov/las-bin/gdsplot.pl?var=" +
                        lvar + "&url=http://lisdata.gsfc.nasa.gov:9090/dods/" +
                        gradsInfo.getDODSName() + 
                        "&W=" + minValues.get("lon") +
                        "&E=" + maxValues.get("lon") +
                        "&S=" + minValues.get("lat") +
                        "&N=" + maxValues.get("lat") +
                        "&Z=" + zSize +
                        "&T=" + tSize +
                        "\"><b> [Visualize!]</b><a/>");
	    info.print("<br>\n");
	    info.print("       </td>\n");
	    info.print("     </tr>\n");
	}

	info.print("   \n");
	info.print("  </tbody> \n");
	info.print("</table>\n");


	info.close();
	
    }

    /** Helper for writeWebInfo() */
    protected void printDim(PrintWriter info, 
			    String dim, 
			    String longName, 
			    String units,
			    String resUnits,
			    int size) {

	info.print("     <tr>\n");
	info.print("       <td valign=\"Bottom\" colspan=\"2\">");
	info.print("<b>");
	info.print(longName);
	info.print(":</b></td>\n");
	info.print("       <td>       \n");
	info.print("      <div align=\"Center\">");
	info.print(minValues.get(dim));
	info.print(units);
	info.print(" to ");
	info.print(maxValues.get(dim));
	info.print(units);
	info.print("</div>\n");
	info.print("       </td>\n");
	info.print("       <td>&nbsp;(");
	info.print(size);
	info.print(" points");
	if (size > 1) {
	    info.print(", avg. res. ");
	    info.print((dim.equals("time")) ? calcTimeRes() : 
		       calcSpaceRes(dim, resUnits));
	}
	info.print(")\n");
	info.print("       </td>\n");
	info.print("     </tr>\n");
    }

    /** Helper for writeWebInfo() */
    protected String calcSpaceRes(String dim, String units) {
	double[] dimArray = (double[])dimValues.get(dim);
	double min = dimArray[0];
	double max = dimArray[dimArray.length - 1];
	double res = Math.abs((max - min) / (dimArray.length - 1));
	return doubleFormat.format(res) + units;
    }

    /** Helper for writeWebInfo() */
    protected String calcTimeRes() {
	double[] dimArray = (double[])(dimValues.get("time"));
	double min = dimArray[0];
	double max = dimArray[dimArray.length - 1];
	double res = Math.abs((max - min) / (dimArray.length - 1));

	double numUnits = 0;
	String unit = null;
	for (int i = 0; i < timeIntervalNames.length; i++) {
	    unit = timeIntervalNames[i];
	    numUnits = res * timeIntervalFreq[i];
	    if (numUnits >= 1.0) {
		break;
	    }
	}
	return doubleFormat.format(numUnits) + " " + unit;
    }

    protected final static String[] timeIntervalNames = {
	"days", "hours", "minutes", "seconds"
    };

    protected final static double[] timeIntervalFreq = {
	1.0, 
	24.0,
	24.0 * 60.0,
	24.0 * 60.0 * 60.0
    };

    protected final static DecimalFormat doubleFormat =
	new DecimalFormat("0.0#");


    /** Converts a GrADS date, a string with format yyyy:M:d:H or 
     * yyyy:M:d:H:m, to a udunits-compatible COARDS date, which is a 
     * floating point number 
     * in units of days since Jan 01, 0001. 
     */
    protected double convertGradsDateToCOARDS(String dateString) {
    
	Date parsedDate = Range.parseGradsFormat(dateString);

	// Set origin date to 01/01/0001, 12am GMT
	Calendar origin = new GregorianCalendar(1, // year
						0, // month, 0-based
						1  // day
						);
	origin.setTimeZone(TimeZone.getTimeZone("GMT"));
	// Calculate difference in milliseconds 
	long difference = parsedDate.getTime() - origin.getTime().getTime();

	// Convert milliseconds to days
	double coardsDate = (double)difference / (double)(1000 * 60 * 60 * 24);
	   
	return coardsDate;
    }


    String title;
    
    /** size of grid dimension for this dataset */
    int xSize;
    /** size of grid dimension for this dataset */
    int ySize;
    /** size of grid dimension for this dataset */
    int zSize;
    /** size of grid dimension for this dataset */
    int tSize;
    
    double missingData;
    
    boolean gotLevels;
    
    ArrayList variableList;
    ArrayList levelCountList;
    ArrayList descriptionList;

    ArrayList unsortedVariableList;
    ArrayList unsortedLevelCountList;

    Map dimValues;
    
    /** minimum values, in string form, for each dimension; 
     *  indexed by dimension name */
    Hashtable minValues;
    /** maximum values, in string form, for each dimension; 
     *  indexed by dimension name */
    Hashtable maxValues;
}

