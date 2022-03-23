Name: aitt
Version: 0.0.1
Release: 0
Summary: AI Telemetry Transport based on MQTT

Group: Machine Learning / ML Framework
License: Apache-2.0
Source0: %{name}-%{version}.tar.gz
Source1001: %{name}.manifest

%{!?stdoutlog: %global stdoutlog 0}
%{!?test: %global test 1}
%{!?gcov: %global gcov 0}

BuildRequires: cmake
BuildRequires: pkgconfig(dlog)
BuildRequires: pkgconfig(flatbuffers)
BuildRequires: pkgconfig(glib-2.0)
BuildRequires: pkgconfig(libmosquitto)
#BuildRequires: pkgconfig(libsrtp2)
BuildRequires: pkgconfig(gmock_main)
%if 0%{gcov}
BuildRequires: lcov
%endif

%description
AITT is a Framework which transfers data of AI service.
It makes distributed AI Inference possible.

%package plugins
Summary: Plugin Libraries for AITT P2P transport
Group: Machine Learning / ML Framework
Requires: %{name} = %{version}

%description plugins
The %{name}-plugins package contains basic plugin libraries for AITT P2P transport.

%package devel
Summary: AITT development package
Group: Development/Libraries
Requires: aitt-devel
Requires: %{name} = %{version}-%{release}

%description devel
The %{name}-devel package contains libraries and header files for
developing programs that use %{name}.

%prep
%setup -q
cp %{SOURCE1001} .

%build
%cmake . \
    -DLOG_STDOUT:BOOL=%{stdoutlog} \
    -DPLATFORM="tizen" \
    -DVERSIONING:BOOL=OFF \
    -DCMAKE_INSTALL_PREFIX:PATH=%{_prefix} \
    -DCMAKE_VERBOSE_MAKEFILE=OFF \
    -DBUILD_TESTING:BOOL=%{test} \
    -DCOVERAGE_TEST:BOOL=%{gcov}

%__make %{?_smp_mflags}

%install
%make_install

%check
ctest --output-on-failure --timeout 20 || true

%if 0%{test} && 0%{gcov}
# Extract coverage information
lcov -c --ignore-errors graph --no-external -b . -d . -o %{name}_gcov.info
genhtml %{name}_gcov.info -o out --legend --show-details
%endif

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%manifest %{name}.manifest
%if 0%{test}
%{_bindir}/*
%endif
%{_libdir}/lib%{name}.so*
%license LICENSE.APLv2

%files plugins
%manifest %{name}.manifest
%{_libdir}/lib%{name}-transport*.so*
%license LICENSE.APLv2

%files devel
%{_includedir}/*
%{_libdir}/pkgconfig/*.pc

%clean
rm -rf %{buildroot}
