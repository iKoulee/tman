Name:		tman
Version:	0.7a
Release:	1%{?dist}
Summary:	Wrap time glibc functions and modify time for wrapped process.
Group:		
License:	GPLv3+
URL:		http://git.engineering.redhat.com/git/users/jprokes/tman.git/

BuildRequires:	kernel-headers
BuildRequires:	glibc-headers
Requires:	glibc

%description


%prep
%setup -q


%build
%configure
make %{?_smp_mflags}


%install
make install DESTDIR=%{buildroot}


%files
%doc



%changelog

