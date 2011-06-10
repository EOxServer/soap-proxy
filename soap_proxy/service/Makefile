#
#  Soap Proxy.
#  Copyright (c) 2011, ANF DATA Spol. s r.o.
#  See the file soap_proxy/LICENSE for terms of use.
#  See README and INSTALL in the same directory for usage.
#
#  There is absolutely no warranty of any kind for this sofware.
#
#  AXIS2C_HOME must be set in the environment before running make.
#
#  This makefile is intended to be run recursively from a make command
#    invoked in the ../src directory using the main Makefile there.
#

CONFIG_FILES    = services.xml soapProxy.wsdl

.PHONY: 	inst_config

.SILENT:	inst_config

inst_config:	${CONFIG_FILES}
	for xx in $? ; do \
	  if [ $$xx -nt ${SERVICE_PATH}/$$xx ] ; then \
	    echo "Installing  $$xx" ;\
	    cp  $$xx ${SERVICE_PATH} ;\
	  fi \
	done


