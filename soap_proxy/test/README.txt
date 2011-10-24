SOAP to POST PROXY test suite.
-------------------------------

Prerequisites:
-------------
  The schema validation assertions for DescribeCoverage and GetCoverage
  require xmllint to be installed for validation with local schemas.  
  Be sure to have the environment variable XML_CATALOG_FILES set in .bashrc
  or otherwise available automatically when a shell starts.

  The tests assume an exoserver is configured  as the backend for soap_proxy,
  with the default test data loaded and available.  In particular the
  following data sets are required:

 MER_FRS_1P_reduced
 mosaic_MER_FRS_1PNPDE20060816_090929_000001972050_00222_23322_0058_RGB_reduced


Testing Instructions
--------------------

Install a soap_proxy service as described in INSTALL in the main soap_proxy
directory.

Use soapui (www.soapui.org) for testing.
See CONFIG_SOAPUI for instructions on using soapui with local xsd schema
definitions.

Ensure web access is available. (No matter what, sopaui seems to load some
schemas from the web. There may be a way to avoid this, but to discover
how remains TBD).  If you use a proxy to access the web set it up under
FIle -> preferences --> Proxy Settings.

Use File -> Import Project:
   load the project 'soap_proxy_test_project.xml'

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
before launching TestRunner - it reads the project from disk rather
than from memory.
