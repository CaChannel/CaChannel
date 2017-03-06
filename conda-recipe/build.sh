#!/bin/bash
export EPICS_BASE=$PREFIX/epics

PLATFORM=$(uname | tr '[:upper:]' '[:lower:]')
if [ $PLATFORM == "linux" ] ; then
  export EPICS_HOST_ARCH=$(uname | tr '[:upper:]' '[:lower:]')-$(uname -m)
elif [ $PLATFORM == "darwin" ] ; then
  export EPICS_HOST_ARCH=darwin-x86
fi

echo Using EPICS_BASE=$EPICS_BASE
echo Using EPICS_HOST_ARCH=$EPICS_HOST_ARCH

OUTPUT_PATH=$(dirname $(conda build --output conda-recipe))

case `uname` in
    Darwin )
        CAREPEATER=$RECIPE_DIR/caRepeater.Darwin
        $PYTHON setup.py install bdist_wheel
        cp dist/*.whl ${OUTPUT_PATH}
        ;;
    Linux )
        CAREPEATER=$RECIPE_DIR/caRepeater.Linux$ARCH
        $PYTHON setup.py install sdist bdist_egg
        cp dist/*.tar.gz ${OUTPUT_PATH}
        cp dist/*.egg ${OUTPUT_PATH}
        ;;
    * )
        echo "Not Supported"
esac

# Add more build steps here, if they are necessary.

# See
# http://docs.continuum.io/conda/build.html
# for a list of environment variables that are set during the build process.
