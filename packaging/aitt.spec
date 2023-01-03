Name: aitt
Version: 0.0.1
Release: 0
Summary: AI Telemetry Transport based on MQTT

Group: Machine Learning / ML Framework
License: Apache-2.0
Source0: %{name}-%{version}.tar.gz
Source1001: %{name}.manifest

%{?!stdoutlog: %global stdoutlog 0}
%{?!test: %global test 1}
%{?!gcov: %global gcov 0}
%{?!use_glib: %global use_glib 1}

BuildRequires: cmake
BuildRequires: pkgconfig(bundle)
BuildRequires: pkgconfig(dlog)
BuildRequires: pkgconfig(flatbuffers)
BuildRequires: pkgconfig(gmock_main)
BuildRequires: pkgconfig(libmosquitto)
BuildRequires: pkgconfig(openssl1.1)
%if %{use_glib}
BuildRequires: pkgconfig(capi-media-camera)
BuildRequires: pkgconfig(capi-media-player)
BuildRequires: pkgconfig(capi-media-sound-manager)
BuildRequires: pkgconfig(capi-media-tool)
BuildRequires: pkgconfig(capi-media-webrtc)
BuildRequires: pkgconfig(capi-media-camera)
BuildRequires: pkgconfig(gstreamer-video-1.0)
BuildRequires: pkgconfig(glib-2.0)
BuildRequires: pkgconfig(json-glib-1.0)
BuildRequires: pkgconfig(mm-display-interface)
%endif
%if %{gcov}
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

%if 0%{test}
%package unittests
Summary: Test Programs for %{name}
Group: System/Testing

%description unittests
The %{name}-unittests package contains programs for checking quality the %{name}.
%endif

%package devel
Summary: AITT development package
Group: Development/Libraries
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
    -DWITH_WEBRTC:BOOL=ON \
    -DWITH_RTSP:BOOL=ON \
    -DCMAKE_INSTALL_PREFIX:PATH=%{_prefix} \
    -DCMAKE_VERBOSE_MAKEFILE=OFF \
    -DBUILD_TESTING:BOOL=%{test} \
    -DCOVERAGE_TEST:BOOL=%{gcov} \
    -DUSE_GLIB=%{use_glib}

%__make %{?_smp_mflags}

%install
%make_install

%check
ctest --output-on-failure --timeout 30 || true

%if 0%{test} && 0%{gcov}
# Extract coverage information
lcov -c --ignore-errors graph --no-external -b . -d . -o %{name}_gcov.info
genhtml %{name}_gcov.info -o out --legend --show-details
%endif

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%manifest %{name}.manifest
%{_libdir}/lib%{name}*.so*
%license LICENSE.APLv2

%files plugins
%manifest %{name}.manifest
%{_libdir}/lib%{name}-*.so*
%license LICENSE.APLv2

%if 0%{test}
%files unittests
%{_bindir}/*
%endif

%files devel
%{_includedir}/*
%{_libdir}/pkgconfig/*.pc

%clean
rm -rf %{buildroot}
