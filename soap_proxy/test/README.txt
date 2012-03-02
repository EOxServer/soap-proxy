SOAP to POST PROXY test suite.
-------------------------------

TD005_TS_puck.xml is for use with the test data and exoserver
                  configuration on the server puck.

TD005_TS_titania.xml was used during development on the server
                     titania. The test data here may not be available
                     for the general public.

Prerequisites:
-------------
  Web access is required.

  The tests assume an exoserver is configured  as the backend for soap_proxy,
  with the default test data loaded and available.  In particular the
  following data sets are required:

  on puck:
    envisat
    ASA_IMG_1PNDPA20111022_092031_000000163107_00424_50438_0034

  on titania:
   MER_FRS_1P_reduced
   mosaic_MER_FRS_1PNPDE20060816_090929_000001972050_00222_23322_0058_RGB_reduced


Set Up
------

Install a soap_proxy service as described in INSTALL in the main soap_proxy
directory.

Use soapui (www.soapui.org) for testing.

It is necessary to configure a local proxy for soapui, in order for soapui
to be able to access local schema definition files. The reason is that
some of the schemas for Earth Observation are not yet available from the OGC
schema repository on the web.

Follow the procedure in the file CONFIG_SOAPUI to set this up.

If you use a proxy to access the web then you can confgure acces to other
sites in the vhosts config file, but the details are beyond the scope of this
document.  An aternative is to use the local soap proxy (as per CONFIG_SOAPUI)
to set up the project at the start (New Project), where the use of the local
schemas is required. After this has been set up, change the proxy from the
local proxy to your proxy used to access the web normally for the rest of the
test steps. The proxy is set up under:
  FIle -> preferences --> Proxy Settings.


Loading the Test Suite
----------------------

Start with an emtpy soapUI.

Create a new project with 
   File->New Soapui project

The name is not significant, use e.g. 'soapproxy'.
For the wsdl file use either a local wsdl that you have configured, or:
  http://puck.eox.at/sp_eowcs?wsdl

Right-click on the sopapproxy project to get a context-menu, and use 
  Import Test Suite
     Import the test suite 'TD005_TS_puck.xml'

Optional if you have set up the service for a custom url (i.e. in
the httpd configaration file you have set ProxyPass for  soapProxy to
someting other than '/sp_eowcs'):
  Open each test step (one for each of the three test cases) and set the
  endpoint URL at the top of its editor window to the URL of your service. Be
  sure to close the editor windows and save the project.


Once the test suite is loaded, right-click the test
suite for a context menu -> Launch TestRunner --> [ Launch ]

A successful run ends with a dialog 'Execution finished successfully'.

Note: if you modify the test suite, you must first save the project 
before launching TestRunner - TesRunner reads the project from disk rather
than from the memory image in running soapUI instance.
