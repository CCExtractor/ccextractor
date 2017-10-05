Name: ccextractor
Version: 0.85
Release: 1
Summary: A closed captions and teletext subtitles extractor for video streams.
Group: Applications/Internet
License: GPL
URL: http://ccextractor.org/
Source0: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root

%description
CCExtractor is a software that extracts closed captions from videos of various formats, even live video streams. Available as a multi-platform desktop application.

%global debug_package %{nil}

%prep
%setup -q

%build
./configure --enable-ocr --prefix="$pkgdir/usr"
make

%install
mkdir -p $RPM_BUILD_ROOT/usr/bin
install ccextractor $RPM_BUILD_ROOT/usr/bin/ccextractor

%files
%defattr(-,root,root)
/usr/bin/ccextractor

%clean
rm -rf $RPM_BUILD_ROOT

%changelog
* Thu Oct 5 2017 Jason Hancock <jason@jasonhancock.com>
- Install to /usr/bin instead of /usr/local/bin

* Fri Apr 14 2017 Carlos Fernandez <carlos@ccextractor.org>
- Initial build
