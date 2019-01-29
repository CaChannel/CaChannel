Summary: CaChannel Interface to EPICS
Name: python-CaChannel
Version: 3.1.2
Release: 1%{?dist}
Source0: https://pypi.io/packages/source/C/CaChannel/CaChannel-%{version}.tar.gz
License: BSD
Group: Development/Libraries
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-buildroot
Prefix: %{_prefix}
Vendor: Xiaoqiang Wang <xiaoqiang.wang AT psi DOT ch>
Url: http://pypi.python.org/pypi/cachannel

BuildRequires: python-devel python-setuptools numpy
BuildRequires: epics-base

# Do not check .so files in the python_sitearch directory
# or any files in the application's directory for provides
%global __provides_exclude_from ^(%{python_sitearch}|%{python3_sitearch})/.*\\.so$

AutoReq: 0
Requires: python-enum34 epics-base

%description
CaChannel - EPICS Channel Access in Python
==========================================

CaChannel is a Python interface to Channel Access. 

Check out `CaChannel documents <https://cachannel.readthedocs.org>`_ to get started.

* Downloads: https://pypi.python.org/pypi/CaChannel
* Source Repo: https://github.com/CaChannel/CaChannel
* Issue Tracker: https://github.com/CaChannel/CaChannel/issues
* Anaconda: https://anaconda.org/paulscherrerinstitute/cachannel



%prep
%setup -n CaChannel-%{version}

%build
env CFLAGS="$RPM_OPT_FLAGS" EPICS_SHARED="YES" python setup.py build

%install
python setup.py install --single-version-externally-managed -O1 --root=$RPM_BUILD_ROOT --record=INSTALLED_FILES

%clean
rm -rf $RPM_BUILD_ROOT

%files -f INSTALLED_FILES
%defattr(-,root,root)
