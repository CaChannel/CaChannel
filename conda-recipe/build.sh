#!/bin/bash
export EPICS_BASE=$PREFIX/epics

PLATFORM=$(uname | tr '[:upper:]' '[:lower:]')
MACHINE=$(uname -m)
if [ $PLATFORM == "linux" ] ; then
  export EPICS_HOST_ARCH=linux-$MACHINE
elif [ $PLATFORM == "darwin" ] ; then
  if [ $MACHINE == "arm64" ]; then
    export EPICS_HOST_ARCH=darwin-aarch64
  elif [ $MACHINE == "x86_64" ]; then
    export EPICS_HOST_ARCH=darwin-x86
  else
    echo "macOS CPU type '$MACHINE' not recognized"
    exit 1
  fi
fi


echo Using EPICS_BASE=$EPICS_BASE
echo Using EPICS_HOST_ARCH=$EPICS_HOST_ARCH

OUTPUT_PATH=$(dirname $(conda build --output conda-recipe))

if [ $PLATFORM == "linux" ]; then
    $PYTHON setup.py sdist
    cp -f dist/*.tar.gz ${OUTPUT_PATH}
    $PYTHON -m pip install .
elif [ $PLATFORM == "darwin" ]; then
    $PYTHON -m pip install wheel .
    cp dist/*.whl ${OUTPUT_PATH}
fi
