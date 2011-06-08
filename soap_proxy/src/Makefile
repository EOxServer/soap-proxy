#
#  Soap Proxy.
#  Copyright (c) 2011, ANF DATA Spol. s r.o.
#  See the file soap_proxy/LICENSE for terms of use.
#  See README and INSTALL in the same directory for usage.
#  The file sp_svc.c contains the main entry point of execution.
#
#  There is absolutely no warranty of any kind for this sofware.
#
#  AXIS2C_HOME must be set in the environment before running make.
#

C_FLAGS = -fPIC -shared
I_FLAGS = -I${AXIS2C_HOME}/include/axis2-1.6.0/
L_FLAGS = -L${AXIS2C_HOME}/lib -laxutil -laxis2_axiom -laxis2_parser \
  -laxis2_engine -lpthread -laxis2_http_sender -laxis2_http_receiver

SP_INCLUDES = soap_proxy.h sp_svc.h
SP_SOURCES = sp_ctype.c sp_svc.c sp_dispatch.c sp_exec_ms.c \
             sp_props.c sp_util.c sp_image.c sp_fault.c \
             sp_wcs20.c sp_wcs11.c sp_ms_version.c sp_process_mime.c
             
SERVICE_NAME = soapProxy
SERVICE_LIB  = lib${SERVICE_NAME}.so
SERVICE_PATH = ${AXIS2C_HOME}/services/${SERVICE_NAME}

.PHONY: 	check_env all

all:		check_env ${SERVICE_LIB}

install:	inst

check_env:	
ifndef AXIS2C_HOME
	@echo '  *** AXIS2C_HOME is not set.' ; exit 1
endif


${SERVICE_LIB}:	${SP_INCLUDES} ${SP_SOURCES}
	gcc ${C_FLAGS} -o $@ ${I_FLAGS} ${L_FLAGS}  ${SP_SOURCES}

${SERVICE_PATH}/${SERVICE_LIB}:  ${SERVICE_LIB}
	cp $< ${SERVICE_PATH}

inst:	check_env ${SERVICE_PATH}/${SERVICE_LIB}
	
