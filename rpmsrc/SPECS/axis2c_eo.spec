#
# spec file for package axis2c_eo-1.6.0.
#
# Copyright (c) 2012 ANF Data spol. s r.o., Prague CZ.
#
# Loosely based on original spec file for axis2c, which was
# Copyright (c) 2010 SUSE LINUX Products GmbH, Nuernberg, Germany.
#
# All modifications and additions to the file contributed by third parties
# remain the property of their copyright owners, unless otherwise agreed
# upon. The license for this file, and modifications and additions to the
# file, is the same license as for axis2c, which is the Apache License, v2.
#
# Axis2c_eo is a version axis2c of tailored for EOxServer.
# Only the installation differs from the standard axis2c  1.6.0.
# Most important functional changes are: - the spec file is no loger a
# multi-package spec. It installs only the libraries and header files needed
# to install, configure and run the soap_proxy axis service for EOxServer.
# During the build it modifies the file samples/server/axis2.xml to enable
# MTOM. On RHAT based system it also installs the axis2c apache module.
# Other changes: Changed the descrtiption, and deleted the multi-package parts.

Name:           axis2c_eo
Version:        1.6.0
Release:        3
Summary:        Web services engine for EOxServer
Group:          Applications/Internet
License:        ASL 2.0,

URL:            http://axis.apache.org/axis2/c/core/index.html
# Axis2/c has a mirror redirector for its downloads
# You can get this tarball by following a link from:
# http://axis.apache.org/axis2/c/core/download.cgi
Source0:        axis2c-src-1.6.0.tar.gz

# This is the configuration base for the httpd daemon.
Source1:        axis2c_httpd.conf

Patch0:         axis2c-docrm.diff

%define major 0
%define libname libaxis2c.0
%define logdir %{_var}/log/axis2
%define httpd_confdir %{_sysconfdir}/httpd/conf.d
%define httpd_conffile 030_axis2c.conf

#for SUSE:   apache2-devel, libapr1-devel
#for CentOs: httpd-devel, apr-devel
%if %{_vendor}==suse
BuildRequires:  apache2-devel
BuildRequires:  libapr1-devel
%else
BuildRequires:  apr-devel
BuildRequires:  httpd-devel
%endif
BuildRequires:  libxml2-devel
BuildRequires:  openssl-devel
BuildRequires:  file

Provides:       %{libname} = %{version}-%{release}
#Provides:       %{name}-devel = %{version}-%{release}
#Provides:       lib%{name}-devel = %{version}-%{release}
# BuildArch:      noarch

BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-buildroot

%description
Apache Axis2/C is a Web services engine implemented in C.
This installation is tailored for EOxServer on Cent/OS. It enables MTOM, and
installs mod_axis2 for apache httpd. The axis libraries are unchanged. Not all
axis2 modules are installed.
Axis2/C is based on the Axis2 architecture, and can be used to provide and consume
WebServices, and as a Web services enabler in other software. Apache Axis2/C 
supports SOAP 1.1 and SOAP 1.2, as well as REST style of Webservices. A single
service could be exposed both as a SOAP style as well as a REST style service
simultaneously. It also has built-in MTOM support, that can be used to exchange
binary data.


%prep
%setup -q -n axis2c-src-%{version}

cp -p %SOURCE1 .

%patch0 -p0

%build

#enable MTOM
sed -i 's^<!--parameter name="enableMTOM" locked="false">true</parameter-->^<parameter name="enableMTOM" locked="false">true</parameter>^' \
    %{_builddir}/axis2c-src-%{version}/samples/server/axis2.xml

if [ -x %{_bindir}/apr-config ]; then
    APR_CONFIG="%{_bindir}/apr-config"
else
    APR_CONFIG="%{_bindir}/apr-1-config"
fi

if [ -x %{_sbindir}/apxs ]; then
    APXS="%{_sbindir}/apxs"
else
    APXS="%{_sbindir}/apxs2"
fi

APACHE_INCLUDES="`$APXS -q INCLUDEDIR`"
APR_INCLUDES="`$APR_CONFIG --includedir`"
CFLAGS=-w

%define _libdir  /usr/lib64


%configure \
    --enable-libxml2 \
    --enable-multi-thread \
    --enable-openssl \
    --with-apache2=$APACHE_INCLUDES \
    --with-apr=$APR_INCLUDES


#make %{?_smp_mflags}

make

#add axi2c_home to pkg-config
sed -i /^includedir/aaxis2c_home=%{_datarootdir}/%{name}  axis2c.pc

%install
make install DESTDIR=%{buildroot}

mkdir -p %{buildroot}%{httpd_confdir}
cp %SOURCE1 %{buildroot}%{httpd_confdir}/%{httpd_conffile}
sed -i "s^__SET_AXIS2C_REPO_PATH__^%{_datarootdir}/%{name}^;s^__SET_AXIS2C_LOGFILE__^%{logdir}/axis2.log^" \
  %{buildroot}%{httpd_confdir}/%{httpd_conffile}

mkdir -p %{buildroot}%{_sbindir}
mv %{buildroot}%{_bindir}/axis2_http_server %{buildroot}%{_sbindir}/

