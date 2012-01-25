#
# spec file for package eo_soap_proxy
#
# Copyright (c) 2012 ANF Data spol. s r.o., Prague CZ.
#
# The spec is loosely based on original spec file for axis2c, which was
# Copyright (c) 2010 SUSE LINUX Products GmbH, Nuernberg, Germany.
#
# All modifications and additions to the file contributed by third parties
# remain the property of their copyright owners, unless otherwise agreed
# upon. The license for this file, and modifications and additions to the
# file, is the same license as eoxserver/soap_proxy, which is the 
# EOxServer-SoapProxy Open License (a MIT-style license).
#

Name:           eo_soap_proxy
Version:        1.0.0
Release:        1
Summary:        Soap proxy for EOxServer
Group:          Applications/Internet
License:        EOxServer-SoapProxy Open License (based on MIT).

URL:            http://eoxserver.org/
# svn co http://eoxserver.org/svn/trunk/soap_proxy
# mv soap_proxy eo_soap_proxy-1.0.0
# tar --exclude-vcs --exclude=eo_soap_proxy-1.0.0/test \
#     -czf eo_soap_proxy-1.0.0.tgz eo_soap_proxy-1.0.0
Source0:        eo_soap_proxy-1.0.0.tgz

%define major 0
%define libname libsoapProxyEo.0
%define logdir %{_var}/log/axis2
%define httpd_confdir %{_sysconfdir}/httpd/conf.d
%define httpd_conffile 031_eo_soap_proxy.conf
%define service_name soapProxy
%define tteconfdir /usr/local/etc/eo_soap_proxy
%define sp_eo_uri sp_eowcs

BuildRequires:  axis2c_eo

Requires:       axis2c_eo

Provides:       %{libname} = %{version}-%{release}
# BuildArch:      noarch

BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-buildroot

%description
Soap-proxy is an implementation of OGC 09-149 Web Coverage Service 2.0 Interface
Standard - XML/SOAP Protocol Binding Extension.
Initially soap_proxy supports WCS 2.0.
Soap_proxy is implemented as a Web Service using the Axis2/C framework [AXIS],
plugged into a standard Apache HTTP server via its mod_axis2 module.

%prep
%setup -q -n soap_proxy-src-%{version}

%build

make -C src SERVICE_LIB=%{libname} \
    SOAP_PROXY_NOCONFIG=1 \
    IINCDIR=`pkg-config --cflags-only-I axis2c` \
    LLIBDIR=`pkg-config --libs-only-L axis2c` \
    AXIS2C_HOME=/tmp

%install

mkdir -p %{buildroot}%{_libdir}
cp src/%{libname} %{buildroot}%{_libdir}

mkdir -p %{buildroot}%{httpd_confdir}
cp service/soap_proxy_httpd.conf %{buildroot}%{httpd_confdir}/%{httpd_conffile}

mkdir -p %{buildroot}%{tteconfdir}
cp service/EOxServer_service.xml %{buildroot}%{tteconfdir}
cp service/SP_SERVICE_NAME.wsdl %{buildroot}%{tteconfdir}

%clean
rm -rf %{buildroot}

%files 
%defattr(-,root,root)
%doc LICENSE
%dir %{_libdir}
%{_libdir}/%{libname}
%dir %{httpd_confdir}
%config(noreplace) %{httpd_confdir}/%{httpd_conffile}
%dir %{tteconfdir}
%config(noreplace) %{tteconfdir}/EOxServer_service.xml
%config(noreplace) %{tteconfdir}/SP_SERVICE_NAME.wsdl

%post
sed -i \
    's^__SP_SERVICE_NAME__^%{service_name}^ ; s^__SP_URI__^%{sp_eo_uri}^' \
    %{httpd_confdir}/%{httpd_conffile}

AXIS2C_HOME=`pkg-config --variable=axis2c_home axis2c`
SP_SVC_HOME=${AXIS2C_HOME}/services/%{service_name}
mkdir -p ${SP_SVC_HOME}

SP_SVC_URI='http://'`hostname -f`'/%{sp_eo_uri}'

SP_SERVICES_XML=${SP_SVC_HOME}/services.xml
if [ -e  ${SP_SERVICES_XML} ] ; then
  echo "Warning: $SP_SERVICES_XML exists, not updating."
else
  cp %{tteconfdir}/EOxServer_service.xml ${SP_SERVICES_XML}
  sed -i \
      's^r name="ServiceClass" locked="xsd:false">__SP_SERVICE_NAME__^r name="ServiceClass" locked="xsd:false">%{service_name}^;
      s^r name="SOAPOperationsURL">http://SERVER_UNDEFINED/service^r name="SOAPOperationsURL">'$SP_SVC_URI'^' \
      ${SP_SERVICES_XML}
fi

SP_SERVICE_WSDL=${SP_SVC_HOME}/%{service_name}.wsdl
if [ -e  ${SP_SERVICE_WSDL} ] ; then
  echo "Warning: $SP_SERVICE_WSDL exists, not updating."
else
    cp %{tteconfdir}/SP_SERVICE_NAME.wsdl ${SP_SERVICE_WSDL}
    sed -i \
        's^<soap:address location="http://www.your.server/sp_wcs"^<soap:address location="'$SP_SVC_URI'"^' \
        ${SP_SERVICE_WSDL}
fi

ln -s %{_libdir}/%{libname} ${SP_SVC_HOME}/lib%{service_name}.so

/sbin/ldconfig

%postun -p /sbin/ldconfig

%changelog
* Tue Jan 24 2012 Milan Novacek 1.0.0
- Initial version.
