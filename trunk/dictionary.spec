%define ver      2.4.2
%define rel      1
%define prefix   /usr
%define dictname  oxford-gb


Summary: %{dictname} dictionary data files for StarDict2
Name: stardict-%{dictname}
Version: %ver
Release: %rel
License:	GPL
Group: 		Applications/System
Source: stardict-%{dictname}-%{ver}.tar.bz2
BuildRoot: /var/tmp/%{name}-%{version}-root
BuildArchitectures: noarch

URL: http://stardict.sourceforge.net

Requires: stardict >= 2.4.2

%description
%{dictname} dictionary data files for StarDict2.

%prep
%setup

%build
%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT%{prefix}/share/stardict/dic/stardict-%{dictname}-%{ver}
cp -rf . $RPM_BUILD_ROOT%{prefix}/share/stardict/dic/stardict-%{dictname}-%{ver}

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-, root, root)

%{prefix}/share/stardict/dic