#mv %%{buildroot}%%{_prefix}/axis2.xml %%{buildroot}%%{_sysconfdir}/%%{name}/
mkdir -p %{buildroot}%{_datarootdir}/%{name}
mv %{buildroot}%{_prefix}/axis2.xml %{buildroot}%{_datarootdir}/%{name}/
mv %{buildroot}%{_prefix}/modules   %{buildroot}%{_datarootdir}/%{name}/

mkdir -p %{buildroot}%{logdir}

#files not provided by eoxserver install
rm -rf %{buildroot}%{_libdir}/*.la
rm -rf %{buildroot}%{_libdir}/*.a
rm -rf %{buildroot}%{_includedir}/axis2-1.6.0/platforms/windows/*

# cleanup
rm -rf %{buildroot}%{_prefix}/logs
rm -rf %{buildroot}%{_prefix}/docs
rm -f %{buildroot}%{_prefix}/AUTHORS
rm -f %{buildroot}%{_prefix}/COPYING
rm -f %{buildroot}%{_prefix}/CREDITS
rm -f %{buildroot}%{_prefix}/INSTALL
rm -f %{buildroot}%{_prefix}/LICENSE
rm -f %{buildroot}%{_prefix}/NEWS
rm -f %{buildroot}%{_prefix}/README
rm -f %{buildroot}%{_prefix}/config.guess
rm -f %{buildroot}%{_prefix}/config.sub
rm -f %{buildroot}%{_prefix}/depcomp
rm -f %{buildroot}%{_prefix}/install-sh
rm -f %{buildroot}%{_prefix}/ltmain.sh
rm -f %{buildroot}%{_prefix}/missing
rm -f %{buildroot}%{_prefix}/NOTICE
rm -f %{buildroot}%{_datadir}/AUTHORS
rm -f %{buildroot}%{_datadir}/COPYING
rm -f %{buildroot}%{_datadir}/INSTALL
rm -f %{buildroot}%{_datadir}/LICENSE
rm -f %{buildroot}%{_datadir}/NEWS
rm -f %{buildroot}%{_datadir}/README
rm -rf %{buildroot}%{_bindir}/tools


%clean
rm -rf %{buildroot}

%files 
%defattr(-,root,root)
%attr(0644,apache,apache) %dir %{logdir}
%attr(0755,root,root) %{_sbindir}/axis2_http_server
%doc COPYING CREDITS LICENSE
%dir %{_datarootdir}/%{name}
%attr(0644,root,root) %config(noreplace) %{_datarootdir}/%{name}/axis2.xml
%dir %{_datarootdir}/%{name}/modules
%dir %{_datarootdir}/%{name}/modules/addressing
%dir %{_datarootdir}/%{name}/modules/logging
%{_datarootdir}/%{name}/modules/addressing/*
%{_datarootdir}/%{name}/modules/logging/*
%dir %{_includedir}/axis2-1.6.0
%{_includedir}/axis2-1.6.0/*.h
%dir %{_includedir}/axis2-1.6.0/platforms/
%dir %{_includedir}/axis2-1.6.0/platforms/unix
%{_includedir}/axis2-1.6.0/platforms/*.h
%{_includedir}/axis2-1.6.0/platforms/unix/*.h
%dir %{_libdir}
%dir %{_libdir}/pkgconfig
%{_libdir}/*.so
%{_libdir}/*.so.*
%{_libdir}/pkgconfig/axis2c.pc
%dir %{httpd_confdir}
%config(noreplace) %{httpd_confdir}/%{httpd_conffile}

%post
cp %{_libdir}/libmod_axis2.so.0.6.0 %{_sysconfdir}/httpd/modules/mod_axis2.so
ln -s %{_libdir} %{_datarootdir}/%{name}/lib
semanage fcontext -a -t lib_t %{_datarootdir}/%{name}/modules/addressing/libaxis2_mod_addr.so.0.6.0
semanage fcontext -a -t lib_t %{_datarootdir}/%{name}/modules/logging/libaxis2_mod_log.so.0.6.0
semanage fcontext -a -t httpd_config_t %{httpd_confdir}/%{httpd_conffile}
restorecon -v %{_datarootdir}/%{name}/modules/logging/libaxis2_mod_log.so.0.6.0
restorecon -v %{_datarootdir}/%{name}/usr/share/axis2c_eo/modules/addressing/libaxis2_mod_addr.so.0.6.0
restorecon -v %{httpd_confdir}/%{httpd_conffile}
/sbin/ldconfig
echo "httpd restart is required to operate axis2c. Please note a 'graceful-restart' is not sufficient."

%postun 
semanage fcontext -d %{_datarootdir}/%{name}/modules/addressing/libaxis2_mod_addr.so.0.6.0
semanage fcontext -d %{_datarootdir}/%{name}//modules/logging/libaxis2_mod_log.so.0.6.0
semanage fcontext -d %{httpd_confdir}/%{httpd_conffile}
restorecon -v /usr/share/axis2c_eo/modules/logging/libaxis2_mod_log.so.0.6.0
restorecon -v /usr/share/axis2c_eo/modules/addressing/libaxis2_mod_addr.so.0.6.0
restorecon -v %{httpd_confdir}/%{httpd_conffile}
/sbin/ldconfig

%changelog
* Mon Jan 23 2012 Milan Novacek 1.6.0
- Initial version.
