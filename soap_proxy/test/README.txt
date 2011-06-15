SOAP to POST PROXY test suite.
-------------------------------

Prerequisites:
-------------
  The schema validation assertions for DescribeCoverage and GetCoverage
  require xmllint to be installed for validation with local schemas.  
  Be sure to have the environment variable XML_CATALOG_FILES set in .bashrc
  or otherwise available automatically when a shell starts.


Testing Instructions
--------------------

Install a soap_proxy service as described in INSTALL in the main soap_proxy
directory.

Use soapui (www.soapui.org) for testing.

Ensure web access is available. (No matter what sopaui seems to load some
schemas from the web. There may be a way to avoid this, but to discover
how remains TBD).  If you use a proxy to access the web set it up under
FIle -> preferences --> Proxy Settings.

Create a new project (CRTL-N),  load the wsdl (under "Initial WSD/WADL:").
  --> After start-up, create a new project. 
      Use the wsdl supplied by the soapProxy service:
                http://your.server/sp_wcs?wsdl
      It is also possible to use either a local wsdl (e.g. from the services
      directory - but then the port binding address location must be edited to
      refer to a working WCS SOAP service)
  --> Uncheck 'Create Requests:'  (i.e. do not create sample requests for the
      operations).

Right click the just-created project to get a context menu, choose
'Import Test Suite'.

Navigate to locate and load 's2p_TestSuite001.xml'.

Once the test suite is loaded, right-click the test suite for a 
context menu -> Launch TestRunner --> [ Launch ]

A successful run ends with a dialog 'Execution finished successfully'.

Note: if you modify the test suite, you must first save the project 
before launching TestRunner - it reads the project from disk rather
than from memory.
